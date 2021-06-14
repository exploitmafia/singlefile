#include <windows.h>
LPVOID Engine, Surface, EntityList, Panel, Client, ClientMode, Events, Globals, ConsoleVars, Sound;
INT WINAPI MH_Initialize(VOID);
INT WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
INT WINAPI MH_EnableHook(LPVOID pTarget);
/*CSTD lib*/ ULONG32 WideStringLength(LPCWSTR StringPointer) { ULONG32 Length = 0; while (StringPointer[Length]) Length++; return Length; };  LPSTR StringFindChar(LPCSTR String, CHAR _Character) { CONST CHAR Character = _Character; for (; String[0] != Character; ++String) { if (!String[0]) return NULL; } return String; }; LPSTR StringFindString(LPCSTR Source, LPSTR String) { if (!String[0]) return (LPSTR)Source; for (; (Source = StringFindChar(Source, String[0])) != NULL; ++Source) { LPCSTR TMPV1, TMPV2; for (TMPV1 = Source, TMPV2 = String;;) { if ((++TMPV2)[0]) return (LPSTR)Source; else if ((++TMPV1)[0] != TMPV2[0]) break; }} return NULL; }; ULONG32 StringLength(LPSTR String) { ULONG32 Iterator = 0; while (String[Iterator]) Iterator++; return Iterator; }; // hehe
#define METHOD(RType, Type, Name, RawArgs, Interface, Index, ...) __forceinline RType Interface##_##Name RawArgs {return (((Type)(((PULONG32*)(Interface))[0][Index]))(Interface, 0, __VA_ARGS__));} // Ensure EDX is NULL for __thiscall emulation
METHOD(LPCSTR, LPCSTR(__fastcall*)(LPVOID, PVOID, ULONG32), GetPanelName, (ULONG32 luPanelID), Panel, 36, luPanelID);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, USHORT, USHORT, USHORT, USHORT), SetColor, (USHORT r, USHORT g, USHORT b, USHORT a), Surface, 15, r, g, b, a);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, INT, INT, INT, INT), DrawFilledRect, (INT x, INT y, INT w, INT h), Surface, 16, x, y, x + w, y + h);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, USHORT, USHORT, USHORT, USHORT), SetTextColor, (USHORT r, USHORT g, USHORT b, USHORT a), Surface, 25, r, g, b, a);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, ULONG32), SetTextPosition, (ULONG32 x, ULONG32 y), Surface, 26, x, y);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, LPCWSTR, ULONG32), DrawText, (LPCWSTR StringPointer), Surface, 28, StringPointer, WideStringLength(StringPointer));
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32), SetTextFont, (ULONG32 Font), Surface, 23, Font);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, ULONG32, ULONG32, ULONG32), DrawOutline, (ULONG32 x, ULONG32 y, ULONG32 w, ULONG32 h), Surface, 18, x, y, x + w, y + h);
VOID(__fastcall* PaintTraverseOriginal)(LPVOID, PVOID, DWORD, BOOLEAN, BOOLEAN);
VOID WINAPI _PaintTraverse(DWORD dwPanel, BOOLEAN bForceRepaint, BOOLEAN bAllowRepaint) { 
	if (StringFindString(Panel_GetPanelName(dwPanel), "MatSystemTopPanel")) {
		Surface_SetColor(255, 255, 255, 255);
		Surface_DrawFilledRect(20, 20, 30, 30);
		LPCSTR Test = StringFindString(Panel_GetPanelName(dwPanel), "MatSystemTopPanel");
		WriteConsoleA(GetStdHandle((ULONG32)-11), Test, StringLength(Test), NULL, NULL);
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
		WriteConsoleA(GetStdHandle((ULONG32)-11), "SingleFile v2.0 Alpha Loaded\n", 28, NULL, NULL);
		Surface = CreateInterface(GetModuleHandleA("vguimatsurface.dll"), "VGUI_Surface031");
		Panel = CreateInterface(GetModuleHandleA("vgui2.dll"), "VGUI_Panel009");
		MH_Initialize();
		MH_CreateHook((*(PVOID**)(Panel))[41], &_PaintTraverse, (PVOID*)&PaintTraverseOriginal);
		MH_EnableHook(NULL);
	}
	return TRUE;
}