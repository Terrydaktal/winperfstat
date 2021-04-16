#include <iostream>
#include <Windows.h>
#include <setupapi.h>

#define BENCHMARK_DRV_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_NEITHER, FILE_ANY_ACCESS)

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

		HINF HInf = SetupOpenInfFile(InfFileName, NULL, INF_STYLE_WIN4|INF_STYLE_OLDNT, &ErrorLine);
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
	LPCSTR AppName = argv[1];
	HMODULE hApp;
	PIMAGE_NT_HEADERS64 PEHeader;
	size_t bufferSize = sizeof(unsigned long long)*argc;
	unsigned long long* bufferOut = (unsigned long long*) calloc (argc, sizeof(unsigned long long));
	int result;
	result = VirtualLock(bufferOut, bufferSize);
	result = VirtualLock((LPVOID)argv, bufferSize);

	if (int i = InstallAndStartDriver()) {  //if driver not installed, install; if driver not started, start
		std::cout << "error" << i;        //if error during install start / install check
		return false;     
	}; 

	hApp = LoadLibraryA(AppName); 
	PEHeader = ((PIMAGE_NT_HEADERS64)((PBYTE)hApp + (int)(*((PBYTE)hApp + 0x3c))));
	result = VirtualLock(hApp, PEHeader->OptionalHeader.SizeOfImage);
	argv[0] = (char*)hApp + (int)PEHeader->OptionalHeader.BaseOfCode;
	((void(*)())(argv[0]))(); //calls the benchmark function, making sure it returns and without error
	                          //don't need SEH handler because there's one in RtlUserThreadStart

	hDevice = CreateFileW(SymLink,
		FILE_READ_ACCESS|FILE_WRITE_ACCESS,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0 , NULL);

	result = DeviceIoControl (
		hDevice,
		BENCHMARK_DRV_IOCTL,
		(LPVOID) argv,
		bufferSize,
		(LPVOID) bufferOut,
		bufferSize,
		&bytesReturned, NULL);

	CloseHandle(hDevice);
	
	result = VirtualUnlock(hApp, PEHeader->OptionalHeader.SizeOfImage);
	result = VirtualUnlock(bufferOut, bufferSize);
	result = VirtualUnlock((LPVOID)argv, bufferSize);

	FreeLibrary(hApp);
	
	std::cout << "\nPerformance Counter stats for " << AppName << ":\n\n";

	for (int i = 2; i < argc; i++) {
		std::cout << "    " << argv[i] << ":   " << bufferOut[i] << "\n";
	}

	std::cout << "\n=< Note: the counters are benchmarked in batches of 4\n";

	free(bufferOut);

	return 0;
}

