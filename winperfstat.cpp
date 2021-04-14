// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <setupapi.h>

#define BENCHMARK_DRV_IOCTL 0x69

int InstallAndStartDriver() {

	UINT ErrorLine;
	PCWSTR InfFileName = L"C:\\Users\\lewis\\source\\repos\\ConsoleApplication2\\x64\\Release\\winperfstat.inf";
	PCWSTR DriverName = L"winperfstat";
	PCWSTR SourceFile = L"winperfstat.sys";
	PCWSTR SourcePathRoot = L"C:\\Users\\lewis\\source\\repos\\ConsoleApplication2\\x64\\Release\\";
	PCWSTR DriverInstallPath = L"C:\\Windows\\system32\\drivers\\winperfstat.sys";
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
		return status;
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
				SERVICE_AUTO_START,
				SERVICE_ERROR_NORMAL,
				DriverInstallPath,
				NULL, NULL, NULL, NULL, NULL);

			RegSetValueEx(hKey, L"Start", NULL, REG_DWORD, (LPBYTE)&StartType, sizeof(DWORD));
				
			if (GetLastError() == ERROR_SERVICE_EXISTS) { //1073
				service = OpenService(manager, DriverName, SERVICE_ALL_ACCESS);
			}

			status = StartService(service, NULL, NULL);
			return GetLastError() == ERROR_SERVICE_ALREADY_RUNNING | status == TRUE ? 0 : 3; //1056
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

int main(int argc, CHAR** argv)
{
	HANDLE hDevice;
	PCWSTR SymLink = L"\\\\.\\winperfstat\\";
	DWORD bytesReturned;
	LPCSTR AppName = argv[1];
	HMODULE hApp;
	PIMAGE_NT_HEADERS64 PEHeader;
	char bufferOut[1000] = { 0 };

	if (int i = InstallAndStartDriver()) {  //if driver not installed, install; if driver not started, start
		std::cout << "error" << i;        //if error during install start / install check
		return false;     
	}; 

	int n;
	std::cout << "done1";
	std::cin >> n;

	hApp = LoadLibraryA(AppName); 
	PEHeader = ((PIMAGE_NT_HEADERS64)((PBYTE)hApp + (int)(*((PBYTE)hApp + 0x3c))));
	VirtualLock(hApp, PEHeader->OptionalHeader.SizeOfImage);
	argv[0] = (char*)hApp + (int)PEHeader->OptionalHeader.BaseOfCode;
	((void(*)())(argv[0]))(); //calls the benchmark function, making sure it returns

	hDevice = CreateFileW(SymLink,
		FILE_READ_ACCESS|FILE_WRITE_ACCESS,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0 , NULL);
	
	std::cout << "done2";

	DeviceIoControl(hDevice,
		BENCHMARK_DRV_IOCTL,
		argv,
		sizeof(char*)*argc,
		bufferOut,
		sizeof(bufferOut),
		&bytesReturned, NULL);

	VirtualUnlock(hApp, PEHeader->OptionalHeader.SizeOfImage);
	FreeLibrary(hApp);
	for (int i = 2; i < argc; i++) {
		std::cout << argv[i] << ":   " << ((unsigned long long*)bufferOut)[i] << "\n";
	}

	return 0;
}

