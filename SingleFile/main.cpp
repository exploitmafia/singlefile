#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <string>
#pragma comment(lib, "minhook")
#define IN_RANGE(x,a,b)        (x >= a && x <= b) 
#define GET_BITS( x )        (IN_RANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xA))
#define GET_BYTE( x )        (GET_BITS(x[0x0]) << 0x4 | GET_BITS(x[0x1]))
void* client_dll = nullptr;
void* engine_dll = nullptr;
int CCSPlayer = 0x28; // ClassID::CCSPlayer = 40;
#define TriggerBotKEY VK_MBUTTON // 0 for no key or a vk code (ex. ALT = VK_LMENU, see https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)
typedef enum MH_STATUS
{
	MH_UNKNOWN = -1,
	MH_OK = 0,
	MH_ERROR_ALREADY_INITIALIZED,
	MH_ERROR_NOT_INITIALIZED,
	MH_ERROR_ALREADY_CREATED,
	MH_ERROR_NOT_CREATED,
	MH_ERROR_ENABLED,
	MH_ERROR_DISABLED,
	MH_ERROR_NOT_EXECUTABLE,
	MH_ERROR_UNSUPPORTED_FUNCTION,
	MH_ERROR_MEMORY_ALLOC,
	MH_ERROR_MEMORY_PROTECT,
	MH_ERROR_MODULE_NOT_FOUND,
	MH_ERROR_FUNCTION_NOT_FOUND
}
MH_STATUS; // get min hook here: https://github.com/TsudaKageyu/minhook | License for minhook (text of license has not been modified, just newlines removed) : /* MinHook - The Minimalistic API Hooking Library for x64 / x86 * Copyright(C) 2009 - 2017 Tsuda Kageyu. * All rights reserved. * *Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met : * *1. Redistributions of source code must retain the above copyright * notice, this list of conditionsand the following disclaimer. * 2. Redistributions in binary form must reproduce the above copyright * notice, this list of conditionsand the following disclaimer in the * documentationand /or other materials provided with the distribution. * *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A * PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, *PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. * /
#define MH_ALL_HOOKS NULL
extern "C" {
	MH_STATUS WINAPI MH_Initialize(VOID);
	MH_STATUS WINAPI MH_Uninitialize(VOID);
	MH_STATUS WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
	MH_STATUS WINAPI MH_RemoveHook(LPVOID pTarget);
	MH_STATUS WINAPI MH_EnableHook(LPVOID pTarget);
	MH_STATUS WINAPI MH_DisableHook(LPVOID pTarget);
}

unsigned char* PatternScan(void* m_pModule, const char* m_szSignature) {
	const char* pat = m_szSignature;
	unsigned char* first_match = 0x0;
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)m_pModule;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((char*)m_pModule + dos->e_lfanew);
	for (unsigned char* current = (unsigned char*)m_pModule; current < (unsigned char*)m_pModule + nt->OptionalHeader.SizeOfCode; current++) {
		if (*(unsigned char*)pat == '\?' || *(unsigned char*)current == GET_BYTE(pat)) {
			if (!*pat)
				return first_match;
			if (!first_match)
				first_match = current;
			if (!pat[0x2])
				return first_match;
			pat += (*(unsigned short*)pat == (unsigned short)'\?\?' || *(unsigned char*)pat != (unsigned char)'\?') ? 0x3 : 0x2;
		}
		else {
			if (first_match != 0x0)
				current = first_match;
			pat = m_szSignature;
			first_match = 0x0;
		}
	}
	return nullptr;
}
namespace fnv_1a {
	template< typename S >
	struct fnv_internal;
	template< typename S >
	struct fnv1a;
	template< >
	struct fnv_internal< uint32_t > {
		constexpr static uint32_t default_offset_basis = 0x811C9DC5;
		constexpr static uint32_t prime = 0x01000193;
	};
	template< >
	struct fnv1a< uint32_t > : public fnv_internal< uint32_t > {
		constexpr static uint32_t hash(char const* string, const uint32_t val = default_offset_basis) {
			return (string[0] == '\0') ? val : hash(&string[1], (val ^ uint32_t(string[0])) * prime);
		}
		constexpr static uint32_t hash(wchar_t const* string, const uint32_t val = default_offset_basis) {
			return (string[0] == L'\0') ? val : hash(&string[1], (val ^ uint32_t(string[0])) * prime);
		}
	};
}
using fnv = fnv_1a::fnv1a< uint32_t >;
#undef DrawText
#undef CreateFont
template <typename I>
__forceinline I v(void* iface, unsigned int index) { return (I)((*(unsigned int**)iface)[index]); }
using matrix_t = float[3][4];
using matrix4x4_t = float[4][4];
// config system
bool menu_open = true;
struct sconfig {
	struct saim {
		bool m_bTriggerbot;
		bool m_bAutoPistol;
	}aimbot;
	struct svisuals {
		bool m_bBoxESP;
		bool m_bNameESP;
		bool m_bHealthBar;
		bool m_bTargetTeam;
		bool m_bDormanyCheck;
		bool m_bOnlyOnDead;
		bool m_bRadar;
		bool m_bDisablePostProcess;
	}visuals;
	struct smisc {
		bool m_bBhop;
		bool m_bHitSound;
		bool m_bNoScopeCrosshair;
		bool m_bRecoilCrosshair;
		bool m_bAutoAccept;
		bool m_bGameKeyboard;
		bool m_bSpectatorList;
	}misc;
}config;
class vec3 {
public:
	float x, y, z;
	vec3(float a = 0, float b = 0, float c = 0) {
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
	float dot(float* a) { return x * a[0] + y * a[1] + z * a[2]; }
	void clear() { x = y = z = 0.f; }
};
struct SPlayerInfo {
	unsigned long long m_ullVersion;
	union {
		unsigned long long m_ullXUID;
		struct {
			unsigned int m_nXUIDLow;
			unsigned int m_nXUIDHigh;
		};
	};
	char m_szName[128];
	int m_nUserID;
	char m_szGUID[33];
	unsigned int m_nFriendsID;
	char m_szFriendsName[128];
	bool m_bIsBot;
	bool m_bIsHLTV;
	int m_nCustomFiles[4];
	unsigned char m_ucFilesDownloaded;
};
class CMatSystemSurface {
public:
	__forceinline void DrawFilledRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
		return v<void(__thiscall*)(void*, unsigned int, unsigned int, unsigned int, unsigned int)>(this, 16)(this, x, y, x + w, y + h);
	}
	__forceinline void SetColor(unsigned short r, unsigned short g, unsigned short b, unsigned short a) {
		return v<void(__thiscall*)(void*, unsigned short, unsigned short, unsigned short, unsigned short)>(this, 15)(this, r, g, b, a);
	}
	__forceinline void SetTextColor(unsigned short r, unsigned short g, unsigned short b, unsigned short a) {
		return v<void(__thiscall*)(void*, unsigned short, unsigned short, unsigned short, unsigned short)>(this, 25)(this, r, g, b, a);
	}
	__forceinline void SetTextPosition(unsigned int x, unsigned int y) {
		return v<void(__thiscall*)(void*, unsigned int, unsigned int)>(this, 26)(this, x, y);
	}
	__forceinline void DrawText(const wchar_t* text, unsigned int len) {
		return v<void(__thiscall*)(void*, const wchar_t*, unsigned int, unsigned int)>(this, 28)(this, text, len, 0);
	}
	__forceinline unsigned long CreateFont() {
		return v<unsigned int(__thiscall*)(void*)>(this, 71)(this);
	}
	__forceinline bool SetFontGlyphs(unsigned long _font, const char* name, unsigned int height, unsigned int weight, unsigned int font_flags) {
		return v<bool(__thiscall*)(void*, unsigned int, const char*, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)>(this, 72)(this, _font, name, height, weight, 0, 0, font_flags, 0, 0);
	}
	__forceinline void SetTextFont(unsigned int _font) {
		return v<void(__thiscall*)(void*, unsigned int)>(this, 23)(this, _font);
	}
	__forceinline void DrawRectOutline(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
		return v<void(__thiscall*)(void*, unsigned int, unsigned int, unsigned int, unsigned int)>(this, 18)(this, x, y, x + w, y + h);
	}
	__forceinline void GetTextSize(unsigned int _font, const wchar_t* text, unsigned int& w, unsigned int& h) {
		return v<void(__thiscall*)(void*, unsigned int, const wchar_t*, unsigned int&, unsigned int&)>(this, 79)(this, _font, text, w, h);
	}
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
	void* CreateClassFn;
	void* CreateEventFn;
	char* m_szNetworkedName;
	void* m_pRecvTable;
	CCSClientClass* m_pNextClass;
	int m_nClassID;
};
class CBaseEntity {
public:
	__forceinline void* GetNetworkable() {
		return (void*)(this + 0x8);
	}
	__forceinline CCSClientClass* GetClientClass() {
		void* networkable = this->GetNetworkable();
		return v<CCSClientClass*(__thiscall*)(void*)>(networkable, 2)(networkable);
	}
	__forceinline vec3 CollisonMins() { // CBaseEntity::m_Collison::m_vecMins
		return *(vec3*)(this + 0x328);
	}
	__forceinline vec3 CollisonMaxs() { // CBaseEntity::m_Collison::m_vecMaxs
		return *(vec3*)(this + 0x334);
	}
	__forceinline vec3 GetAbsOrigin() {
		return v<vec3&(__thiscall*)(void*)>(this, 10)(this);
	}
	__forceinline vec3 GetViewOffset() { //	CBaseEntity::localdata::m_vecViewOffset
		return *(vec3*)(this + 0x108);
	}
	__forceinline vec3 GetEyePosition() {
		return (this->GetAbsOrigin() + this->GetViewOffset());
	}
	__forceinline unsigned char IsDormant() { // client.dll!8A 81 ? ? ? ? C3 32 C0 + 0x2
		return *(unsigned char*)(this + 0xED);
	}
	__forceinline bool IsInImmunity() { // CCSPlayer::m_bHasGunGameImmunity
		return *(bool*)(this + 0x3944);
	}
	__forceinline int GetFlags() { // CCSPlayer::m_fFlags
		return *(int*)(this + 0x104);
	}
	__forceinline int MoveType() { // CCSPlayer::m_MoveType
		return *(int*)(this + 0x25C);
	}
	__forceinline int GetHealth() { // CBaseEntity::m_iHealth
		return *(int*)(this + 0x100);
	}
	__forceinline CBaseEntity* GetWeapon() {
		return v<CBaseEntity* (__thiscall*)(void*)>(this, 267)(this);
	}
	__forceinline int GetWeaponType() {
		return v<int(__thiscall*)(void*)>(this, 454)(this);
	}
	__forceinline float WeaponNextAttack() { // CBaseCombatWeapon::m_flNextPrimaryAttack
		return *(float*)(this + 0x3238);
	}
	__forceinline matrix_t& GetCoordinateFrame() {
		return *(matrix_t*)((this) + 0x444);
	}
	__forceinline int GetTeamNumber() {
		return *(int*)(this + 0xF4);
	}
	__forceinline CBaseEntity* GetObserverTarget() {
		return v<CBaseEntity* (__thiscall*)(void*)>(this, 294)(this);
	}
	__forceinline bool& IsScoped() {
		return *(bool*)(this + 0x3928);
	}
	__forceinline bool& Spotted() {
		return *(bool*)(this + 0x93D);
	}
	__forceinline int& ObserverMode() {
		return *(int*)(this + 0x3378);
	}
	__forceinline float& FlashDuration() {
		return *(float*)(this + 0xA420);
	}
	__forceinline int Ammo() {
		return *(int*)(this + 0x3264);
	}
	__forceinline int CrosshairTarget() {
		return *(int*)(this + 0xB3E4);
	}
};
class CGlobalVarsBase {
public:
	float		m_flRealTime;
	int         m_nFrameCount;
	float		m_flAbsFrameTime;
	float		m_flAbsFrameStart;
	float		m_flCurrentTime;
	float		m_flFrameTime;
	int         m_nMaxClients;
	int         m_nTickCount;
	float		m_flTickInterval;
	float		m_flInteropolationAmount;
	int	        m_nTicksThisFrmae;
	int         m_nNetworkProtocol;
	void*		m_pGameSaveData;
	bool		m_bClient;
	bool		m_bRemoteClient;
private:
	int32_t     unk1;
	int32_t     unk2;
};
class CBaseEntityList {
public:
	__forceinline CBaseEntity* GetEntity(int index) {
		return v<CBaseEntity* (__thiscall*)(void*, int)>(this, 3)(this, index);
	}
};
class IVEngineClient {
public:
	__forceinline int GetMaxClients() {
		return v<int(__thiscall*)(void*)>(this, 20)(this);
	}
	__forceinline void GetScreenSize(uint32_t& w, uint32_t& h) {
		return v<void(__thiscall*)(void*, uint32_t&, uint32_t&)>(this, 5)(this, w, h);
	}
	__forceinline bool GetPlayerInfo(int idx, SPlayerInfo* info) {
		return v<bool(__thiscall*)(void*, int, SPlayerInfo*)>(this, 8)(this, idx, info);
	}
	__forceinline unsigned int GetLocalPlayer() {
		return v<unsigned int(__thiscall*)(void*)>(this, 12)(this);
	}
	__forceinline bool IsInGame() {
		return v<bool(__thiscall*)(void*)>(this, 26)(this);
	}
	__forceinline matrix4x4_t& GetViewMatrix() {
		return v<matrix4x4_t& (__thiscall*)(void*)>(this, 37)(this);
	}
	__forceinline void ClientCmdUnrestricted(const char* szCommand) {
		return v<void(__thiscall*)(void*, const char*, bool)>(this, 114)(this, szCommand, false);
	}
	__forceinline const char* GetVersionString() {
		return v<const char* (__thiscall*)(void*)>(this, 105)(this);
	}
	__forceinline int GetPlayerIndex(int m_nIndex) {
		return v<int(__thiscall*)(void*, int)>(this, 9)(this, m_nIndex);
	}
};
class IGameEvent {
public:
	__forceinline const char* GetName() {
		return v<const char*(__thiscall*)(void*)>(this, 1)(this);
	}
	__forceinline bool GetBool(const char* keyName) {
		return v<bool(__thiscall*)(void*, const char*, bool)>(this, 5)(this, keyName, false);
	}
	__forceinline int GetInt(const char* keyName) {
		return v<int(__thiscall*)(void*, const char*, int)>(this, 6)(this, keyName, 0);
	}
};
class IPanel {
public:
	__forceinline void SetInputKeyboardState(unsigned int PanelID, bool State) {
		return v<void(__thiscall*)(void*, unsigned int, bool)>(this, 31)(this, PanelID, State);
	}
	__forceinline void SetInputMouseState(unsigned int PanelID, bool State) {
		return v<void(__thiscall*)(void*, unsigned int, bool)>(this, 32)(this, PanelID, State);
	}
	__forceinline const char* GetPanelName(unsigned int PanelID) {
		return v<const char* (__thiscall*)(void*, unsigned int)>(this, 36)(this, PanelID);
	}
};
class CConvar {
public:
	__forceinline float GetFloat() {
		return v<float(__thiscall*)(void*)>(this, 12)(this);
	}
	__forceinline int GetInt() {
		return v<int(__thiscall*)(void*)>(this, 13)(this);
	}
	__forceinline void SetValue(const char* value) {
		return v<void(__thiscall*)(void*, const char*)>(this, 14)(this, value);
	}
	__forceinline void SetValue(float value) {
		return v<void(__thiscall*)(void*, float)>(this, 15)(this, value);
	}
	__forceinline void SetValue(int value) {
		return v<void(__thiscall*)(void*, int)>(this, 16)(this, value);
	}
};
class ICVar {
public:
	__forceinline CConvar* FindVar(const char* name) {
		return v<CConvar* (__thiscall*)(void*, const char*)>(this, 15)(this, name);
	}
};
class IClient;
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
	int x, y;
};
int* Cursor() {
	static int tmp[2];
	POINT m_poCursor;
	GetCursorPos(&m_poCursor);
	ScreenToClient(csgo_window, &m_poCursor);
	tmp[0] = m_poCursor.x;
	tmp[1] = m_poCursor.y;
	return tmp;
}
bool IsMouseInRegion(int x, int y, int w, int h) {
	return Cursor()[0] > x && Cursor()[1] > y && Cursor()[0] < w + x && Cursor()[1] < h + y;
}
void load() { // not proud of using cpp here, but line count matters...
	FILE* cfg = fopen("singlefile.cfg", "r");
	fread(&config, sizeof(config), 1, cfg);
	fclose(cfg);
}
void save() {
	FILE* cfg = fopen("singlefile.cfg", "w");
	fwrite(&config, sizeof(config), 1, cfg);
	fclose(cfg);
}
namespace menu {
	unsigned long font, esp;
	int x_pos = 0, y_pos = 0, vheight = 0, rpos = 0;
	void window(const wchar_t* name, vec2 pos, vec2 size) {
		interfaces.surface->SetColor(23, 23, 30, 255);
		interfaces.surface->DrawRectOutline(pos.x - 1, pos.y - 1, size.x + 2, size.y + 2);
		interfaces.surface->SetColor(62, 62, 72, 255);
		interfaces.surface->DrawRectOutline(pos.x, pos.y, size.x, size.y);
;		interfaces.surface->SetColor(37, 37, 37, 255);
		interfaces.surface->DrawFilledRect(pos.x + 1, pos.y + 1, size.x - 2, size.y - 2);
		interfaces.surface->SetColor(62, 62, 72, 255);
		interfaces.surface->DrawRectOutline(pos.x + 5, pos.y + 5, size.x - 10, size.y - 10);
		interfaces.surface->SetColor(30, 30, 30, 255);
		interfaces.surface->DrawFilledRect(pos.x + 6, pos.y + 6, size.x - 12, size.y - 12);
		interfaces.surface->SetColor(45, 45, 48, 255);
		interfaces.surface->DrawFilledRect(pos.x + 6, pos.y + 6, size.x - 12, 14);
		interfaces.surface->SetColor(60, 60, 70, 255);
		interfaces.surface->DrawFilledRect(pos.x + 6, pos.y + 20, size.x - 12, 1);
		interfaces.surface->SetTextColor(255, 255, 255, 255);
		interfaces.surface->SetTextFont(menu::font);
		static unsigned int u, i;
		interfaces.surface->GetTextSize(menu::font, name, u, i);
		interfaces.surface->SetTextPosition(pos.x + (size.x / 2) - (u / 2), pos.y + 6);
		interfaces.surface->DrawText(name, wcslen(name));
		x_pos = pos.x + 10;
		y_pos = pos.y + 25;
		rpos = pos.x;
		vheight = size.y;
	}
	void column(int x) {
		x_pos += 20 + x;
		interfaces.surface->SetColor(17, 17, 17, 255);
		interfaces.surface->DrawFilledRect(x_pos - 5, rpos + 26, 1, vheight - 60 + 24);
		y_pos = rpos + 25;
	}
	void checkbox(const wchar_t* name, bool* option) {
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
		interfaces.surface->SetTextFont(menu::font);
		interfaces.surface->DrawText(name, wcslen(name));
		if (IsMouseInRegion(x_pos, y_pos, 12, 12) && GetAsyncKeyState(VK_LBUTTON) & 1 && GetAsyncKeyState(VK_LBUTTON))
			*option = !(*option);
		y_pos += 15;
	}
	bool button(const wchar_t* name, vec2 pos, vec2 size) {
		interfaces.surface->SetColor(15, 15, 15, 255);
		interfaces.surface->DrawRectOutline(pos.x, pos.y, size.x, size.y);
		interfaces.surface->SetColor(27, 27, 27, 255);
		interfaces.surface->DrawRectOutline(pos.x + 1, pos.y + 1, size.x - 2, size.y - 2);
		interfaces.surface->SetColor(37, 37, 37, 255);
		interfaces.surface->DrawFilledRect(pos.x + 2, pos.y + 2, size.x - 4, size.y - 4);
		interfaces.surface->SetTextColor(255, 255, 255, 255);
		interfaces.surface->SetTextFont(menu::font);
		unsigned int u, i;
		interfaces.surface->GetTextSize(menu::font, name, u, i);
		interfaces.surface->SetTextPosition(pos.x + (size.x / 2) - u / 2, pos.y + (size.y / 2) - i / 2);
		interfaces.surface->DrawText(name, wcslen(name));
		if (IsMouseInRegion(pos.x, pos.y, size.x, size.y) && GetAsyncKeyState(VK_LBUTTON) & 1 && GetAsyncKeyState(VK_LBUTTON))
			return true;
		return false;
	}
}
void SetupFonts() {
	menu::font = interfaces.surface->CreateFont();
	interfaces.surface->SetFontGlyphs(menu::font, "Verdana", 12, 600, 0);
	menu::esp = interfaces.surface->CreateFont();
	interfaces.surface->SetFontGlyphs(menu::esp, "Tahoma", 12, 600, 0x080); // dropshadow = 0x080, antialias = 0x010, outline = 0x200
}
void RenderMenu() {
	menu::window(L"singlefile csgo internal", { 50, 50 }, { 420, 260 });
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
	menu::checkbox(L"radar", &config.visuals.m_bRadar);
	menu::checkbox(L"disable keyboard in menu", &config.misc.m_bGameKeyboard);

	if (menu::button(L"load", {60, 270}, {195, 30}))
		load();
	if (menu::button(L"save", {265, 270}, {195, 30}))
		save();
}

class CUserCmd {
private:
	unsigned char pad_0x0[0x4];
public:
	int			m_nCommandNumber;
	int			m_nTickCount;
	vec3		m_vecAngles;
	vec3		m_vecDirection;
	float		m_flForwardMove;
	float		m_flSideMove;
	float		m_flUpMove;
	int			m_nButtons;
	char		m_chImpulse;
	int			m_nWeaponSelect;
	int			m_nWeaponType;
	short		m_shSeed;
	short		m_shMouseDX;
	short		m_shMouseDY;
	bool		m_bHasBeenPredicted;
private:
	unsigned char pad_0x1[0x18];
};
bool(__stdcall* CreateMoveOriginal)(float, CUserCmd*);
void(__thiscall* PaintTraverseOriginal)(IPanel*, unsigned int, bool, bool);
bool(__thiscall* GameEventsOriginal)(IGameEventManager2*, IGameEvent*);
void(__stdcall* EmitSoundOriginal)(void*, int, int, const char*, unsigned int, const char*, float, int, int, int, int, const vec3&, const vec3&, void*, bool, float, int, void*);
int(__fastcall* SvCheatsGetIntOriginal)(void*);
LRESULT CALLBACK Wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN) {
		switch (wParam) {
		case VK_INSERT:
			menu_open = !menu_open;
			break;
		}
	}
	return CallWindowProc(orig_proc, hWnd, uMsg, wParam, lParam);
}
enum {
	IN_ATTACK = 1 << 0,
	IN_JUMP = 1 << 1,
	IN_COUNT = 1 << 26,
};
void bhop(CUserCmd* cmd) {
	if (config.misc.m_bBhop) {
		CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
		if (localplayer->GetHealth() == 0)
			return;
		int m_nMoveType = localplayer->MoveType();
		if (m_nMoveType == LADDER || m_nMoveType == NOCLIP || m_nMoveType == FLY || m_nMoveType == OBSERVER)
			return;
		if (!(localplayer->GetFlags() & ONGROUND))
			cmd->m_nButtons &= ~IN_JUMP;
	}
}
void autopistol(CUserCmd* cmd) {
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
void autoaccept(const char* sound) {
	if (strstr(sound, "UIPanorama.popup_accept_match_beep")) {
		static bool(__stdcall * SetLPReady)(const char*) = (decltype(SetLPReady))PatternScan(client_dll, "55 8B EC 83 E4 F8 8B 4D 08 BA ? ? ? ? E8 ? ? ? ? 85 C0 75 12");
		if (config.misc.m_bAutoAccept)
			SetLPReady("");
	}
}
template <typename T>
static T RelativeToAbsolute(unsigned int m_pAddress)  {
	return (T)(m_pAddress + 0x4 + *(int*)(m_pAddress));
}
struct bbox {
	int x, y, w, h;
};
bool WorldToScreen(const vec3& world, vec3& screen)
{
	matrix4x4_t& view = interfaces.engine->GetViewMatrix();
	screen.x = world.x * view[0][0] + world.y * view[0][1] + world.z * view[0][2] + view[0][3];
	screen.y = world.x * view[1][0] + world.y * view[1][1] + world.z * view[1][2] + view[1][3];
	screen.z = world.x * view[2][0] + world.y * view[2][1] + world.z * view[2][2] + view[2][3];
	float  w = world.x * view[3][0] + world.y * view[3][1] + world.z * view[3][2] + view[3][3];
	if (w < 0.1f)
		return false;
	vec3 ndc;
	ndc.x = screen.x / w;
	ndc.y = screen.y / w;
	ndc.z = screen.z / w;
	unsigned int x, y;
	interfaces.engine->GetScreenSize(x, y);
	screen.x = (x / 2 * ndc.x) + (ndc.x + x / 2);
	screen.y = -(y / 2 * ndc.y) + (ndc.y + y / 2);
	return true;
}
#define FL_MAX 3.40282e+038;
vec3 VectorTransform(vec3 in, matrix_t matrix) {
	return vec3(in.dot(matrix[0]) + matrix[0][3], in.dot(matrix[1]) + matrix[1][3], in.dot(matrix[2]) + matrix[2][3]);
}
bool getbbot(CBaseEntity* player, bbox& box) {
	matrix_t& m_rgflTransFrame = (matrix_t&)player->GetCoordinateFrame();
	const vec3 min = player->CollisonMins();
	const vec3 max = player->CollisonMaxs();
	vec3 m_vecTransScreen[8];
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
	for (int i = 0; i <= 7; i++) {
		if (!WorldToScreen(VectorTransform(points[i], m_rgflTransFrame), m_vecTransScreen[i]))
			return false;
	}
	vec3 m_vecBoxes[] = {
		m_vecTransScreen[3],
		m_vecTransScreen[5], 
		m_vecTransScreen[0], 
		m_vecTransScreen[4],
		m_vecTransScreen[2], 
		m_vecTransScreen[1],
		m_vecTransScreen[6],
		m_vecTransScreen[7] 
	};
	float m_flLeft = m_vecTransScreen[3].x, m_flBottom = m_vecTransScreen[3].y, m_flRight = m_vecTransScreen[3].x, m_flTop = m_vecTransScreen[3].y;
	for (int i = 0; i <= 7; i++) {
		if (m_flLeft > m_vecBoxes[i].x)
			m_flLeft = m_vecBoxes[i].x;
		if (m_flBottom < m_vecBoxes[i].y)
			m_flBottom = m_vecBoxes[i].y;
		if (m_flRight < m_vecBoxes[i].x)
			m_flRight = m_vecBoxes[i].x;
		if (m_flTop > m_vecBoxes[i].y)
			m_flTop = m_vecBoxes[i].y;
	}
	box.x = (int)(m_flLeft);
	box.y = (int)(m_flTop);
	box.w = (int)(m_flRight)-(int)(m_flLeft);
	box.h = (int)(m_flBottom)-(int)(m_flTop);
	return true;
}
struct rgba {
	int r, g, b, a;
	rgba(int r = 0, int g = 0, int b = 0, int a = 255) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
};
void players() {
	if (!interfaces.engine->IsInGame())
		return;
	CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	if (localplayer->GetHealth() > 0 && (config.visuals.m_bOnlyOnDead))
		return;
	for (int i = 1; i <= interfaces.engine->GetMaxClients(); i++) {
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
			unsigned int o, p;
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
				healthclr = rgba((255 - entity->GetHealth() * 2.55f), (entity->GetHealth() * 2.55f), 0, 255);
			interfaces.surface->SetColor(0, 0, 0, 255);
			interfaces.surface->DrawFilledRect(box.x - 10, box.y - 1, 5, box.h + 2);
			interfaces.surface->SetColor(healthclr.r, healthclr.g, healthclr.b, healthclr.a);
			interfaces.surface->DrawFilledRect(box.x - 9, box.y + box.h - ((box.h * (entity->GetHealth() / 100.f))), 3, (box.h * entity->GetHealth() / 100.f) + (entity->GetHealth() == 100 ? 0 : 1));
		}
		if (config.visuals.m_bRadar)
			entity->Spotted() = true;
	}
}
void cvars() {
	CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	interfaces.cvar->FindVar("mat_postprocess_enable")->SetValue(config.visuals.m_bDisablePostProcess ? 0 : 1); 
	interfaces.cvar->FindVar("cl_crosshair_recoil")->SetValue(config.misc.m_bRecoilCrosshair ? 1 : 0); // i'm sure the ? 1 : 0 doesn't matter but this feels better. /shrug
	interfaces.cvar->FindVar("weapon_debug_spread_show")->SetValue(((config.misc.m_bNoScopeCrosshair) && !localplayer->IsScoped()) ? 2 : 0);
}
int b = 0;
void speclist() {
	if (!interfaces.engine->IsInGame())
		return;
	CBaseEntity* localplayer = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	if (!localplayer)
		return;
	if (config.misc.m_bSpectatorList) {
		for (int i = 1; i <= interfaces.engine->GetMaxClients(); i++) {
			CBaseEntity* entity = interfaces.entitylist->GetEntity(i);
			if (!entity || entity->GetHealth() > 0 || !entity->GetObserverTarget())
				continue;
			if (entity->GetObserverTarget() != localplayer)
				continue;
			SPlayerInfo player;
			interfaces.engine->GetPlayerInfo(i, &player);
			interfaces.surface->SetTextColor(255, 255, 255, 255);
			interfaces.surface->SetTextFont(menu::font);
			unsigned x, y;
			interfaces.engine->GetScreenSize(x, y);
			interfaces.surface->SetTextPosition(10, 10 + b);
			wchar_t tmp[128];
			if (MultiByteToWideChar(65001, 0, player.m_szName, -1, tmp, 128)) {
				interfaces.surface->DrawText(tmp, wcslen(tmp));
			}
			b += 12;
		}
	}
	b = 0;
}
void triggerbot(CUserCmd* cmd) {
	if (!(config.aimbot.m_bTriggerbot))
		return;
	CBaseEntity* lp = interfaces.entitylist->GetEntity(interfaces.engine->GetLocalPlayer());
	if (!lp || (lp->GetHealth() < 1) || !lp->CrosshairTarget())
		return;
	CBaseEntity* target = interfaces.entitylist->GetEntity((lp->CrosshairTarget()));
	if (TriggerBotKEY && !GetAsyncKeyState(TriggerBotKEY))
		return;
	if (!target->GetHealth() || lp->FlashDuration() > 0.1f || !lp->GetWeapon()->Ammo() || lp->GetWeapon()->WeaponNextAttack() > interfaces.globals->m_flCurrentTime || (!lp->IsScoped() && lp->GetWeapon()->GetWeaponType() == 5) || lp->GetTeamNumber() == target->GetTeamNumber()) // dumbass "hitchance" calculation but it's close enough ig
		return;
	cmd->m_nButtons |= IN_ATTACK;
}
bool __stdcall _CreateMove(float m_flInputSampleTime, CUserCmd* cmd) {
	bool SetViewAngles = CreateMoveOriginal(m_flInputSampleTime, cmd);
	if (cmd->m_nCommandNumber % 4 == 1) {
		cmd->m_nButtons |= IN_COUNT;
		cvars(); // commands that do not to run each tick (i.e don't need usercmd, just dependent on localplayer & being in game)
	}
	bhop(cmd);
	autopistol(cmd);
	triggerbot(cmd);
	return SetViewAngles;
}
void __stdcall _EmitSound(void* filter, int entityIndex, int channel, const char* soundEntry, unsigned int soundEntryHash, const char* sample, float volume, int seed, int soundLevel, int flags, int pitch, const vec3& origin, const vec3& direction, void* utlVecOrigins, bool updatePositions, float soundtime, int speakerentity, void* soundParams) { // thank you danielkrupinski/Osiris for these arguments
	autoaccept(soundEntry);
	return EmitSoundOriginal(filter, entityIndex, channel, soundEntry, soundEntryHash, sample, volume, seed, soundLevel, flags, pitch, origin, direction, utlVecOrigins, updatePositions, soundtime, speakerentity, soundParams);
}
bool __stdcall _GameEvents(IGameEvent* event) {
	if (config.misc.m_bHitSound) {
		if (strstr(event->GetName(), "player_hurt")) {
			SPlayerInfo player;
			interfaces.engine->GetPlayerInfo(interfaces.engine->GetLocalPlayer(), &player);
			if (event->GetInt("attacker") == player.m_nUserID)
				interfaces.engine->ClientCmdUnrestricted("play buttons/arena_switch_press_02");
		}
	}
	return GameEventsOriginal(interfaces.events, event);
}
void __stdcall _PaintTraverse(unsigned int panel, bool m_bForceRepaint, bool m_bAllowRepaint) {
	auto drawing = fnv::hash(interfaces.panel->GetPanelName(panel));
	if (drawing == fnv::hash("MatSystemTopPanel")) {
		players();
		speclist();
		if (menu_open)
			RenderMenu();
	}
	if (drawing == fnv::hash("FocusOverlayPanel")) {
		interfaces.panel->SetInputMouseState(panel, menu_open);
		interfaces.panel->SetInputKeyboardState(panel, menu_open && (config.misc.m_bGameKeyboard));
	}
	return PaintTraverseOriginal(interfaces.panel, panel, m_bForceRepaint, m_bAllowRepaint);
}
void LoadHooks() {
	MH_Initialize();
	void* CreateMoveAddress = v<void*>(interfaces.client_mode, 24);
	void* PaintTraverseAddress = v<void*>(interfaces.panel, 41);
	void* FireGameEventsAddress = v<void*>(interfaces.events, 9);
	void* EmitSoundAddress = v<void*>(interfaces.sound, 5);
	MH_CreateHook(CreateMoveAddress, &_CreateMove, (void**)&CreateMoveOriginal);
	MH_CreateHook(PaintTraverseAddress, &_PaintTraverse, (void**)&PaintTraverseOriginal);
	MH_CreateHook(FireGameEventsAddress, &_GameEvents, (void**)&GameEventsOriginal);
	MH_CreateHook(EmitSoundAddress, &_EmitSound, (void**)&EmitSoundOriginal);
	MH_EnableHook(MH_ALL_HOOKS);
}
template <class T>
T CreateInterface(void* m_pModule, const char* m_szInterface) {
	return ((T(*)(const char*, unsigned int))GetProcAddress((HMODULE)m_pModule, "CreateInterface"))(m_szInterface, 0x0);
}
int GetLineCount();
void __stdcall Init (HMODULE mod) {
	while (!GetModuleHandleA("serverbrowser.dll"))
		Sleep(250);
	AllocConsole();
	SetConsoleTitleA("singlefile: console");
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	printf("singlefile v1.1.1: loading... (compiled with %d lines of code)\n", GetLineCount());
	csgo_window = FindWindowA("Valve001", nullptr);
	orig_proc = (WNDPROC)SetWindowLongA(csgo_window, GWLP_WNDPROC, (LONG)Wndproc);
	client_dll = GetModuleHandleA("client.dll");
	engine_dll = GetModuleHandleA("engine.dll");
	void* surface_dll = GetModuleHandleA("vguimatsurface.dll");
	void* vgui2_dll = GetModuleHandleA("vgui2.dll");
	void* vstdlib_dll = GetModuleHandleA("vstdlib.dll");
	interfaces.engine = CreateInterface<IVEngineClient*>(engine_dll, "VEngineClient014");
	if (!strstr(interfaces.engine->GetVersionString(), "1.37.8.6"))
		printf("note: you are using an unknown cs:go client version (%s). if you are expierencing crashes, you may need to update offsets. each offset in the source code has it's netvar name, or you can find it on hazedumper.\n", interfaces.engine->GetVersionString());
	interfaces.entitylist = CreateInterface<CBaseEntityList*>(client_dll, "VClientEntityList003");
	interfaces.surface = CreateInterface<CMatSystemSurface*>(surface_dll, "VGUI_Surface031");
	interfaces.panel = CreateInterface<IPanel*>(vgui2_dll, "VGUI_Panel009");
	interfaces.client = CreateInterface<IClient*>(client_dll, "VClient018");
	interfaces.cvar = CreateInterface<ICVar*>(vstdlib_dll, "VEngineCvar007");
	interfaces.events = CreateInterface<IGameEventManager2*>(engine_dll, "GAMEEVENTSMANAGER002");
	interfaces.sound = CreateInterface<ISound*>(engine_dll, "IEngineSoundClient003");
	interfaces.client_mode = **(IClientModeShared***)((*(unsigned int**)(interfaces.client))[0xA] + 0x5);
	interfaces.globals = **(CGlobalVarsBase***)((*(unsigned int**)(interfaces.client))[0xB] + 0xA);
	LoadHooks();
	SetupFonts();
	printf("finished loading.\n");
	while (!GetAsyncKeyState(VK_END))
		Sleep(500);
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}
BOOL APIENTRY DllMain(HMODULE m_hModule, DWORD m_dwReason, LPVOID m_pReserved) {
	DisableThreadLibraryCalls(m_hModule);
	if (m_dwReason == DLL_PROCESS_ATTACH)
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Init, m_hModule, 0, NULL);
	return TRUE;
}
int GetLineCount() { // must be at bottom obviously :P
	return (__LINE__ + 1);
}
