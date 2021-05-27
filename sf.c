#include <windows.h>
LPVOID Engine, Surface, EntityList, Panel, Client, ClientMode, Events, Globals, ConsoleVars, Sound;
INT WINAPI MH_Initialize(VOID);
INT WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
INT WINAPI MH_EnableHook(LPVOID pTarget);
#define METHOD(RType, Type, Name, RawArgs, Interface, Index, ...) __forceinline RType Interface##_##Name RawArgs {return (((Type)(((PUINT*)(Interface))[0][Index]))(Interface, 0, __VA_ARGS__));} // Ensure EDX is NULL for __thiscall emulation
METHOD(LPCSTR, LPCSTR(__fastcall*)(LPVOID, PVOID, ULONG32), GetPanelName, (ULONG32 luPanelID), Panel, 36, luPanelID);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, USHORT, USHORT, USHORT, USHORT), SetColor, (USHORT r, USHORT g, USHORT b, USHORT a), Surface, 15, r, g, b, a);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, INT, INT, INT, INT), DrawFilledRect, (INT x, INT y, INT w, INT h), Surface, 16, x, y, x + w, y + h);
VOID(__fastcall* PaintTraverseOriginal)(LPVOID, PVOID, DWORD, BOOLEAN, BOOLEAN);
VOID WINAPI _PaintTraverse(DWORD dwPanel, BOOLEAN bForceRepaint, BOOLEAN bAllowRepaint) { 
	if (strstr(Panel_GetPanelName(dwPanel), "MatSystemTopPanel")) {
		Surface_SetColor(255, 255, 255, 255);
		Surface_DrawFilledRect(20, 20, 30, 30);
	}
	return PaintTraverseOriginal(Panel, 0, dwPanel, bForceRepaint, bAllowRepaint);
}
LPVOID CreateInterface(HANDLE hModule, LPCSTR szName) {
	LPVOID(*pfnCreateInterface)(LPCSTR, INT) = (LPVOID(*)(LPCSTR, INT))GetProcAddress(hModule, "CreateInterface");
	return pfnCreateInterface(szName, 0);
}
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		AllocConsole();
		SetConsoleTitleA("SingleFile");
		WriteConsoleA(GetStdHandle((ULONG32)-11), "SingleFile v2.0 Alpha Loaded", 28, NULL, NULL);
		Surface = CreateInterface(GetModuleHandleA("vguimatsurface.dll"), "VGUI_Surface031");
		Panel = CreateInterface(GetModuleHandleA("vgui2.dll"), "VGUI_Panel009");
		MH_Initialize();
		MH_CreateHook((*(PVOID**)(Panel))[41], &_PaintTraverse, (PVOID*)&PaintTraverseOriginal);
		MH_EnableHook(NULL);
	}
	return TRUE;
}