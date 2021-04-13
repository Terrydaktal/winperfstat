// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <setupapi.h>

int InstallAndStartDriver() {

	UINT ErrorLine;
	PCWSTR InfFileName = L"winperfstat.inf";
	PCWSTR DriverName = L"winperfstat";
	HINF HInf = SetupOpenInfFile(InfFileName, NULL, INF_STYLE_WIN4, &ErrorLine);
	PCWSTR SourceFile = L"winperfstat.sys";
	PCWSTR DriverInstallPath = L"C:\\Windows\\system32\\drivers\\winperfstat.sys";
	LPCSTR SubKey = "\\System\\CurrentControlSet\\Services\\winperfstat";
	HKEY hKey;
	BYTE ErrorControl = 0x1;

	if (RegCreateKeyExA(HKEY_LOCAL_MACHINE,
		SubKey,
		NULL, NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&hKey,
		NULL)) {

		if (RegSetValueEx(hKey, L"DriverName", NULL, REG_SZ, (LPBYTE)DriverName, sizeof(wchar_t)*(wcslen(DriverName) + 1))
		&& RegSetValueEx(hKey, L"DisplayName", NULL, REG_DWORD, (LPBYTE)DriverName, sizeof(wchar_t)*(wcslen(DriverName) + 1))
		&& RegSetValueEx(hKey, L"ErrorControl", NULL, REG_SZ, (LPBYTE)&ErrorControl, sizeof(DWORD))
		) {
			return false;
		}
	}
	else {
		return false;
	}

	if (SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) {

		if (SetupInstallFileEx(HInf, NULL, SourceFile, NULL, DriverInstallPath,
			SP_COPY_NEWER_OR_SAME, NULL, NULL, FALSE)) {

			if (SC_HANDLE service = CreateService(manager,
				DriverName,
				DriverName,
				SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER,
				SERVICE_AUTO_START,
				SERVICE_ERROR_NORMAL,
				DriverInstallPath,
				NULL, NULL, NULL, NULL, NULL)
				) {
				
				if (!StartService(service, NULL, NULL)) {
					return false;
				}
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}

	return true;
}

int main(int argc, char** argv)
{
	HANDLE hDevice;
	PCWSTR SymLink = L"\\\\.\\winperfstat\\";
	DWORD bytesReturned;
	LPCSTR AppName = argv[1];
	HMODULE hApp;
	PIMAGE_NT_HEADERS64 PEHeader;
	char bufferOut[1000] = { 0 };

	if (!InstallAndStartDriver()) {  //if driver not installed, install; if driver not started, start
		std::cout << "error";        //if error during install start / install check
		return false;     
	}; 

	hApp = LoadLibraryA(AppName); 
	PEHeader = ((PIMAGE_NT_HEADERS64)((PBYTE)hApp + (int)((PBYTE)hApp + 0x3c)));
	VirtualLock(hApp, PEHeader->OptionalHeader.SizeOfImage);
	argv[0] = (char*)hApp + (int)PEHeader->OptionalHeader.BaseOfCode;
	((void(*)())(argv[0]))(); //calls the benchmark function, making sure it returns

	hDevice = CreateFileW(SymLink,
		FILE_READ_ACCESS|FILE_WRITE_ACCESS,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0 , NULL);

	DeviceIoControl(hDevice,
		0x69,
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
