#include "hooks.h"

#include <stdexcept>
#include <intrin.h>

#include "../ThirdParty/minhook/MinHook.h"

#include "../ThirdParty/imgui/imgui.h"
#include "../ThirdParty/imgui/imgui_impl_dx9.h"
#include "../ThirdParty/imgui/imgui_impl_win32.h"


void hooks::Setup()
{
	if (MH_Initialize())
	{
		throw std::runtime_error("MH_Initialize failed!");
	}

	if (MH_CreateHook(VirtualFunction(gui::device, 16), &Reset, reinterpret_cast<void**>(&ResetOriginal)))
	{
		throw std::runtime_error("MH_CreateHook reset failed!");
	}

	if (MH_CreateHook(VirtualFunction(gui::device, 42), &EndScene, reinterpret_cast<void**>(&EndSceneOriginal)))
	{
		throw std::runtime_error("MH_CreateHook endscene failed!");
	}

	if (MH_EnableHook(MH_ALL_HOOKS))
	{
		throw std::runtime_error("MH_EnableHook failed!");
	}

	gui::DestroyDirectX();
}

void hooks::Destroy() noexcept
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

long __stdcall hooks::EndScene(IDirect3DDevice9* device) noexcept
{
	static const auto returnAddress = _ReturnAddress();

	const auto result = EndSceneOriginal(device, device);

	if (_ReturnAddress() == returnAddress)
	{
		return result;
	}

	if (!gui::bIsSetup)
	{
		gui::SetupImGuiMenu(device);
	}

	gui::RenderImGui();

	return result;
}

HRESULT __stdcall hooks::Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = ResetOriginal(device, device, params);
	ImGui_ImplDX9_CreateDeviceObjects();

	return result;
}
