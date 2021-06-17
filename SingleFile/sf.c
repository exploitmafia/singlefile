#include <windows.h>
LPVOID Engine, Surface, EntityList, Panel, Client, ClientMode, Events, Globals, ConsoleVars, Sound; WNDPROC OriginalWndProc; HANDLE Window;
INT WINAPI MH_Initialize(VOID);
INT WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
INT WINAPI MH_EnableHook(LPVOID pTarget);
/*CSTD lib*/ ULONG32 WideStringLength(LPCWSTR StringPointer) { ULONG32 Length = 0; while (StringPointer[Length]) Length++; return Length; }; LPSTR StringFindChar(LPCSTR String, CHAR _Character) { CONST CHAR Character = _Character; for (; String[0] != Character; ++String) { if (!String[0]) return NULL; } return String; }; LPSTR StringFindString(LPCSTR Source, LPSTR String) { if (!String[0]) return (LPSTR)Source; for (; (Source = StringFindChar(Source, String[0])) != NULL; ++Source) { LPCSTR TMPV1, TMPV2; for (TMPV1 = Source, TMPV2 = String;;) { if ((++TMPV2)[0]) return (LPSTR)Source; else if ((++TMPV1)[0] != TMPV2[0]) break; }} return NULL; }; ULONG32 StringLength(LPSTR String) { ULONG32 Iterator = 0; while (String[Iterator]) Iterator++; return Iterator; }; // hehe
#define METHOD(RType, Type, Name, RawArgs, Interface, Index, ...) __forceinline RType Interface##_##Name RawArgs {return (((Type)(((PULONG32*)(Interface))[0][Index]))(Interface, 0, __VA_ARGS__));} // Ensure EDX is NULL for __thiscall emulation
#define NMETHOD(RType, Type, Name, RawArgs, Interface, Index) __forceinline RType Interface##_##Name RawArgs {return (((Type)(((PULONG32*)(Interface))[0][Index]))(Interface, 0));} // Ensure EDX is NULL for __thiscall emulation
#define OFFSET(RType, Name, Offset) __forceinline RType CBaseEntity_##Name (PBYTE Entity) {return *(RType*)(Entity + Offset); };
struct TAGConfig {
	BOOLEAN bBunnyHop, bAutoStrafe; // Movement
}Config;
struct vec3 {
	FLOAT x, y, z;
};
struct CUserCmd {
	ULONG32 : 32; // Padding
	LONG CommandNumber, TickCount;
	struct vec3 Angles, Direction;
	FLOAT Forward, Side, Up;
	INT Buttons;
};
METHOD(LPCSTR, LPCSTR(__fastcall*)(LPVOID, PVOID, ULONG32), GetPanelName, (ULONG32 luPanelID), Panel, 36, luPanelID);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, WORD, WORD, WORD, WORD), SetColor, (WORD r, WORD g, WORD b, WORD a), Surface, 15, r, g, b, a);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, INT, INT, INT, INT), DrawFilledRect, (INT x, INT y, INT w, INT h), Surface, 16, x, y, x + w, y + h);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, WORD, WORD, WORD, WORD), SetTextColor, (WORD r, WORD g, WORD b, WORD a), Surface, 25, r, g, b, a);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, ULONG32), SetTextPosition, (ULONG32 x, ULONG32 y), Surface, 26, x, y);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, LPCWSTR, ULONG32), DrawText, (LPCWSTR StringPointer), Surface, 28, StringPointer, WideStringLength(StringPointer));
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32), SetTextFont, (ULONG32 Font), Surface, 23, Font);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, ULONG32, ULONG32, ULONG32), DrawOutline, (ULONG32 x, ULONG32 y, ULONG32 w, ULONG32 h), Surface, 18, x, y, x + w, y + h);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, LPCWSTR, PULONG32, PULONG32), GetTextSize, (ULONG32 Font, LPCWSTR StringPointer, PULONG32 X, PULONG32 Y), Surface, 79, Font, StringPointer, X, Y);
NMETHOD(INT, INT(__fastcall*)(LPVOID, PVOID), GetLocalPlayer, (VOID), Engine, 12);
METHOD(PVOID, PVOID(__fastcall*)(LPVOID, PVOID, ULONG32), GetEntity, (ULONG32 luIndex), EntityList, 3, luIndex);
OFFSET(INT, Health, 0x100);
OFFSET(INT, MoveType, 0x25C);
OFFSET(INT, MoveFlags, 0x104);
BOOLEAN bMenuActive, bClicked, bInMove, bDragging, bItem, bWasClicked; ULONG32 MenuX, MenuY, ActiveX, ActiveY, LastX, LastY; WORD ActiveElement; // ActiveX CS:GO Hackage Package
BOOLEAN __fastcall Utils_InRange(WORD x, WORD y, WORD w, WORD h) {
	return (LastX >= x && LastY >= y && LastX <= x + w && LastY <= y + h);
}
VOID Menu_RunInput(ULONG32 RawParams) {
	if (!bClicked) {
		bDragging = FALSE;
		LastX = LOWORD(RawParams); LastY = HIWORD(RawParams);
	}
	if (Utils_InRange(MenuX, MenuY, 420, 20) && !bItem)
		bDragging = TRUE;
	if (bDragging) {
		MenuX += LOWORD(RawParams) - LastX;
		MenuY += HIWORD(RawParams) - LastY;
	}
	LastX = LOWORD(RawParams); LastY = HIWORD(RawParams);
}
BOOLEAN Menu_IsClicked(WORD x, WORD y, WORD w, WORD h) {
	if (bDragging)
		return FALSE;
	if (GetAsyncKeyState(VK_LBUTTON) & 1 && Utils_InRange(x, y, w, h))
		return TRUE;
	return FALSE;
}
VOID Menu_Window(LPCWSTR WindowName, ULONG32 Width, ULONG32 Height) {
	ULONG32 X, Y;
	Surface_SetTextFont(0x11);
	Surface_SetColor(0x17, 0x17, 0x1E, 0xFF);
	Surface_DrawOutline(MenuX - 0x1, MenuY - 0x1, Width + 0x2, Height + 0x2);
	Surface_SetColor(0x3E, 0x3E, 0x48, 0xFF);
	Surface_DrawOutline(MenuX + 0x5, MenuY + 0x5, Width - 0xA, Height - 0xA);
	Surface_DrawOutline(MenuX, MenuY, Width, Height);
	Surface_SetColor(0x25, 0x25, 0x25, 0xFF);
	Surface_DrawFilledRect(MenuX + 0x1, MenuY + 0x1, Width - 0x2, Height - 0x2);
	Surface_DrawFilledRect(MenuX + 0x6, MenuY + 0x6, Width - 0xC, Height - 0xC);
	Surface_SetColor(0x34, 0x34, 0x37, 0xFF);
	Surface_DrawFilledRect(MenuX + 0x6, MenuY + 0x6, Width - 0xC, 0x10);
	Surface_SetColor(0x20, 0x20, 0x27, 0xFF);
	Surface_DrawOutline(MenuX + 0x6, MenuY + 0x6, Width - 0xC, 0x10);
	Surface_SetTextColor(0xFF, 0xFF, 0xFF, 0xFF);
	Surface_GetTextSize(0x11, WindowName, &X, &Y);
	Surface_SetTextPosition(MenuX + (Width / 0x2) - (X / 0x2), MenuY + 0x6);
	Surface_DrawText(WindowName);
	ActiveX = MenuX + 0xA; ActiveY = MenuY + 0x1E;
}
VOID Menu_Checkbox(LPCWSTR Name, PBOOLEAN Byte) {
	Surface_SetColor(0x17, 0x17, 0x17, 0xFF);
	Surface_DrawOutline(ActiveX, ActiveY, 0xC, 0xC);
	Surface_SetColor(0x25, 0x25, 0x2A, 0xFF);
	Surface_DrawFilledRect(ActiveX + 0x1, ActiveY + 0x1, 0xA, 0xA);
	if (*Byte) {
		Surface_SetColor(0x1C, 0x1C, 0xCC, 0xFF);
		Surface_DrawFilledRect(ActiveX + 0x1, ActiveY + 0x1, 0xA, 0xA);
	}
	Surface_SetColor(0xFF, 0xFF, 0xFF, 0xFF);
	Surface_SetTextPosition(ActiveX + 0xF, ActiveY);
	Surface_DrawText(Name);
	if (Menu_IsClicked(ActiveX, ActiveY, 120, 12))
		*Byte ^= TRUE;
	ActiveY += 0xF;
}
VOID Features_Bhop(struct CUserCmd* Command) {
	if (!Config.bBunnyHop) return;
	PBYTE LocalPlayer = EntityList_GetEntity(Engine_GetLocalPlayer());
	if (CBaseEntity_Health(LocalPlayer) < 1)
		return;
	INT nMoveType = CBaseEntity_MoveType(LocalPlayer);
	if (nMoveType == 0x9 || nMoveType == 0x8 || nMoveType == 0x4 || nMoveType == 0xA)
		return;
	if (!(CBaseEntity_MoveFlags(LocalPlayer) & 0x1))
		Command->Buttons &= ~0x2;
}
VOID Features_AutoStrafe(struct CUserCmd* Command) {
	if (!Config.bBunnyHop) return;
	PBYTE LocalPlayer = EntityList_GetEntity(Engine_GetLocalPlayer());
	if (CBaseEntity_Health(LocalPlayer) < 1)
		return;
}
VOID(__fastcall* PaintTraverseOriginal)(LPVOID, PVOID, DWORD, BOOLEAN, BOOLEAN);
BOOLEAN(WINAPI* CreateMoveOriginal)(FLOAT, struct CUserCmd*);
BOOLEAN WINAPI _CreateMove(FLOAT flInputTime, struct CUserCmd* Command ) {
	BOOLEAN bSetAngles = CreateMoveOriginal(flInputTime, Command);
	Features_Bhop(Command);
	return bSetAngles;
}
VOID WINAPI _PaintTraverse(DWORD dwPanel, BOOLEAN bForceRepaint, BOOLEAN bAllowRepaint) { 
	if (StringFindString(Panel_GetPanelName(dwPanel), "MatSystemTopPanel")) {
		Menu_Window(L"SingleFile", 420, 260);
		Menu_Checkbox(L"Bunnyhop", &Config.bBunnyHop);
		Menu_Checkbox(L"Autostrafe", &Config.bAutoStrafe);
	}
	return PaintTraverseOriginal(Panel, 0, dwPanel, bForceRepaint, bAllowRepaint);
}
LPVOID CreateInterface(HANDLE hModule, LPCSTR szName) {
	LPVOID(*pfnCreateInterface)(LPCSTR, INT) = (LPVOID(*)(LPCSTR, INT))GetProcAddress(hModule, "CreateInterface");
	return pfnCreateInterface(szName, 0);
}
LRESULT CALLBACK WndProc(HWND hWindow, ULONG32 luMessage, WPARAM wParam, LPARAM lParam) {
	if (luMessage == WM_KEYDOWN && wParam == VK_INSERT)
		bMenuActive = !bMenuActive;
	bClicked = (BOOLEAN)(wParam == MK_LBUTTON);
	if (luMessage == WM_MOUSEMOVE) {
		Menu_RunInput(lParam);
		bInMove = TRUE;
	}
	else {
		bInMove = FALSE;
	} // thanks to es3n1n for this
	return CallWindowProcA(OriginalWndProc, hWindow, luMessage, wParam, lParam);
}
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		Window = FindWindowA("Valve001", NULL);
		OriginalWndProc = SetWindowLongA(Window, GWLP_WNDPROC, (ULONG32)WndProc);
		AllocConsole();
		SetConsoleTitleA("SingleFile");
		Surface = CreateInterface(GetModuleHandleA("vguimatsurface.dll"), "VGUI_Surface031");
		Panel = CreateInterface(GetModuleHandleA("vgui2.dll"), "VGUI_Panel009");
		Client = CreateInterface(GetModuleHandleA("client.dll"), "VClient018");
		EntityList = CreateInterface(GetModuleHandleA("client.dll"), "VClientEntityList003");
		Engine = CreateInterface(GetModuleHandleA("engine.dll"), "VEngineClient014");
		ClientMode = **(VOID***)((*(PDWORD*)(Client))[0xA] + 0x5);
		MenuX = 200; MenuY = 200;
		MH_Initialize();
		MH_CreateHook((*(PVOID**)(Panel))[41], &_PaintTraverse, (PVOID*)&PaintTraverseOriginal);
		MH_CreateHook((*(PVOID**)(ClientMode))[24], &_CreateMove, (PVOID*)&CreateMoveOriginal);
		MH_EnableHook(NULL);
		WriteConsoleA(GetStdHandle((ULONG32)-11), "SingleFile v2.0 Alpha Loaded\n", 29, NULL, NULL);
	}
	return TRUE;
}