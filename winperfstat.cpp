#include <iostream> //required for std::cout
#include <Windows.h> //required for for function and structure type definitons and preprocessor definitions
#include <setupapi.h> //required for SetupOpenInfFile and SetupInstallFileW type defintions

#define BENCHMARK_DRV_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_NEITHER, FILE_ANY_ACCESS)

int InstallAndStartDriver(void);
LPCSTR AppName;
PVOID ExceptionAddress;

int InstallAndStartDriver() {

	UINT ErrorLine;
	PCWSTR InfFileName = L"C:\\Users\\lewis\\source\\repos\\ConsoleApplication2\\x64\\Release\\winperfstat.inf";
	PCWSTR DriverName = L"winperfstat";
	PCWSTR SourceFile = L"winperfstat.sys";
	PCWSTR SourcePathRoot = L"C:\\Users\\lewis\\source\\repos\\ConsoleApplication2\\x64\\Release\\";
	PCWSTR DriverInstallPath = L"C:\\Users\\lewis\\source\\repos\\ConsoleApplication2\\x64\\Release\\winperfstat.sys";
	LPCSTR SubKey = "System\\CurrentControlSet\\Services\\winperfstat";
	HKEY hKey;
	DWORD ErrorControl = 1;
	DWORD StartType = 3;
	PBOOL FileWasInUse = NULL;
	LSTATUS status;

	if (status = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
		SubKey,
		NULL, NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&hKey,
		NULL))
	{
		return 1;
	}
	else {
		if (RegSetValueEx(hKey, L"DriverName", NULL, REG_SZ, (LPBYTE)DriverName, sizeof(wchar_t)*(wcslen(DriverName) + 1))
			|| RegSetValueEx(hKey, L"DisplayName", NULL, REG_SZ, (LPBYTE)DriverName, sizeof(wchar_t)*(wcslen(DriverName) + 1))
			|| RegSetValueEx(hKey, L"ErrorControl", NULL, REG_DWORD, (LPBYTE)&ErrorControl, sizeof(DWORD))
			)
		{
			return 2;
		}
	}

	if (SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) {

		HINF HInf = SetupOpenInfFile(InfFileName, NULL, INF_STYLE_WIN4 | INF_STYLE_OLDNT, &ErrorLine);
		if (SetupInstallFileW(HInf, NULL, SourceFile, SourcePathRoot, DriverInstallPath,
			SP_COPY_NEWER_OR_SAME, NULL, FileWasInUse)) {

			SC_HANDLE service;
			service = CreateService(manager,
				DriverName,
				DriverName,
				SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER,
				SERVICE_DEMAND_START,
				SERVICE_ERROR_NORMAL,
				DriverInstallPath,
				NULL, NULL, NULL, NULL, NULL);

			if (GetLastError() == ERROR_SERVICE_EXISTS) { //1073
				service = OpenService(manager, DriverName, SERVICE_ALL_ACCESS);
			}

			status = StartService(service, NULL, NULL);
			DWORD error = GetLastError();
			return  error == ERROR_SERVICE_ALREADY_RUNNING | status == TRUE ? 0 : error; //1056
		}

		else {
			return 4;
		}
	}
	else {
		return 5;
	}
	return 0;
}



int main(int argc, CHAR* volatile * argv)
{
	HANDLE hDevice;
	PCWSTR SymLink = L"\\\\.\\winperfstat";
	DWORD bytesReturned;
	HMODULE hApp;
	PIMAGE_NT_HEADERS64 PEHeader;

	//sanity check of parameters

	if (argc < 3) {   //if fewer than 3 arguments
		if (argc < 2) {   //if fewer than 2 arguments
			std::cout << "invalid usage, use /? for help";
		}
		else if (!strcmp("/?", argv[1])) {  //if 2 arguments and the 2nd argument is /?
			std::cout << "winperfstat version 1.0.0\n";
			std::cout << "usage: winperfstat [executable] [event list]";
		} 
		else {  //otherwise it means an app was supplied but no events
			std::cout << "no events supplied to benchmark";
		}

		return false;
	}

	if (int i = InstallAndStartDriver()) {  //if driver not installed, install; if driver not started, start
		std::cout << "error" << i;        //if error during install start / install check
		return false;
	};

	AppName = argv[1];
	hApp = LoadLibraryA(AppName);   //attempt to open a handle to an application of that name

	if (!hApp) {
		std::cout << "invalid application";   //if it fails, return
		return false;
	}

	// get start of COFF file header using the address of the executable in memory
	// and then get the address of the entry point fron the optional PE header

	PEHeader = ((PIMAGE_NT_HEADERS64)((PBYTE)hApp + (int)(*((PBYTE)hApp + 0x3c))));
	argv[0] = (char*)hApp + (int)PEHeader->OptionalHeader.AddressOfEntryPoint;
	
	__try {
		((void(*)())(argv[0]))(); //calls the benchmark function, making sure it returns and without error
	}
	__except ((ExceptionAddress = ((LPEXCEPTION_POINTERS)(GetExceptionInformation()))->ExceptionRecord->ExceptionAddress) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_EXECUTE_HANDLER) {
		// GetExceptionInformation can only be used in the exception filter, so we perform it and store it in the data section
		std::cout << AppName << " crashed with exception: " << std::hex << GetExceptionCode() << " at code section offset: " << (unsigned long long) ExceptionAddress - (unsigned long long) hApp - PEHeader->OptionalHeader.BaseOfCode;

		return false;
	}


	//lock the benchmark executable into memory so that there isn't a page fault during execution
	//which will hang the system at IRQL >2
	int result = 1;
	result &= VirtualLock(hApp, PEHeader->OptionalHeader.SizeOfImage);

	//allocate and zero the output buffer on the heap
	size_t bufferSize = sizeof(unsigned long long)*argc;
	unsigned long long* bufferOut = (unsigned long long*) calloc(argc, sizeof(unsigned long long));
	
	//lock the output and input buffer into memory so that there isn't a page fault during execution
    //which will hang the system at IRQL >2
	result &= VirtualLock(bufferOut, bufferSize);
	result &= VirtualLock((LPVOID)argv, bufferSize);

	

	//open a handle to the device
	hDevice = CreateFileW(SymLink,
		FILE_READ_ACCESS | FILE_WRITE_ACCESS,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);

	//if any of the above fail, then free potential buffer and unlock any ranges
	if (!result || !hDevice) goto fail;

	//send an IOCTL to the driver
	result = DeviceIoControl(
		hDevice,
		BENCHMARK_DRV_IOCTL,
		(LPVOID)argv,
		bufferSize,
		(LPVOID)bufferOut,
		bufferSize,
		&bytesReturned, NULL);

	std::cout << "\nPerformance Counter stats for " << AppName << ":\n\n";

	for (int i = 2; i < argc; i++) {
		std::cout << "    " << argv[i] << ":   " << bufferOut[i] << "\n";
	}

	std::cout << "\n=< Note: the events are benchmarked in batches of 4\n";
	std::cout << "=<Use 1 counter for the least noise";

	//close the handle to the device
	CloseHandle(hDevice);
	
	fail:   //this will fail when there is no handle, so no need to close handle

	//unlock any ranges
	VirtualUnlock(hApp, PEHeader->OptionalHeader.SizeOfImage);
	VirtualUnlock(bufferOut, bufferSize);
	VirtualUnlock((LPVOID)argv, bufferSize);

	//unload the library
	FreeLibrary(hApp);

	//free the heap allocation
	free(bufferOut);

	return 0;
}

