#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <unordered_map>
#include <string>
#pragma comment(lib, "minhook")
#define IN_RANGE(x,a,b)        (x >= a && x <= b) 
#define GET_BITS( x )        (IN_RANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xA))
#define GET_BYTE( x )        (GET_BITS(x[0x0]) << 0x4 | GET_BITS(x[0x1]))
PVOID client_dll = NULL;  PVOID engine_dll = NULL; HMODULE pModule = NULL; DWORD dwModuleSize;
INT CCSPlayer = 0x28; // ClassID::CCSPlayer = 40;
typedef enum MH_STATUS {
	MH_UNKNOWN = -1, MH_OK = 0, MH_ERROR_ALREADY_INITIALIZED, MH_ERROR_NOT_INITIALIZED, MH_ERROR_ALREADY_CREATED, MH_ERROR_NOT_CREATED, MH_ERROR_ENABLED, MH_ERROR_DISABLED, MH_ERROR_NOT_EXECUTABLE, MH_ERROR_UNSUPPORTED_FUNCTION, MH_ERROR_MEMORY_ALLOC, MH_ERROR_MEMORY_PROTECT, MH_ERROR_MODULE_NOT_FOUND, MH_ERROR_FUNCTION_NOT_FOUND
}
MH_STATUS; // get minhook here: https://github.com/TsudaKageyu/minhook | License for minhook (text of license has not been modified, just newlines removed) : /* MinHook - The Minimalistic API Hooking Library for x64 / x86 * Copyright(C) 2009 - 2017 Tsuda Kageyu. * All rights reserved. * *Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met : * *1. Redistributions of source code must retain the above copyright * notice, this list of conditionsand the following disclaimer. * 2. Redistributions in binary form must reproduce the above copyright * notice, this list of conditionsand the following disclaimer in the * documentationand /or other materials provided with the distribution. * *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A * PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, *PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. * /
extern "C" {
	MH_STATUS WINAPI MH_Initialize(VOID);
	MH_STATUS WINAPI MH_Uninitialize(VOID);
	MH_STATUS WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID ppOriginal);
	MH_STATUS WINAPI MH_RemoveHook(LPVOID pTarget);
	MH_STATUS WINAPI MH_EnableHook(LPVOID pTarget);
	MH_STATUS WINAPI MH_DisableHook(LPVOID pTarget);
}
PBYTE PatternScan(PVOID m_pModule, LPCSTR m_szSignature) {
	LPCSTR pat = m_szSignature;
	PBYTE first_match = 0x0;
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)m_pModule;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((PBYTE)m_pModule + dos->e_lfanew);
	for (PBYTE current = (PBYTE)m_pModule; current < (PBYTE)m_pModule + nt->OptionalHeader.SizeOfCode; current++) {
		if (*(PBYTE)pat == '\?' || *(PBYTE)current == GET_BYTE(pat)) {
			if (!*pat)
				return first_match;
			if (!first_match)
				first_match = current;
			if (!pat[0x2])
				return first_match;
			pat += (*(PUSHORT)pat == (USHORT)'\?\?' || *(PBYTE)pat != (BYTE)'\?') ? 0x3 : 0x2;
		} else {
			if (first_match != 0x0)
				current = first_match;
			pat = m_szSignature;
			first_match = 0x0;
		}
	}
	return NULL;
}
#undef DrawText
#undef CreateFont
template <typename I, std::size_t Idx, typename ...Args>
__forceinline I v(PVOID iface, Args... args) { return (*(I(__thiscall***)(void*, Args...))(iface))[Idx](iface, args...); }
#define VIRTUAL_METHOD(returnType, name, idx, args, argsRaw) __forceinline returnType name args { return v<returnType, idx>argsRaw; }
#define OFFSET(type, name, offset) __forceinline type name(VOID) { return *(type*)(this + offset); }
#define ROFFSET(type, name, offset) __forceinline type& name(VOID) { return *(type*)(this + offset);} // not sure if there's a better way to do this but whatever
using matrix_t = FLOAT[3][4];
using matrix4x4_t = FLOAT[4][4];
// config system
BOOLEAN menu_open = TRUE;
struct sconfig {
	struct saim {
		BOOLEAN m_bTriggerbot;
		BOOLEAN m_bAutoPistol;
		INT     m_nTriggerKey = VK_MBUTTON;
		BOOLEAN m_bTriggerKey;
	}aimbot;
	struct svisuals {
		BOOLEAN m_bBoxESP;
		BOOLEAN m_bNameESP;
		BOOLEAN m_bHealthBar;
		BOOLEAN m_bTargetTeam;
		BOOLEAN m_bDormanyCheck;
		BOOLEAN m_bOnlyOnDead;
		BOOLEAN m_bRadar;
		BOOLEAN m_bDisablePostProcess;
		BOOLEAN m_bRankRevealer;
		BOOLEAN m_bFlashReducer;
	}visuals;
	struct smisc {
		BOOLEAN m_bBhop;
		BOOLEAN m_bHitSound;
		BOOLEAN m_bNoScopeCrosshair;
		BOOLEAN m_bRecoilCrosshair;
		BOOLEAN m_bAutoAccept;
		BOOLEAN m_bGameKeyboard;
		BOOLEAN m_bSpectatorList;
		BOOLEAN m_bUseSpam;
		BOOLEAN m_bVoteRevealer;
	}misc;
}config;
class vec3 {
public:
	FLOAT x, y, z;
	vec3(FLOAT a = 0, FLOAT b = 0, FLOAT c = 0) {
		this->x = a;
		this->y = b;
		this->z = c;
	}
	vec3 operator-=(const vec3& in) { x -= in.x; y -= in.y; z -= in.z; return *this; }
	vec3 operator+=(const vec3& in) { x += in.x; y += in.y; z += in.z; return *this; }
	vec3 operator/=(const vec3& in) { x /= in.x; y /= in.y; z /= in.z; return *this; }
	vec3 operator*=(const vec3& in) { x *= in.x; y *= in.y; z *= in.z; return *this; }
	vec3 operator+(const vec3& in) { return vec3(x + in.x, y + in.y, z + in.z); }
	vec3 operator-(const vec3& in) { return vec3(x - in.x, y - in.y, z - in.z); }
	vec3 operator*(const vec3& in) { return vec3(x * in.x, y * in.y, z * in.z); }
	vec3 operator/(const vec3& in) { return vec3(x / in.x, y / in.y, z / in.z); }
	FLOAT dot(FLOAT* a) { return x * a[0] + y * a[1] + z * a[2]; }
	VOID clear() { x = y = z = 0.f; }
};
struct SPlayerInfo {
	ULONG64 m_ullVersion;
	union {
		ULONG64 m_ullXUID;
		struct {
			DWORD m_nXUIDLow;
			DWORD m_nXUIDHigh;
		};
	};
	CHAR m_szName[128];
	INT m_nUserID;
	CHAR m_szGUID[33];
	DWORD m_nFriendsID;
	CHAR m_szFriendsName[128];
	BOOLEAN m_bIsBot;
	BOOLEAN m_bIsHLTV;
	INT m_nCustomFiles[4];
	BYTE m_ucFilesDownloaded;
};
class CMatSystemSurface {
public:
	VIRTUAL_METHOD(VOID, DrawFilledRect, 16, (DWORD x, DWORD y, DWORD w, DWORD h), (this, x, y, x + w, y + h))
	VIRTUAL_METHOD(VOID, SetColor, 15, (USHORT r, USHORT g, USHORT b, USHORT a), (this, r, g, b, a))
	VIRTUAL_METHOD(VOID, SetTextColor, 25, (USHORT r, USHORT g, USHORT b, USHORT a), (this, r, g, b, a))
	VIRTUAL_METHOD(VOID, SetTextPosition, 26, (DWORD x, DWORD y), (this, x, y))
	VIRTUAL_METHOD(VOID, DrawText, 28, (LPCWSTR text, DWORD len), (this, text, len, 0))
	VIRTUAL_METHOD(DWORD, CreateFont, 71, (VOID), (this))
	VIRTUAL_METHOD(BOOLEAN, SetFontGlyphs, 72, (DWORD _font, LPCSTR name, DWORD height, DWORD weight, DWORD font_flags), (this, _font, name, height, weight, 0, 0, font_flags, 0, 0))
	VIRTUAL_METHOD(VOID, SetTextFont, 23, (DWORD _font), (this, _font))
	VIRTUAL_METHOD(VOID, DrawRectOutline, 18, (DWORD x, DWORD y, DWORD w, DWORD h), (this, x, y, x + w, y + h))
	VIRTUAL_METHOD(VOID, GetTextSize, 79, (DWORD _font, LPCWSTR text, DWORD& w, DWORD& h), (this, _font, text, std::ref(w), std::ref(h)))
};
enum EMoveType {
	NONE = 0,
	FLY = 4,
	NOCLIP = 8,
	LADDER = 9,
	OBSERVER = 10
};
enum EPlayerFlags {
	ONGROUND = 1,
	DUCKING = 2
};
class CCSClientClass {
public:
	PVOID CreateClassFn;
	PVOID CreateEventFn;
	char* m_szNetworkedName;
	PVOID m_pRecvTable;
	CCSClientClass* m_pNextClass;
	INT m_nClassID;
};
class CBaseEntity {
public:
	__forceinline PVOID GetNetworkable() {
		return (PVOID)(this + 0x8);
	}
	VIRTUAL_METHOD(CCSClientClass*, GetClientClass, 2, (VOID), (this->GetNetworkable()));
	OFFSET(vec3, CollisonMins, 0x328); // CBaseEntity::m_Collison::m_vecMins
	OFFSET(vec3, CollisonMaxs, 0x334);
	VIRTUAL_METHOD(vec3&, GetAbsOrigin, 10, (VOID), (this));
	OFFSET(vec3, GetViewOffset, 0x108); // CBaseEntity::localdata::m_vecViewOffset
	__forceinline vec3 GetEyePosition(VOID) {
		return (this->GetAbsOrigin() + this->GetViewOffset());
	}
	OFFSET(BOOLEAN, IsDormant, 0xED);
	OFFSET(BOOLEAN, IsInImmunity, 0x3944);// CCSPlayer::m_bHasGunGameImmunity
	OFFSET(INT, GetFlags, 0x104);
	OFFSET(INT, MoveType, 0x25C);
	OFFSET(INT, GetHealth, 0x100);
	VIRTUAL_METHOD(CBaseEntity*, GetWeapon, 267, (VOID), (this));
	VIRTUAL_METHOD(INT, GetWeaponType, 454, (VOID), (this));
	OFFSET(FLOAT, WeaponNextAttack, 0x3238);
	ROFFSET(matrix_t, GetCoordinateFrame, 0x444);
	OFFSET(INT, GetTeamNumber, 0xF4);
	VIRTUAL_METHOD(CBaseEntity*, GetObserverTarget, 294, (VOID), (this))
	OFFSET(BOOLEAN, IsScoped, 0x3928);
	ROFFSET(BOOLEAN, Spotted, 0x93D);
	OFFSET(FLOAT, FlashDuration, 0xA420);
	ROFFSET(FLOAT, FlashMaxAlpha, 0xA41C)
	OFFSET(INT, Ammo, 0x3264);
	OFFSET(INT, CrosshairTarget, 0xB3E4);
};
class CGlobalVarsBase {
public:
	FLOAT			m_flRealTime;
	INT				m_nFrameCount;
	FLOAT			m_flAbsFrameTime;
	FLOAT			m_flAbsFrameStart;
	FLOAT			m_flCurrentTime;
	FLOAT			m_flFrameTime;
	INT				m_nMaxClients;
	INT				m_nTickCount;
	FLOAT			m_flTickInterval;
	FLOAT			m_flInteropolationAmount;
	INT				m_nTicksThisFrmae;
	INT          	m_nNetworkProtocol;
	PVOID			m_pGameSaveData;
	BOOLEAN			m_bClient;
	BOOLEAN			m_bRemoteClient;
private:
	DWORD	unk1;
	DWORD	unk2;
};
template <typename T>
T RelativeToAbsolute(DWORD m_pAddress) {
	return (T)(m_pAddress + 0x4 + *(int*)(m_pAddress));
}
class CBaseEntityList {
public:
	VIRTUAL_METHOD(CBaseEntity*, GetEntity, 3, (INT index), (this, index));
};
class IVEngineClient {
public:
	VIRTUAL_METHOD(INT, GetMaxClients, 20, (VOID), (this));
	VIRTUAL_METHOD(VOID, GetScreenSize, 5, (DWORD& w, DWORD& h), (this, std::ref(w), std::ref(h)));
	VIRTUAL_METHOD(BOOLEAN, GetPlayerInfo, 8, (INT idx, SPlayerInfo* info), (this, idx, info));
	VIRTUAL_METHOD(DWORD, GetLocalPlayer, 12, (VOID), (this));
	VIRTUAL_METHOD(BOOLEAN, IsInGame, 26, (VOID), (this));
	VIRTUAL_METHOD(matrix4x4_t&, GetViewMatrix, 37, (VOID), (this));
	VIRTUAL_METHOD(VOID, ClientCmdUnrestricted, 114, (LPCSTR szCommand), (this, szCommand, FALSE));
	VIRTUAL_METHOD(LPCSTR, GetVersionString, 105, (VOID), (this));
	VIRTUAL_METHOD(INT, GetPlayerIndex, 9, (INT nIndex), (this, nIndex));
};
class IGameEvent {
public:
	VIRTUAL_METHOD(LPCSTR, GetName, 1, (VOID), (this));
	VIRTUAL_METHOD(BOOLEAN, GetBool, 5, (LPCSTR keyName), (this, keyName, FALSE));
	VIRTUAL_METHOD(INT, GetInt, 6, (LPCSTR keyName), (this, keyName, 0));
};
class IPanel {
public:
	VIRTUAL_METHOD(VOID, SetInputKeyboardState, 31, (DWORD PanelID, BOOLEAN State), (this, PanelID, State));
	VIRTUAL_METHOD(VOID, SetInputMouseState, 32, (DWORD PanelID, BOOLEAN State), (this, PanelID, State));
	VIRTUAL_METHOD(LPCSTR, GetPanelName, 36, (DWORD PanelID), (this, PanelID));
};
class CConvar {
public:
	VIRTUAL_METHOD(FLOAT, GetFloat, 12, (VOID), (this));
	VIRTUAL_METHOD(INT, GetInt, 13, (VOID), (this));
	VIRTUAL_METHOD(VOID, SetValue, 14, (LPCSTR value), (this, value));
	VIRTUAL_METHOD(VOID, SetValue, 15, (FLOAT value), (this, value));
	VIRTUAL_METHOD(VOID, SetValue, 16, (INT value), (this, value));
};
class ICVar {
public:
	VIRTUAL_METHOD(CConvar*, FindVar, 15, (LPCSTR name), (this, name));
};
class CRecvProp;
class CClientClass {
public:
	PVOID			m_pCreateFunction;
	PVOID			m_pCreateEventFunction;
	char*			m_szNetworkName;
	CRecvProp*      m_pRecvPointer;
	CClientClass*	m_pNextPointer;
	int				m_nClassID;
};
class IClient {
public:
	VIRTUAL_METHOD(CClientClass*, GetClientClasses, 8, (VOID), (this))
	VIRTUAL_METHOD(BOOLEAN, DispatchUserMessage, 38, (INT m_nMessageType, INT m_nArgument1, INT m_nArgument2, PVOID m_pData), (this, m_nMessageType, m_nArgument1, m_nArgument2, m_pData))
};
class IClientModeShared;
class IGameEventManager2;
class ISound;
struct sinterfaces {
	IVEngineClient* engine = nullptr;
	CMatSystemSurface* surface = nullptr;
	CBaseEntityList* entitylist = nullptr;
	IPanel* panel = nullptr;
	IClient* client = nullptr;
	IClientModeShared* client_mode = nullptr;
	IGameEventManager2* events = nullptr;
	CGlobalVarsBase* globals = nullptr;
	ICVar* cvar = nullptr;
	ISound* sound = nullptr;
}interfaces;
HWND csgo_window;
WNDPROC orig_proc;
struct vec2 {
	INT x, y;
	vec2(INT x = 0, INT y = 0) {
		this->x = x;
		this->y = y;
	}
};
VOID load(LPCSTR szConfigName) {
	FILE* cfg = fopen(szConfigName, "r");
	fread(&config, sizeof(config), 1, cfg);
	fclose(cfg);
}
VOID save(LPCSTR szConfigName) {
	FILE* cfg = fopen(szConfigName, "w");
	fwrite(&config, sizeof(config), 1, cfg);
	fclose(cfg);
}
namespace menu {
	struct sctx { BOOLEAN open; INT width; };
	std::unordered_map < LPCWSTR, BOOLEAN> item_clicks = {};
	std::unordered_map<PINT, sctx> Keys = {}; // I hate these stl containers but whatever its convienent for line count & index by pointer location rather than a custom id, this assumes you won't be having two keybinders with the same value
	DWORD font, esp;
	vec2 start_pos, size;
	BOOLEAN dragging = FALSE, clicked = FALSE, item_active = FALSE, inmove = FALSE;
	INT x_pos = 0, y_pos = 0, last_mouse_x = 0, last_mouse_y = 0;
	BOOLEAN in_region( INT x, INT y, INT w, INT h ) {
		return last_mouse_x >= x && last_mouse_y >= y && last_mouse_x <= x + w && last_mouse_y <= y + h;
	}
	BOOLEAN clicked_at( LPCWSTR n, INT x, INT y, INT w, INT h ) {
		if (item_clicks.count(n) == 0) item_clicks[n] = FALSE;
		if (!in_region(x, y, w, h) && !item_clicks[n] || inmove) return FALSE;
		item_active = TRUE;
		if (clicked) {
			BOOLEAN ret = !item_clicks[n];
			item_clicks[n] = TRUE;
			return ret;
		}
		item_clicks[n] = FALSE;
		return FALSE;
	}
	VOID window(LPCWSTR name) {
		item_active = FALSE;
		interfaces.surface->SetTextFont(menu::font);
		interfaces.surface->SetColor(23, 23, 30, 255);
		interfaces.surface->DrawRectOutline(start_pos.x - 1, start_pos.y - 1, size.x + 2, size.y + 2);
		interfaces.surface->SetColor(62, 62, 72, 255);
		interfaces.surface->DrawRectOutline(start_pos.x, start_pos.y, size.x, size.y);
		interfaces.surface->SetColor(37, 37, 37, 255);
		interfaces.surface->DrawFilledRect(start_pos.x + 1, start_pos.y + 1, size.x - 2, size.y - 2);
		interfaces.surface->SetColor(62, 62, 72, 255);
		interfaces.surface->DrawRectOutline(start_pos.x + 5, start_pos.y + 5, size.x - 10, size.y - 10);
		interfaces.surface->SetColor(30, 30, 30, 255);
		interfaces.surface->DrawFilledRect(start_pos.x + 6, start_pos.y + 6, size.x - 12, size.y - 12);
		interfaces.surface->SetColor(45, 45, 48, 255);
		interfaces.surface->DrawFilledRect(start_pos.x + 6, start_pos.y + 6, size.x - 12, 14);
		interfaces.surface->SetColor(60, 60, 70, 255);
		interfaces.surface->DrawFilledRect(start_pos.x + 6, start_pos.y + 20, size.x - 12, 1);
		interfaces.surface->SetTextColor(255, 255, 255, 255);
		static DWORD u, i;
		interfaces.surface->GetTextSize(menu::font, name, u, i);
		interfaces.surface->SetTextPosition( start_pos.x + (size.x / 2) - (u / 2), start_pos.y + 6);
		interfaces.surface->DrawText(name, wcslen(name));
		x_pos = start_pos.x + 10;
		y_pos = start_pos.y + 25;
	}
	VOID column(INT x_offset) {
		x_pos += x_offset + 20;
		y_pos = start_pos.y + 25;
		interfaces.surface->SetColor(17, 17, 17, 255);
		interfaces.surface->DrawFilledRect(x_pos - 5, start_pos.y + 26, 1, size.y - 60 + 24);
	}
	VOID checkbox(LPCWSTR name, BOOLEAN* option) {
		interfaces.surface->SetColor(27, 27, 27, 255);
		interfaces.surface->DrawRectOutline(x_pos, y_pos, 12, 12);
		interfaces.surface->SetColor(37, 37, 38, 255);
		interfaces.surface->DrawFilledRect(x_pos + 1, y_pos + 1, 10, 10);
		if (*option) {
			interfaces.surface->SetColor(25, 100, 255, 255);
			interfaces.surface->DrawFilledRect(x_pos + 1, y_pos + 1, 10, 10);
		}
		interfaces.surface->SetTextColor(255, 255, 255, 255);
		interfaces.surface->SetTextPosition(x_pos + 15, y_pos);
		interfaces.surface->DrawText(name, wcslen(name));
		if (clicked_at(name, x_pos, y_pos, 12, 12))
			*option = !(*option);
		y_pos += 15;
	}
	BOOLEAN button(LPCWSTR name, vec2 pos, vec2 size) {
		interfaces.surface->SetColor(15, 15, 15, 255);
		interfaces.surface->DrawRectOutline(pos.x, pos.y, size.x, size.y);
		interfaces.surface->SetColor(27, 27, 27, 255);
		interfaces.surface->DrawRectOutline(pos.x + 1, pos.y + 1, size.x - 2, size.y - 2);
		interfaces.surface->SetColor(37, 37, 37, 255);
		interfaces.surface->DrawFilledRect(pos.x + 2, pos.y + 2, size.x - 4, size.y - 4);
		interfaces.surface->SetTextColor(255, 255, 255, 255);
		DWORD u, i;
		interfaces.surface->GetTextSize(menu::font, name, u, i);
		interfaces.surface->SetTextPosition(pos.x + (size.x / 2) - u / 2, pos.y + (size.y / 2) - i / 2);
		interfaces.surface->DrawText(name, wcslen(name));
		if (clicked_at(name, pos.x, pos.y, size.x, size.y))
			return TRUE;
		return FALSE;
	}
	VOID move(INT x, INT y) {
		auto store = [x, y] () -> VOID { menu::last_mouse_x = x; menu::last_mouse_y = y; };
		if (!clicked) {
			menu::dragging = FALSE;
			return store();
		}
		if (in_region(start_pos.x, start_pos.y, size.x, 20) && !item_active)
			menu::dragging = TRUE;
		if (menu::dragging) {
			start_pos.x += x - menu::last_mouse_x;
			start_pos.y += y - menu::last_mouse_y;
		}
		return store();
	}
	LPCWSTR pwszVirtualKeys[] = { L"?", L"M1", L"M2", L"BREAK", L"M3", L"M4", L"M5", L"?", L"BACK", L"TAB", L"?", L"?", L"?", L"ENTER", L"?", L"?", L"SHIFT", L"CTRL", L"ALT", L"PAUSE", L"CAPS", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"SPACE", L"PGUP", L"PGDN", L"END", L"HOME", L"LEFT", L"UP", L"RIGHT", L"DOWN", L"?", L"SYSRQ", L"?", L"PRNSCR", L"NONE", L"DEL", L"?", L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L", L"M", L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z", L"WIN", L"RWIN", L"?", L"?", L"?", L"NUM 0", L"NUM 1", L"NUM 2", L"NUM 3", L"NUM 4", L"NUM 5", L"NUM 6", L"NUM 7", L"NUM 8", L"NUM 9", L"*", L"+", L"_", L"-", L".", L"/", L"F1", L"F2", L"F3", L"F4", L"F5", L"F6", L"F7", L"F8", L"F9", L"F10", L"F11", L"F12", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", L"?", }; // mhhh i'll fill in the rest later
	VOID keybinder(PINT pKey) { // Pints for the low hello Dex.
		INT x = x_pos + 180 - Keys[pKey].width;
		INT y = y_pos - 15;
		LPCWSTR  wszKeyName = pwszVirtualKeys[*pKey];
		DWORD w, xh, h = 20;
		interfaces.surface->GetTextSize(menu::font, wszKeyName, w, xh);
		Keys[pKey].width = w + 16;
		interfaces.surface->SetColor(17, 17, 17, 255);
		interfaces.surface->DrawRectOutline(x, y, Keys[pKey].width, h);
		interfaces.surface->SetColor(37, 37, 37, 255);
		interfaces.surface->DrawFilledRect(x + 1, y + 1, Keys[pKey].width - 2, h - 2);
		interfaces.surface->SetColor(21, 21, 21, 255);
		interfaces.surface->DrawRectOutline(x + 2, y + 2, Keys[pKey].width - 4, h - 4);
		interfaces.surface->SetTextColor(255, 255, 255, 255);
		interfaces.surface->SetTextPosition(x + (Keys[pKey].width / 2) - (w / 2), y + 3);
		interfaces.surface->DrawText(wszKeyName, wcslen(wszKeyName));
		if (in_region(x, y, Keys[pKey].width, h) && GetAsyncKeyState(VK_LBUTTON))
			Keys[pKey].open = TRUE;
		if (!in_region(x, y, Keys[pKey].width, h) && GetAsyncKeyState(VK_LBUTTON))
			Keys[pKey].open = FALSE;
		if (Keys[pKey].open) {
			interfaces.surface->SetColor(44, 44, 44, 255);
			interfaces.surface->DrawFilledRect(x + 1, y + 1, Keys[pKey].width - 2, h - 2);
			interfaces.surface->SetTextPosition(x + (Keys[pKey].width / 2) - 0x6, y + 3); // 0x6 = 12 / 2, 12 is the size of L"..." on the menu.
			interfaces.surface->DrawText(L"...", 0x3);
			INT nState = *pKey;
			for (INT i = 0; i < 256; i++) {
				USHORT nValue = GetAsyncKeyState(i) & 1;
				if (nValue && i != nState) {
					*pKey = i;
					Keys[pKey].open = FALSE;
				}
			}
		}
	}
}   
VOID SetupFonts() {
	menu::font = interfaces.surface->CreateFont();
	interfaces.surface->SetFontGlyphs(menu::font, "Verdana", 12, 600, 0);
	menu::esp = interfaces.surface->CreateFont();
	interfaces.surface->SetFontGlyphs(menu::esp, "Tahoma", 12, 600, 0x080); // dropshadow = 0x080, antialias = 0x010, outline = 0x200
}
VOID RenderMenu() {
	if (static BOOLEAN once = FALSE; !once) { // cringe init be like | this is better than std::once :P
		menu::size = vec2(420, 260);
		menu::start_pos = vec2(50, 50);
		once = TRUE;
	}
	menu::window(L"singlefile csgo internal");
	menu::checkbox(L"bhop", &config.misc.m_bBhop); 
	menu::checkbox(L"auto pistol", &config.aimbot.m_bAutoPistol);
	menu::checkbox(L"hitsound", &config.misc.m_bHitSound);
	menu::checkbox(L"box esp", &config.visuals.m_bBoxESP);
	menu::checkbox(L"name esp", &config.visuals.m_bNameESP);
	menu::checkbox(L"health bar", &config.visuals.m_bHealthBar);
	menu::checkbox(L"dormant esp", &config.visuals.m_bDormanyCheck);
	menu::checkbox(L"team esp", &config.visuals.m_bTargetTeam);
	menu::checkbox(L"spectator list", &config.misc.m_bSpectatorList);
	menu::checkbox(L"disable post process", &config.visuals.m_bDisablePostProcess);
	menu::checkbox(L"noscope crosshair", &config.misc.m_bNoScopeCrosshair);
	menu::checkbox(L"recoil crosshair", &config.misc.m_bRecoilCrosshair);
	menu::checkbox(L"auto accept", &config.misc.m_bAutoAccept);
	menu::column(184);
	menu::checkbox(L"triggerbot", &config.aimbot.m_bTriggerbot);
	menu::checkbox(L"trigger key", &config.aimbot.m_bTriggerKey);
	menu::keybinder(&config.aimbot.m_nTriggerKey);
	menu::checkbox(L"radar", &config.visuals.m_bRadar);
	menu::checkbox(L"disable keyboard in menu", &config.misc.m_bGameKeyboard);
	menu::checkbox(L"rank revealer", &config.visuals.m_bRankRevealer);
	menu::checkbox(L"use spam", &config.misc.m_bUseSpam);
	menu::checkbox(L"flash reducer", &config.visuals.m_bFlashReducer);
	menu::checkbox(L"vote revealer", &config.misc.m_bVoteRevealer);
	if (menu::button(L"load", {menu::start_pos.x + 10, menu::start_pos.y + 220}, {195, 30}))
		load("singlefile");
	if (menu::button(L"save", {menu::start_pos.x + 215, menu::start_pos.y + 220}, {195, 30}))
		save("singlefile");
}

class CUserCmd {
private:
	BYTE pad_0x0[0x4];
public:
	int			m_nCommandNumber;
	int			m_nTickCount;
	vec3		m_vecAngles;
	vec3		m_vecDirection;
	FLOAT		m_flForwardMove;
	FLOAT		m_flSideMove;
	FLOAT		m_flUpMove;
	int			m_nButtons;
	char		m_chImpulse;
	int			m_nWeaponSelect;
	int			m_nWeaponType;
	short		m_shSeed;
	short		m_shMouseDX;
	short		m_shMouseDY;
	BOOLEAN		m_bHasBeenPredicted;
private:
	BYTE pad_0x1[0x18];
};
BOOLEAN(WINAPI* CreateMoveOriginal)(FLOAT, CUserCmd*);
VOID(__thiscall* PaintTraverseOriginal)(IPanel*, DWORD, BOOLEAN, BOOLEAN);
BOOLEAN(__thiscall* GameEventsOriginal)(IGameEventManager2*, IGameEvent*);
VOID(WINAPI* EmitSoundOriginal)(PVOID, INT, INT, LPCSTR, DWORD, LPCSTR, FLOAT, INT, INT, INT, INT, const vec3&, const vec3&, PVOID, BOOLEAN, FLOAT, INT, PVOID);
BOOLEAN(__thiscall* DispatchUserMessageOriginal)(PVOID, INT, INT, INT, PVOID);
LRESULT CALLBACK Wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN) {
		switch (wParam) {
		case VK_INSERT:
			menu_open = !menu_open;
			break;
		}
	}
	menu::clicked = (BOOLEAN)(wParam & MK_LBUTTON);
	if (uMsg == WM_MOUSEMOVE) {
		menu::move((INT)((SHORT)(LOWORD(lParam))), (INT)((SHORT)(HIWORD(lParam))));
		menu::inmove = TRUE;
	} else {
		menu::inmove = FALSE;
	}
	return CallWindowProc(orig_proc, hWnd, uMsg, wParam, lParam);
}
enum {
	IN_ATTACK = 1 << 0,
	IN_JUMP = 1 << 1,
	IN_USE = 1 << 5,
	IN_SCORE = 1 << 16,
	IN_COUNT = 1 << 26,
};
namespace colors { unsigned char green[4] = {0, 255, 0, 255}; unsigned char lightgreen[4] = {10, 200, 10, 255}; unsigned char red[4] = {255, 0, 0, 255}; unsigned char lightred[4] = {200, 10, 10, 255}; };
void(*ColoredMsg)(PUCHAR, LPCSTR, ...);
VOID voterevealer(IGameEvent* evt = NULL) {
	if (!config.misc.m_bVoteRevealer || !interfaces.engine->IsInGame())
		return;
	CBaseEntity* pEntity = interfaces.entitylist->GetEntity(evt->GetInt("entityid"));
	if (!pEntity || pEntity->GetClientClass()->m_nClassID != CCSPlayer)
		return;
	BOOLEAN bF1d = (!evt->GetBool("vote_option"));
	SPlayerInfo PlayerInfo;
	interfaces.engine->GetPlayerInfo(evt->GetInt("entityid"), &PlayerInfo);
	ColoredMsg(bF1d ? colors::lightgreen : colors::lightred, "[singlefile]: %s voted %s.\n", PlayerInfo.m_szName, bF1d ? "Yes" : "No");
	Beep(bF1d ? 626 : 560, 75);
}
VOID bhop(CUserCmd* cmd) {
	if (config.misc.m_bBhop) {
		CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
		if (localplayer->GetHealth() == 0)
			return;
		INT m_nMoveType = localplayer->MoveType();
		if (m_nMoveType == LADDER || m_nMoveType == NOCLIP || m_nMoveType == FLY || m_nMoveType == OBSERVER)
			return;
		if (!(localplayer->GetFlags() & ONGROUND))
			cmd->m_nButtons &= ~IN_JUMP;
	}
}
VOID autopistol(CUserCmd* cmd) {
	if (config.aimbot.m_bAutoPistol) {
		CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
		if (localplayer->GetHealth() == 0)
			return;
		CBaseEntity* weapon = localplayer->GetWeapon();
		if (weapon->GetWeaponType() != 1)
			return;
		if (weapon->WeaponNextAttack() > interfaces.globals->m_flCurrentTime)
			cmd->m_nButtons &= ~IN_ATTACK;
	}
}
VOID autoaccept(LPCSTR sound) {
	if (strstr(sound, "UIPanorama.popup_accept_match_beep")) {
		static BOOLEAN(WINAPI * SetLPReady)(LPCSTR) = (decltype(SetLPReady))PatternScan(client_dll, "55 8B EC 83 E4 F8 8B 4D 08 BA ? ? ? ? E8 ? ? ? ? 85 C0 75 12");
		if (config.misc.m_bAutoAccept)
			SetLPReady("");
	}
}
VOID flashreducer() {
	if (!config.visuals.m_bFlashReducer || !interfaces.engine->IsInGame())
		return;
	CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	if (localplayer->FlashDuration() > 3.f) {
		localplayer->FlashMaxAlpha() = 100;
		interfaces.surface->SetTextColor(255, 100, 100, 255);
		DWORD w, h, tw, th;
		interfaces.engine->GetScreenSize(w, h);
		interfaces.surface->GetTextSize(6, L"FLASHED!", tw, th); // first 50 built-in vgui fonts: https://cdn.discordapp.com/attachments/634094496300400641/821827439042101258/unknown.png
		interfaces.surface->SetTextPosition((DWORD)((w * 0.5f) - tw * 0.5f), (DWORD)(h * 0.75f));
		interfaces.surface->SetTextFont(6); 
		interfaces.surface->DrawText(L"FLASHED!", 0x8);
	}
}
struct bbox {
	INT x, y, w, h;
};
BOOLEAN WorldToScreen(const vec3& world, vec3& screen)
{
	matrix4x4_t& view = interfaces.engine->GetViewMatrix();
	screen.x = world.x * view[0][0] + world.y * view[0][1] + world.z * view[0][2] + view[0][3];
	screen.y = world.x * view[1][0] + world.y * view[1][1] + world.z * view[1][2] + view[1][3];
	screen.z = world.x * view[2][0] + world.y * view[2][1] + world.z * view[2][2] + view[2][3];
	FLOAT  w = world.x * view[3][0] + world.y * view[3][1] + world.z * view[3][2] + view[3][3];
	if (w < 0.1f)
		return FALSE;
	vec3 ndc;
	ndc.x = screen.x / w;
	ndc.y = screen.y / w;
	ndc.z = screen.z / w;
	DWORD x, y;
	interfaces.engine->GetScreenSize(x, y);
	screen.x = (x / 2 * ndc.x) + (ndc.x + x / 2);
	screen.y = -(y / 2 * ndc.y) + (ndc.y + y / 2);
	return TRUE;
}
#define FL_MAX 3.40282e+038;
vec3 VectorTransform(vec3 in, matrix_t matrix) {
	return vec3(in.dot(matrix[0]) + matrix[0][3], in.dot(matrix[1]) + matrix[1][3], in.dot(matrix[2]) + matrix[2][3]);
}
BOOLEAN getbbot(CBaseEntity* player, bbox& box) {
	matrix_t& rgflTransFrame = (matrix_t&)player->GetCoordinateFrame();
	const vec3 min = player->CollisonMins();
	const vec3 max = player->CollisonMaxs();
	vec3 vecTransScreen[8];
	vec3 points[] = {
		vec3(min.x, min.y, min.z),
		vec3(min.x, max.y, min.z),
		vec3(max.x, max.y, min.z),
		vec3(max.x, min.y, min.z),
		vec3(max.x, max.y, max.z),
		vec3(min.x, max.y, max.z),
		vec3(min.x, min.y, max.z),
		vec3(max.x, min.y, max.z)
	};
	for (INT i = 0; i <= 7; i++) {
		if (!WorldToScreen(VectorTransform(points[i], rgflTransFrame), vecTransScreen[i]))
			return FALSE;
	}
	vec3 vecBoxes[] = {
		vecTransScreen[3], vecTransScreen[5], vecTransScreen[0], vecTransScreen[4], vecTransScreen[2], vecTransScreen[1], vecTransScreen[6], vecTransScreen[7] 
	};
	FLOAT flLeft = vecTransScreen[3].x, flBottom = vecTransScreen[3].y, flRight = vecTransScreen[3].x, flTop = vecTransScreen[3].y;
	for (INT i = 0; i <= 7; i++) {
		if (flLeft > vecBoxes[i].x)
			flLeft = vecBoxes[i].x;
		if (flBottom < vecBoxes[i].y)
			flBottom = vecBoxes[i].y;
		if (flRight < vecBoxes[i].x)
			flRight = vecBoxes[i].x;
		if (flTop > vecBoxes[i].y)
			flTop = vecBoxes[i].y;
	}
	box.x = (INT)(flLeft);
	box.y = (INT)(flTop);
	box.w = (INT)(flRight)-(INT)(flLeft);
	box.h = (INT)(flBottom)-(INT)(flTop);
	return TRUE;
}
struct rgba {
	INT r, g, b, a;
	rgba(INT r = 0, INT g = 0, INT b = 0, INT a = 255) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
};
VOID players() {
	if (!interfaces.engine->IsInGame())
		return;
	CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	if (localplayer->GetHealth() > 0 && (config.visuals.m_bOnlyOnDead))
		return;
	for (INT i = 1; i <= interfaces.engine->GetMaxClients(); i++) {
		CBaseEntity* entity = interfaces.entitylist->GetEntity(i);
		if (!entity || entity->GetHealth() == 0 || entity->GetClientClass()->m_nClassID != CCSPlayer)
			continue;
		if (!(config.visuals.m_bTargetTeam) && entity->GetTeamNumber() == localplayer->GetTeamNumber())
			continue;
		if (!(config.visuals.m_bDormanyCheck) && entity->IsDormant())
			continue;
		bbox box;
		if (!getbbot(entity, box))
			continue;
		if (config.visuals.m_bBoxESP) {
			interfaces.surface->SetColor(255, 255, 255, 255);
			interfaces.surface->DrawRectOutline(box.x, box.y, box.w, box.h);
			interfaces.surface->SetColor(0, 0, 0, 255);
			interfaces.surface->DrawRectOutline(box.x + 1, box.y + 1, box.w - 2, box.h - 2);
			interfaces.surface->DrawRectOutline(box.x - 1, box.y - 1, box.w + 2, box.h + 2);
		}
		if (config.visuals.m_bNameESP) {
			interfaces.surface->SetTextColor(255, 255, 255, 255);
			interfaces.surface->SetTextFont(menu::font);
			DWORD o, p;
			SPlayerInfo plr;
			interfaces.engine->GetPlayerInfo(i, &plr);
			wchar_t wname[128];
			if (MultiByteToWideChar(65001, 0, plr.m_szName, -1, wname, 128)) {
				interfaces.surface->GetTextSize(menu::esp, wname, o, p);
				interfaces.surface->SetTextPosition(box.x + (box.w / 2) - o / 2, box.y - 12);
				interfaces.surface->DrawText(wname, wcslen(wname));
			}
		}
		if (config.visuals.m_bHealthBar) {
			rgba healthclr;
			if (entity->GetHealth() > 100)
				healthclr = rgba(0, 255, 0, 255);
			else
				healthclr = rgba((INT)(255 - entity->GetHealth() * 2.55f), (INT)(entity->GetHealth() * 2.55f), 0, 255);
			interfaces.surface->SetColor(0, 0, 0, 255);
			interfaces.surface->DrawFilledRect(box.x - 10, box.y - 1, 5, box.h + 2);
			interfaces.surface->SetColor(healthclr.r, healthclr.g, healthclr.b, healthclr.a);
			interfaces.surface->DrawFilledRect(box.x - 9, (box.y + box.h - (DWORD)((box.h * (entity->GetHealth() / 100.f)))), 3, (DWORD)((FLOAT)box.h * entity->GetHealth() / 100.f) + (entity->GetHealth() == 100 ? 0 : 1));
		}
		if (config.visuals.m_bRadar)
			entity->Spotted() = TRUE;
	}
}
VOID cvars() {
	CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	interfaces.cvar->FindVar("mat_postprocess_enable")->SetValue(config.visuals.m_bDisablePostProcess ? 0 : 1); 
	interfaces.cvar->FindVar("cl_crosshair_recoil")->SetValue(config.misc.m_bRecoilCrosshair ? 1 : 0); // i'm sure the ? 1 : 0 doesn't matter but this feels better. /shrug
	interfaces.cvar->FindVar("weapon_debug_spread_show")->SetValue(((config.misc.m_bNoScopeCrosshair) && !localplayer->IsScoped()) ? 2 : 0);
}
VOID speclist() {
	if (!interfaces.engine->IsInGame())
		return;
	CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	if (!localplayer)
		return;
	static INT b = 0;
	if (config.misc.m_bSpectatorList) {
		for (INT i = 1; i <= interfaces.engine->GetMaxClients(); i++) {
			CBaseEntity* entity = interfaces.entitylist->GetEntity(i);
			if (!entity || entity->GetHealth() > 0 || !entity->GetObserverTarget())
				continue;
			if (entity->GetObserverTarget() != localplayer)
				continue;
			SPlayerInfo player;
			interfaces.engine->GetPlayerInfo(i, &player);
			interfaces.surface->SetTextColor(255, 255, 255, 255);
			interfaces.surface->SetTextFont(menu::font);
			DWORD x, y;
			interfaces.engine->GetScreenSize(x, y);
			interfaces.surface->SetTextPosition(10, 10 + b);
			WCHAR tmp[128];
			if (MultiByteToWideChar(65001, 0, player.m_szName, -1, tmp, 128)) {
				interfaces.surface->DrawText(tmp, wcslen(tmp));
			}
			b += 12;
		}
	}
	b = 0;
}
VOID triggerbot(CUserCmd* cmd) {
	if (!(config.aimbot.m_bTriggerbot))
		return;
	CBaseEntity* lp = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	if (!lp || (lp->GetHealth() < 1) || !lp->CrosshairTarget())
		return;
	CBaseEntity* target = interfaces.entitylist->GetEntity((lp->CrosshairTarget()));
	if (config.aimbot.m_bTriggerKey && !GetAsyncKeyState(config.aimbot.m_nTriggerKey))
		return;
	if (!target->GetHealth() || lp->FlashDuration() > 0.1f || !lp->GetWeapon()->Ammo() || lp->GetWeapon()->WeaponNextAttack() > interfaces.globals->m_flCurrentTime || (!lp->IsScoped() && lp->GetWeapon()->GetWeaponType() == 5) || lp->GetTeamNumber() == target->GetTeamNumber()) // dumbass "hitchance" calculation but it's close enough ig
		return;
	cmd->m_nButtons |= IN_ATTACK;
}
VOID usespam(CUserCmd* cmd) {
	if (config.misc.m_bUseSpam && cmd->m_nButtons == IN_USE) {
		if (cmd->m_nCommandNumber % 2 == 0)
			cmd->m_nButtons |= IN_USE;
		else
			cmd->m_nButtons &= ~IN_USE;
	}
}
BOOLEAN __fastcall _DispatchUserMessage(PVOID ecx, PVOID edx, INT nMessageType, INT nArgument, INT nArgument2, PVOID pData) {
	if (nMessageType == 47 && config.misc.m_bVoteRevealer) {
		ColoredMsg(colors::green, "[singlefile] Vote Passed!\n"); Beep(670, 50); }
	if (nMessageType == 48 && config.misc.m_bVoteRevealer) {
		ColoredMsg(colors::red, "[singlefile] Vote Failed!\n"); Beep(343, 50); }
	return DispatchUserMessageOriginal(interfaces.client, nMessageType, nArgument, nArgument2, pData);
}
BOOLEAN WINAPI _CreateMove(FLOAT flInputSampleTime, CUserCmd* cmd) {
	BOOLEAN SetViewAngles = CreateMoveOriginal(flInputSampleTime, cmd);
	if (cmd->m_nCommandNumber % 4 == 1) {
		cmd->m_nButtons |= IN_COUNT; // anti-afk kick maybe make it it's own option at some point :P
		cvars(); // commands that do not to run each tick (i.e don't need usercmd, just dependent on localplayer & being in game)
	}
	if (cmd->m_nButtons & IN_SCORE && config.visuals.m_bRankRevealer)
		interfaces.client->DispatchUserMessage(50, 0, 0, NULL);
	bhop(cmd);
	autopistol(cmd);
	triggerbot(cmd);
	usespam(cmd);
	return SetViewAngles;
}
VOID WINAPI _EmitSound(void* filter, int entityIndex, int channel, const char* soundEntry, unsigned int soundEntryHash, const char* sample, float volume, int seed, int soundLevel, int flags, int pitch, const vec3& origin, const vec3& direction, void* utlVecOrigins, bool updatePositions, float soundtime, int speakerentity, void* soundParams) { // thank you danielkrupinski/Osiris for these arguments
	autoaccept(soundEntry);
	return EmitSoundOriginal(filter, entityIndex, channel, soundEntry, soundEntryHash, sample, volume, seed, soundLevel, flags, pitch, std::cref(origin), std::cref(direction), utlVecOrigins, updatePositions, soundtime, speakerentity, soundParams);
}
DWORD fnv(LPCSTR szString, DWORD nOffset = 0x811C9DC5) {
	return (*szString == '\0') ? nOffset : fnv(&szString[1], (nOffset ^ DWORD(*szString)) * 0x01000193);
}
BOOLEAN WINAPI _GameEvents(IGameEvent* event) {
	DWORD dwEventHash = fnv(event->GetName());
	if (config.misc.m_bHitSound && dwEventHash == 0x1B30DDF0) {
		SPlayerInfo player;
		interfaces.engine->GetPlayerInfo(interfaces.engine->GetLocalPlayer(), &player);
		if (event->GetInt("attacker") == player.m_nUserID)
			interfaces.engine->ClientCmdUnrestricted("play buttons/arena_switch_press_02");
	}
	if (dwEventHash == 0xFDAD5FE5 && config.misc.m_bVoteRevealer)
		voterevealer(event);
	return GameEventsOriginal(interfaces.events, event);
}
VOID WINAPI _PaintTraverse(DWORD dwPanel, BOOLEAN bForceRepaint, BOOLEAN bAllowRepaint) {
	DWORD drawing = fnv(interfaces.panel->GetPanelName(dwPanel));
	if (drawing == 0xA4A548AF) { // fnv("MatSystemTopPanel") = 0xA4A548AF
		players();
		speclist();
		flashreducer();
		if (menu_open)
			RenderMenu();
	}
	if (drawing == 0x8BE56F81) { // fnv("FocusOverlayPanel") = 0x8BE56F81
		interfaces.panel->SetInputMouseState(dwPanel, menu_open);
		interfaces.panel->SetInputKeyboardState(dwPanel, menu_open && (config.misc.m_bGameKeyboard));
	}
	return PaintTraverseOriginal(interfaces.panel, dwPanel, bForceRepaint, bAllowRepaint);
}
VOID LoadHooks() {
	MH_Initialize();
	MH_CreateHook((*(PVOID**)(interfaces.client_mode))[24], &_CreateMove, (PVOID*)&CreateMoveOriginal);
	MH_CreateHook((*(PVOID**)(interfaces.panel))[41], &_PaintTraverse, (PVOID*)&PaintTraverseOriginal);
	MH_CreateHook((*(PVOID**)(interfaces.events))[9], &_GameEvents, (PVOID*)&GameEventsOriginal);
	MH_CreateHook((*(PVOID**)(interfaces.sound))[5], &_EmitSound, (PVOID*)&EmitSoundOriginal);
	MH_CreateHook((*(PVOID**)(interfaces.client))[38], &_DispatchUserMessage, (PVOID*)&DispatchUserMessageOriginal);
	MH_EnableHook(NULL);
}
template <class T>
T CreateInterface(PVOID m_pModule, LPCSTR m_szInterface) {
	return ((T(*)(LPCSTR, DWORD))GetProcAddress((HMODULE)m_pModule, "CreateInterface"))(m_szInterface, 0x0);
}
INT GetLineCount();
VOID WINAPI Init (HMODULE mod) {
	while (!GetModuleHandleA("serverbrowser.dll"))
		Sleep(250);
	AllocConsole();
	SetConsoleTitleA("singlefile: console");
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	printf("singlefile v1.3 beta: loading... (compiled with %d lines of code)\n", GetLineCount());
	csgo_window = FindWindowA("Valve001", NULL);
	orig_proc = (WNDPROC)SetWindowLongA(csgo_window, GWLP_WNDPROC, (LONG)Wndproc);
	client_dll = GetModuleHandleA("client.dll");
	engine_dll = GetModuleHandleA("engine.dll");
	PVOID surface_dll = GetModuleHandleA("vguimatsurface.dll");
	PVOID vgui2_dll = GetModuleHandleA("vgui2.dll");
	PVOID vstdlib_dll = GetModuleHandleA("vstdlib.dll");
	interfaces.engine = CreateInterface<IVEngineClient*>(engine_dll, "VEngineClient014");
	if (!strstr(interfaces.engine->GetVersionString(), "1.37.8.7"))
		printf("note: you are using an unknown cs:go client version (%s). if you are experiencing crashes, you may need to update offsets. each offset in the source code has it's netvar name, or you can find it on hazedumper.\n", interfaces.engine->GetVersionString());
	interfaces.entitylist = CreateInterface<CBaseEntityList*>(client_dll, "VClientEntityList003");
	interfaces.surface = CreateInterface<CMatSystemSurface*>(surface_dll, "VGUI_Surface031");
	interfaces.panel = CreateInterface<IPanel*>(vgui2_dll, "VGUI_Panel009");
	interfaces.client = CreateInterface<IClient*>(client_dll, "VClient018");
	interfaces.cvar = CreateInterface<ICVar*>(vstdlib_dll, "VEngineCvar007");
	interfaces.events = CreateInterface<IGameEventManager2*>(engine_dll, "GAMEEVENTSMANAGER002");
	interfaces.sound = CreateInterface<ISound*>(engine_dll, "IEngineSoundClient003");
	interfaces.client_mode = **(IClientModeShared***)((*(DWORD**)(interfaces.client))[0xA] + 0x5);
	interfaces.globals = **(CGlobalVarsBase***)((*(DWORD**)(interfaces.client))[0xB] + 0xA);
	ColoredMsg = (decltype(ColoredMsg))GetProcAddress(GetModuleHandleA("tier0.dll"), "?ConColorMsg@@YAXABVColor@@PBDZZ");
	SetupFonts();
	LoadHooks();
	printf("finished loading.\n");
	while (!GetAsyncKeyState(VK_END))
		Sleep(500);
	MH_DisableHook(NULL); // NULL = all hooks
	MH_RemoveHook(NULL);
	MH_Uninitialize();
	FreeConsole();
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID pReserved) {
	if (dwReason == DLL_PROCESS_ATTACH)
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Init, hModule, 0, NULL);
	return TRUE;
}
INT GetLineCount() { // must be at bottom obviously :P
	return (__LINE__ + 0x1);
}