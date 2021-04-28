#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <TlHelp32.h>
#include <shobjidl.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#define Assert(exp, msg) if (!(exp)) {MessageBoxA(0, msg, "SingleFile Injector", MB_ICONERROR); __debugbreak(); return FALSE;}

DWORD GetProcessID(LPCSTR szProcessName) {
	HANDLE hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);;
	PROCESSENTRY32 preProcEntry;
	DWORD top = 0x0;
	preProcEntry.dwSize = sizeof(preProcEntry);
	do {
		if (strstr(preProcEntry.szExeFile, szProcessName)) {
			DWORD dwPID = preProcEntry.th32ProcessID;
			CloseHandle(hProcess);
			top = dwPID;
			goto finish;
		}
	} while (Process32Next(hProcess, &preProcEntry));
finish:
	Assert(top, "Failed to access CS:GO! Is it running or are you admin?");
	return top;
}
BOOL WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, INT nShowCmd) {
	char szDLLPath[MAX_PATH];
	GetFullPathNameA("SingleFile.dll", MAX_PATH, szDLLPath, NULL);
	if (!PathFileExistsA(szDLLPath)) {
		if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) >= 0) {
			IFileOpenDialog* pFileDialog;
			COMDLG_FILTERSPEC pFileType[] = {
				L"Injectable Modules", L"*.dll"
			};
			if (CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)(&pFileDialog)) >= 0) {
				pFileDialog->SetFileTypes(1, pFileType);
				if (pFileDialog->Show(NULL) >= 0) {
					IShellItem* pDLL;
					pFileDialog->GetResult(&pDLL);
					LPWSTR pwszFile;
					pDLL->GetDisplayName(SIGDN_FILESYSPATH, &pwszFile);
					wcstombs(szDLLPath, pwszFile, wcslen(pwszFile) + 1);
				}
			}
		}
	}
	DWORD dwPID = GetProcessID("csgo.exe");
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	HMODULE hNTDLL = LoadLibraryA("ntdll.dll");
	PVOID pNTOpenFileOriginal = malloc(0x5);
	Assert(hNTDLL, "Failed to obtain NTDLL handle!");
	FARPROC pNTOpenFileAddress = GetProcAddress(hNTDLL, "NtOpenFile");
	Assert(pNTOpenFileOriginal, "Failed to allocate memory!");
	Assert(pNTOpenFileAddress, "Failed to get address of NtOpenFile!");
	memcpy(pNTOpenFileOriginal, pNTOpenFileAddress, 0x5);
	WriteProcessMemory(hProc, pNTOpenFileAddress, pNTOpenFileOriginal, 0x5, NULL);
	LPVOID pNewMemory = VirtualAllocEx(hProc, 0x0, strlen(szDLLPath) + 0x1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	Assert(pNewMemory, "Failed to allocate memory!");
	WriteProcessMemory(hProc, pNewMemory, szDLLPath, strlen(szDLLPath) + 0x1, NULL);
	CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pNewMemory, 0x0, 0x0);
	free(pNTOpenFileOriginal);
	CloseHandle(hProc);
	return TRUE;
}