#pragma once

#include <d3d9.h>
//#include <d3d11.h>

#include "inicpp.h"

#include "../ThirdParty/imgui/imgui.h"
#include "../ThirdParty/imgui/imgui_internal.h"
#include "../ThirdParty/imgui/imgui_impl_dx9.h"
#include "../ThirdParty/imgui/imgui_impl_win32.h"

namespace gui
{
	// Settings
	inline ini::IniFile userDataIni;
	static std::string iniFileName = "UserData.ini";
	static std::string iniKeyBindsSection = "Key Binds";

	// Cheats
	inline std::map<const char*, int> keyBinds;
	inline std::map<const char*, char*> showKeyBinds;

	void SaveUserData() noexcept;

	// Common stuff
	inline bool exit = true;

	inline bool bIsOpen = true;
	inline bool bIsSetup = false;

	inline HWND window = nullptr;
	inline WNDCLASSEX windowClass = {};
	inline WNDPROC originalWindowProcess = nullptr;

	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline LPDIRECT3D9 d3d9 = nullptr;

	bool SetupWindowClass(const char* WindowClassName) noexcept;
	void DestroyWindowClass() noexcept;

	bool SetupWindow(const char* WindowName) noexcept;
	void DestroyWindow() noexcept;

	bool SetupDirectX() noexcept;
	void DestroyDirectX() noexcept;

	void Setup();

	void SetupImGuiMenu(LPDIRECT3DDEVICE9 Device) noexcept;
	void DestroyImGuiMenu() noexcept;

	void RenderImGui() noexcept;
	void ImGuiContent() noexcept;
	void ImGuiCursor() noexcept;
}