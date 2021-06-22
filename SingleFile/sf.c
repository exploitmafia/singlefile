#include <windows.h>
LPVOID Engine, Surface, EntityList, Panel, Client, ClientMode, Events, ConsoleVars, Sound; WNDPROC OriginalWndProc; HANDLE Window; 
INT WINAPI MH_Initialize(VOID);
INT WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
INT WINAPI MH_EnableHook(LPVOID pTarget);
#define GET_BITS(x)        ((x >= '0' && x <= '9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xA))
#define GET_BYTE(x)        (GET_BITS(x[0x0]) << 0x4 | GET_BITS(x[0x1]))
PBYTE PatternScan(HMODULE hModule, LPCSTR _szPattern) {
	LPCSTR szPattern = _szPattern;  PBYTE Match; PIMAGE_NT_HEADERS Header = (PIMAGE_NT_HEADERS)(hModule + 0x3C); // Skip casting to DOS header, we know the LONG pointer to the NT header @ e_lfanew is Image + 0x3C
	for (PBYTE pCurrent = (PBYTE)hModule; pCurrent < (PBYTE)(hModule + Header->OptionalHeader.SizeOfCode); pCurrent++) {
		if (*(PBYTE)szPattern == '?' || *(PBYTE)pCurrent == GET_BYTE(szPattern)) {
			if (!(*(szPattern))) // End of string
				return Match;
			if (!Match)
				Match = pCurrent;
			szPattern += 0x2;
		} else {
			if (Match)
				pCurrent = Match;
			szPattern = _szPattern;
			Match = NULL;
		}
	}
	return NULL;
}
/*CSTD lib*/ ULONG32 WideStringLength(LPCWSTR StringPointer) { ULONG32 Length = 0; while (StringPointer[Length]) Length++; return Length; }; LPSTR StringFindChar(LPCSTR String, CHAR _Character) { CONST CHAR Character = _Character; for (; String[0] != Character; ++String) { if (!String[0]) return NULL; } return String; }; LPSTR StringFindString(LPCSTR Source, LPSTR String) { if (!String[0]) return (LPSTR)Source; for (; (Source = StringFindChar(Source, String[0])) != NULL; ++Source) { LPCSTR TMPV1, TMPV2; for (TMPV1 = Source, TMPV2 = String;;) { if ((++TMPV2)[0]) return (LPSTR)Source; else if ((++TMPV1)[0] != TMPV2[0]) break; }} return NULL; }; ULONG32 StringLength(LPSTR String) { ULONG32 Iterator = 0; while (String[Iterator]) Iterator++; return Iterator; }; // hehe
#define METHOD(RType, Type, Name, RawArgs, Interface, Index, ...) __forceinline RType Interface##_##Name RawArgs {return (((Type)(((PULONG32*)(Interface))[0][Index]))(Interface, 0, __VA_ARGS__));} // Ensure EDX is NULL for __thiscall emulation
#define NMETHOD(RType, Type, Name, RawArgs, Interface, Index) __forceinline RType Interface##_##Name RawArgs {return (((Type)(((PULONG32*)(Interface))[0][Index]))(Interface, 0));} // Ensure EDX is NULL for __thiscall emulation
#define VMETHOD(RType, Type, EName, Name, RawArgs, Index) __forceinline RType EName##_##Name RawArgs {return (((Type)(((PULONG32*)(Interface))[0][Index]))(Interface, 0));} // Ensure EDX is NULL for __thiscall emulation
#define VAMETHOD(RType, Type, EName, Name, RawArgs, Index, ...) __forceinline RType EName##_##Name RawArgs {return (((Type)(((PULONG32*)(Interface))[0][Index]))(Interface, 0, __VA_ARGS__));} // Ensure EDX is NULL for __thiscall emulation
#define OFFSET(RType, IName, Name, Offset) __forceinline RType IName##_##Name (PBYTE Entity) {return *(RType*)(Entity + Offset); };
struct TAGConfig {
	BOOLEAN bBunnyHop, bAutoStrafe; // Movement
	BOOLEAN bAutoPistol, bHitSound, bHitEffect; // Aimbot and related
	BOOLEAN bAutoAccept, bVoteRevealer, bAntiPopup; // Game UI-related things
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
	ULONG32 : 24; ULONG64 : 64; // Padding, 88-bits
	WORD MouseDeltaX, MouseDeltaY;
};
struct CGlobalVarsClientBase {
	ULONG64 : 64; ULONG64 : 64; // Padding, 128-bits
	FLOAT m_flCurrentTime;
}*Globals;
struct CEnginePlayerInformation {
	ULONG64 : 64; ULONG64 : 64; // Pading, 128-bits
	CHAR szPlayerName[128];
	INT nUserID;
}*PlayerInfo; // Constant pointer, to avoid redefinition.
METHOD(LPCSTR, LPCSTR(__fastcall*)(LPVOID, PVOID, ULONG32), GetPanelName, (ULONG32 luPanelID), Panel, 36, luPanelID); // VGUIPanel Block
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, WORD, WORD, WORD, WORD), SetColor, (WORD r, WORD g, WORD b, WORD a), Surface, 15, r, g, b, a); // CMatSystemSurface Block
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, INT, INT, INT, INT), DrawFilledRect, (INT x, INT y, INT w, INT h), Surface, 16, x, y, x + w, y + h);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, WORD, WORD, WORD, WORD), SetTextColor, (WORD r, WORD g, WORD b, WORD a), Surface, 25, r, g, b, a);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, ULONG32), SetTextPosition, (ULONG32 x, ULONG32 y), Surface, 26, x, y);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, LPCWSTR, ULONG32), DrawText, (LPCWSTR StringPointer), Surface, 28, StringPointer, WideStringLength(StringPointer));
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32), SetTextFont, (ULONG32 Font), Surface, 23, Font);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, ULONG32, ULONG32, ULONG32), DrawOutline, (ULONG32 x, ULONG32 y, ULONG32 w, ULONG32 h), Surface, 18, x, y, x + w, y + h);
METHOD(VOID, VOID(__fastcall*)(LPVOID, PVOID, ULONG32, LPCWSTR, PULONG32, PULONG32), GetTextSize, (ULONG32 Font, LPCWSTR StringPointer, PULONG32 X, PULONG32 Y), Surface, 79, Font, StringPointer, X, Y);
NMETHOD(INT, INT(__fastcall*)(LPVOID, PVOID), GetLocalPlayer, (VOID), Engine, 12); // IVEngineClient Block
METHOD(BOOLEAN, BOOLEAN(__fastcall*)(LPVOID, PVOID, INT, struct CEnginePlayerInformation*), GetPlayerInfo, (INT nIndex, struct CEnginePlayerInformation* pPlayer), Engine, 8, nIndex, pPlayer);
METHOD(VOID, VOID(__fastcall*)(LPVOID, LPVOID, LPCSTR, BOOLEAN), ClientCommand, (LPCSTR szCommand), Engine, 114, szCommand, FALSE);
METHOD(PVOID, PVOID(__fastcall*)(LPVOID, PVOID, ULONG32), GetEntity, (ULONG32 luIndex), EntityList, 3, luIndex); // CBaseClientEntityList Block
OFFSET(INT, CBaseEntity, Health, 0x100); // CBaseEntity Block
OFFSET(INT, CBaseEntity, MoveType, 0x25C);
OFFSET(INT, CBaseEntity, MoveFlags, 0x104);
OFFSET(PFLOAT, CBaseEntity, HealthShotTime, 0x3AAC); // this offset is very likely wrong, however I do not have CS:GO installed at the moment, new laptop :p
OFFSET(FLOAT, CBaseCombatWeapon, NextAttack, 0x3238);
VMETHOD(PVOID, PVOID(__fastcall*)(LPVOID, PVOID), CBaseEntity, GetWeapon, (PVOID Interface), 267);
VMETHOD(INT, INT(__fastcall*)(LPVOID, PVOID), CBaseCombatWeapon, GetWeaponType, (PVOID Interface), 454); // CBaseCombatWeapon Block
VMETHOD(LPCSTR, LPCSTR(__fastcall*)(LPVOID, PVOID), IGameEvent, GetEventName, (PVOID Interface), 1); // IGameEvent Block
VAMETHOD(INT, INT(__fastcall*)(LPVOID, PVOID, LPCSTR), IGameEvent, GetInteger, (PVOID Interface, LPCSTR szKeyName), 6, szKeyName);
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
	if (CBaseEntity_Health(LocalPlayer) < 1) return;
	if (!(CBaseEntity_MoveFlags(LocalPlayer) & 1) && CBaseEntity_MoveType(LocalPlayer) != 0x8) {
		if (Command->MouseDeltaX > 0)
			Command->Side = 450.0f;
		else if (Command->MouseDeltaX < 0)
			Command->Side = -450.0f;
	}
}
VOID Features_AutoPistol(struct CUserCmd* Command) {
	if (!Config.bAutoPistol) return;
	PBYTE LocalPlayer = EntityList_GetEntity(Engine_GetLocalPlayer());
	if (CBaseEntity_Health(LocalPlayer) < 1) return;
	PBYTE ActiveWeapon = CBaseEntity_GetWeapon(LocalPlayer);
	if (CBaseCombatWeapon_GetWeaponType(ActiveWeapon) != 0x1) return;
	if (CBaseCombatWeapon_NextAttack(ActiveWeapon) > Globals->m_flCurrentTime)
		Command->Buttons &= ~(1 << 0); // IN_ATTACK
}
VOID Features_HitEffects(PVOID Event) {
	if (StringFindString(IGameEvent_GetEventName(Event), "player_hurt") ) {
		Engine_GetPlayerInfo(Engine_GetLocalPlayer(), PlayerInfo);
		if (IGameEvent_GetInteger(Event, "attacker") == PlayerInfo->nUserID) {
			if (Config.bHitEffect) *(CBaseEntity_HealthShotTime(EntityList_GetEntity(Engine_GetLocalPlayer()))) = Globals->m_flCurrentTime + 1.0f;
			if (Config.bHitSound) Engine_ClientCommand("play buttons/arena_switch_press_02");
		}
	}
}
VOID(__fastcall* PaintTraverseOriginal)(LPVOID, PVOID, DWORD, BOOLEAN, BOOLEAN);
BOOLEAN(WINAPI* CreateMoveOriginal)(FLOAT, struct CUserCmd*);
BOOLEAN(__fastcall* GameEventsOriginal)(LPVOID, PVOID, PVOID);
VOID(WINAPI* EmitSoundOriginal)(PVOID, INT, INT, LPCSTR, DWORD, LPCSTR, FLOAT, INT, INT, INT, INT, struct vec3, struct vec3, PVOID, BOOLEAN, FLOAT, INT, PVOID);
VOID WINAPI _EmitSound(PVOID pFilter, INT nEntityIndex, INT nChannel, LPCSTR szSoundEntry, DWORD dwSoundEntryHash, LPCSTR szSample, FLOAT flVolume, INT nSeed, INT nSoundLevel, INT nFlags, INT nPitch, struct vec3 Origin, struct vec3 Direction, PVOID vecOrigins, BOOLEAN bUpdatePosition, FLOAT flAudioLength, INT nSpeakingEntity, PVOID pUnknown) {
	if (StringFindString(szSoundEntry, "UIPanorama.popup_accept_match_beep") && Config.bAutoAccept)
		((BOOLEAN(WINAPI*)(LPCSTR))PatternScan(GetModuleHandleA("client.dll"), "55 8B EC 83 E4 F8 8B 4D 08 BA ? ? ? ? E8 ? ? ? ? 85 C0 75 12"))("");
	return EmitSoundOriginal(pFilter, nEntityIndex, nChannel, szSoundEntry, dwSoundEntryHash, szSample, flVolume, nSeed, nSoundLevel, nFlags, nPitch, Origin, Direction, vecOrigins, bUpdatePosition, flAudioLength, nSpeakingEntity, pUnknown);
}
BOOLEAN WINAPI _GameEvents(PVOID Event) {
	Features_HitEffects(Event);
	return GameEventsOriginal(Events, 0, Event);
}
BOOLEAN WINAPI _CreateMove(FLOAT flInputTime, struct CUserCmd* Command ) {
	BOOLEAN bSetAngles = CreateMoveOriginal(flInputTime, Command);
	Features_Bhop(Command);
	Features_AutoStrafe(Command);
	Features_AutoPistol(Command);
	return bSetAngles;
}
VOID WINAPI _PaintTraverse(DWORD dwPanel, BOOLEAN bForceRepaint, BOOLEAN bAllowRepaint) { 
	if (StringFindString(Panel_GetPanelName(dwPanel), "MatSystemTopPanel")) {
		if (bMenuActive) {
			Menu_Window(L"SingleFile", 420, 260);
			Menu_Checkbox(L"Bunnyhop", &Config.bBunnyHop);
			Menu_Checkbox(L"Autostrafe", &Config.bAutoStrafe);
			Menu_Checkbox(L"Autopistol", &Config.bAutoPistol);
			Menu_Checkbox(L"Hit Effect", &Config.bHitEffect);
			Menu_Checkbox(L"Hit Sound", &Config.bHitSound);
		}
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
		Events = CreateInterface(GetModuleHandleA("engine.dll"), "GAMEEVENTSMANANGER002");
		ClientMode = **(VOID***)((*(PDWORD*)(Client))[0xA] + 0x5);
		Globals = **(struct CGlobalVarsClientBase***)((*(PDWORD*)(Client))[0xB] + 0xA);
		MenuX = 200; MenuY = 200;
		MH_Initialize();
		MH_CreateHook((*(PVOID**)(Panel))[41], &_PaintTraverse, (PVOID*)&PaintTraverseOriginal);
		MH_CreateHook((*(PVOID**)(ClientMode))[24], &_CreateMove, (PVOID*)&CreateMoveOriginal);
		MH_CreateHook((*(PVOID**)(Events))[9], &_GameEvents, (PVOID*)&GameEventsOriginal);
		MH_EnableHook(NULL);
		WriteConsoleA(GetStdHandle((ULONG32)-11), "SingleFile v2.0 Alpha Loaded\n", 29, NULL, NULL);
	}
	return TRUE;
}