#pragma once

#include "gui.h"

#include "ImGuiPresets.h"

#include "DishonoredCheat.h"

#include <map>
#include <unordered_map>
#include <stdexcept>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void gui::SaveUserData() noexcept
{
	// 写入到文件
	for (auto bind : keyBinds)
	{
		userDataIni[iniKeyBindsSection][bind.first] = bind.second;
	}
	userDataIni.save(iniFileName);
}

bool gui::SetupWindowClass(const char* WindowClassName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = NULL;
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = WindowClassName;
	windowClass.hIconSm = NULL;

	if (!RegisterClassEx(&windowClass))
	{
		return false;
	}

	return true;
}

void gui::DestroyWindowClass() noexcept
{
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::SetupWindow(const char* WindowName) noexcept
{
	window = CreateWindow(windowClass.lpszClassName, WindowName, WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, 0, 0, windowClass.hInstance, 0);

	if (!window)
	{
		return false;
	}

	return true;
}

void gui::DestroyWindow() noexcept
{
	if (window)
	{
		DestroyWindow(window);
	}
}

bool gui::SetupDirectX() noexcept
{
	const auto handle = GetModuleHandle("d3d9.dll");

	if (!handle)
	{
		return false;
	}

	using CreateFn = LPDIRECT3D9(__stdcall*)(UINT);
	const auto create = reinterpret_cast<CreateFn>(GetProcAddress(handle, "Direct3DCreate9"));

	if (!create)
	{
		return false;
	}

	d3d9 = create(D3D_SDK_VERSION);

	if (!d3d9)
	{
		return false;
	}

	D3DPRESENT_PARAMETERS params = {};
	params.BackBufferWidth = 0;
	params.BackBufferHeight = 0;
	params.BackBufferFormat = D3DFMT_UNKNOWN;
	params.BackBufferCount = 0;
	params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.MultiSampleQuality = NULL;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.Windowed = 1;
	params.EnableAutoDepthStencil = 0;
	params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	params.Flags = NULL;
	params.FullScreen_RefreshRateInHz = 0;
	params.PresentationInterval = 0;

	if (d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &device) < 0)
	{
		return false;
	}

	return true;
}

void gui::DestroyDirectX() noexcept
{
	if (device)
	{
		device->Release();
		device = NULL;
	}

	if (d3d9) {
		d3d9->Release();
		d3d9 = NULL;
	}
}

void gui::Setup()
{
	if (!SetupWindowClass("InternalHackWindowClass"))
	{
		throw std::runtime_error("Failed to create WindowClass!");
	}

	if (!SetupWindow("InternalHackWindow"))
	{
		throw std::runtime_error("Failed to create Window!");
	}

	if (!SetupDirectX())
	{
		throw std::runtime_error("Failed to create device!");
	}

	DestroyWindow();
	DestroyWindowClass();
}

void gui::SetupImGuiMenu(LPDIRECT3DDEVICE9 Device) noexcept
{
	auto params = D3DDEVICE_CREATION_PARAMETERS{};

	Device->GetCreationParameters(&params);

	window = params.hFocusWindow;

	originalWindowProcess = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcess)));

	ImGui::CreateContext();
	ImGui::StyleColorsLight();
	
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(Device);

	bIsSetup = true;
}

void gui::DestroyImGuiMenu() noexcept
{
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX9_Shutdown();
	ImGui::DestroyContext();

	// restore wnd proc
	SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWindowProcess));

	DestroyDirectX();
}

void gui::RenderImGui() noexcept
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// draw menu
	ImGuiContent();

	ImGuiCursor();

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void gui::ImGuiContent() noexcept
{
	// optionsBool
	enum {
		invis, infHealth, infMana, infOxy, infHealthElixir, infManaElixir, infDarkVision,
		blinkMarker, noBlinkCooldown, blinkHeightIncrease, infTimeBend, infPossession, infClip, infAmmo
	};

	// otherBool
	enum {
		gameHooked, showBinds, lastState, showStyleEditor
	};

	// predefining some stuff to simplify creation of gui
	const char* buttonIds[] = { "-5", "-1", "+1", "+5" };
	const int buttonVals[] = { -5, -1, +1, +5 };

	const char* elixirs[] = { "Health Elixir", "Mana Elixir" };
	const char* equipment[] = { "Bullets", "Explosive Bullets", "Crossbow Bolts", "CrossBow Sleep Darts",
								"Crossbow Incendiary Bolts", "Spring Razors", "Grenades", "Sticky Grenades" };

	static const char* successHook = "Successfully Hooked Game";
	static const char* failureHook = "Hook Failed";

	static DishonoredCheat* cheatInstance;

	const char* options[14] = { "Invisibility", "Infinite Health", "Infinite Mana", "Infinite Oxygen", "Unlimited Health Elixirs",
								"Unlimited Mana Elixirs", "Infinite Dark Vision", "Leave Blink Destination", "No Blink Cooldown", "Increase Blink Height",
								"Infinite Bend Time", "Infinite Possession", "Infinite Clip", "Unlimited Ammo" };
	const int optionsSize = 14;

	static bool optionStates[14] = { false, false, false, false, false , false , false,
				false, false, false, false, false, false };

	static bool otherStates[4] = { false, false, false, false };

	// Load key binds
	static bool once = false;
	if (!once) 
	{
		userDataIni.load(iniFileName);

		for (auto option : options) 
		{
			if (userDataIni.contains(iniKeyBindsSection) && userDataIni[iniKeyBindsSection].contains(option))
			{
				int key = userDataIni[iniKeyBindsSection][option].as<int>();
				keyBinds[option] = key;
				showKeyBinds[option] = (char*)ImGui::GetKeyName((ImGuiKey)key);
				continue;
			}
			keyBinds[option] = '\0';
			showKeyBinds[option] = (char*)"Not Bound";
		}
	}
	once = true;

	// Button Binds
	if (!otherStates[showBinds] && !otherStates[lastState])
	{
		for (int i = 0; i < optionsSize; i++)
		{
			if (ImGui::IsKeyPressed((ImGuiKey)(keyBinds[options[i]]), false))
			{
				optionStates[i] = !optionStates[i];
			}
		}
	}

	static float curX, curY, curZ;
	static float posXInput;
	static float posYInput;
	static float posZInput;
	static float posX, posY, posZ;

	if (otherStates[showStyleEditor])
	{
		ImGui::SetNextWindowCollapsed(!bIsOpen);
		ImGui::Begin("Dear ImGui Style Editor", &otherStates[showStyleEditor]);
		ImGui::ShowStyleEditor(&ImGui::GetStyle());
		ImGui::End();
	}

	if (otherStates[showBinds]) {
		ImGui::SetNextWindowCollapsed(!bIsOpen);

		ImGui::Begin("Binds Window", &exit, bIsOpen ? ImGuiPresets::Default : ImGuiPresets::DisplayOnly);
		static bool bindingPage = false;
		static int type = NULL;
		for (int i = 0; i < optionsSize; i++) {
			if (ImGui::Button(showKeyBinds[options[i]], ImVec2(130.f, 18))) {
				bindingPage = true;
				type = i;
			}
			ImGui::PushID("Bind");
			ImGui::SameLine(140.f);
			if (ImGui::Button("Unbind", ImVec2(50.f, 18))) {
				keyBinds[options[i]] = '\0';
				showKeyBinds[options[i]] = (char*)"Not Bound";
			}
			ImGui::PushID("Unbind");
			ImGui::SameLine();
			ImGui::Text(": ");
			ImGui::SameLine();
			ImGui::Text(options[i]);

		}
		if (bindingPage) {
			//ImGui::SetNextWindowPos({ 0, 0 });
			//ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
			ImGui::Begin("Awaiting Keyboard Input", &exit, bIsOpen ? ImGuiPresets::Default : ImGuiPresets::DisplayOnly);
			ImGui::Text("Awaiting Keyboard Input");
			if (ImGui::Button("Cancel")) {
				bindingPage = false;
				type = NULL;
			}
			for (int j = ImGuiKey_NamedKey_BEGIN; j <= ImGuiKey_GamepadRStickDown; j++) {
				if (ImGui::IsKeyPressed((ImGuiKey)j)) 
				{
					showKeyBinds[options[type]] = (char*)ImGui::GetKeyName((ImGuiKey)j);
					keyBinds[options[type]] = (ImGuiKey)j;
					bindingPage = false;
					type = NULL;
				}
			}
			ImGui::End();
		}
		if (ImGui::Button("Close")) {
			otherStates[showBinds] = false;
			SaveUserData();
		}
		ImGui::End();
	}
	
	// Cheats start
	//ImGui::SetNextWindowCollapsed(!bIsOpen);
	ImGui::SetNextWindowPos(bIsOpen ? ImVec2(0, 0) : ImGui::GetMainViewport()->Size);

	ImGui::Begin("Dishonored Cheat", &exit, bIsOpen ? ImGuiPresets::Default : ImGuiPresets::DisplayOnly);

	// This makes sure the pointers are up-to date
	if (otherStates[gameHooked])
		cheatInstance->SyncPointers();

	if (!cheatInstance)
		cheatInstance = new DishonoredCheat();

	if (cheatInstance->IsHooked())
		otherStates[gameHooked] = true;
	else
		otherStates[gameHooked] = false;

	ImGui::SeparatorText("Insert : Toggle UI");

	if (ImGui::Button("Bind Buttons")) {
		otherStates[showBinds] = !otherStates[showBinds];
		if (!otherStates[showBinds])
		{
			SaveUserData();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Change Style")) {
		otherStates[showStyleEditor] = !otherStates[showStyleEditor];
	}

	// Checkmark look for all checkmarks
	for (int i = 0; i < optionsSize; i++) {
		if (i == 0)
		{
			ImGui::SeparatorText("Player");
		}
		if (i == infDarkVision)
		{
			ImGui::SeparatorText("Abilities");
		}
		if (i == infClip)
		{
			ImGui::SeparatorText("Ammo");
		}
		ImGui::Checkbox(options[i], &optionStates[i]);
		if (optionStates[i] && otherStates[gameHooked])
		{
			cheatInstance->ActivateCheat(i);
		}
		if (i == invis || i == infHealth || i == blinkMarker || i == infClip)
		{
			if (!optionStates[i] && otherStates[gameHooked])
			{
				cheatInstance->RepairCheat(i);
			}
		}
	}

	// Ammo GUI section
	for (int i = 0; i < 7; i++) {
		ImGui::Text(equipment[i]);
		for (int j = 0; j < 4; j++) {
			ImGui::SameLine();
			if (ImGui::Button(buttonIds[j]) && otherStates[gameHooked]) {
				cheatInstance->IncreaseAmmoCount(i, buttonVals[j]);
			}
			ImGui::PushID(buttonIds[j]);
		}
	}

	// Resources GUI section
	ImGui::SeparatorText("Resources");
	for (int i = 0; i < 2; i++) {
		ImGui::Text(elixirs[i]);
		for (int j = 0; j < 4; j++) {
			ImGui::SameLine();
			if (ImGui::Button(buttonIds[j]) && otherStates[gameHooked]) {
				cheatInstance->IncreaseElixir(i, buttonVals[j]);
			}
			ImGui::PushID(buttonIds[j]);
		}
	}

	ImGui::Text("Gold / Runes / Rewires");
	for (int j = 0; j < 4; j++) {
		ImGui::SameLine();
		if (ImGui::Button(buttonIds[j]) && otherStates[gameHooked]) {
			cheatInstance->IncreaseResourceCount(buttonVals[j]);
		}
		ImGui::PushID(buttonIds[j]);
	}

	otherStates[lastState] = false;

	for (int i = 0; i < optionsSize; i++)
		if (GetAsyncKeyState(keyBinds[options[i]])) {
			otherStates[lastState] = true;
			break;
		}

	// Players GUI section
	ImGui::SeparatorText("Player");
	if (cheatInstance) {
		ImGui::Text("Current Coords");
		ImGui::Text("X: %f", cheatInstance->GetX());
		ImGui::SameLine(150.f);
		if (ImGui::Button("Copy X"))
		{
			posXInput = cheatInstance->GetX();
		}
		ImGui::Text("Y: %f", cheatInstance->GetY());
		ImGui::SameLine(150.f);
		if (ImGui::Button("Copy Y"))
		{
			posYInput = cheatInstance->GetY();
		}
		ImGui::Text("Z: %f", cheatInstance->GetZ());
		ImGui::SameLine(150.f);
		if (ImGui::Button("Copy Z"))
		{
			posZInput = cheatInstance->GetZ();
		}
	}

	// teleportation
	ImGui::Text("Teleport to Coords");
	ImGui::PushItemWidth(100.f);
	ImGui::DragFloat("x", &posXInput);
	ImGui::SameLine();
	ImGui::PushItemWidth(100.f);
	ImGui::DragFloat("y", &posYInput);
	ImGui::SameLine();
	ImGui::PushItemWidth(100.f);
	ImGui::DragFloat("z", &posZInput);

	if (ImGui::Button("Teleport") && otherStates[gameHooked]) {
		if (!cheatInstance->TeleportToCoords(posXInput, posYInput, posZInput) && !cheatInstance->IsHooked()) {
			otherStates[gameHooked] = false;
		}
	}

	// This displays memory slots for the client, player, inventory
	ImGui::SeparatorText("Process");
	if (cheatInstance) {
		ImGui::Text("Client: %x", cheatInstance->getClientAddress());
		ImGui::Text("Player: %x", cheatInstance->getPlayerAddress());
		ImGui::Text("Abilities: %x", cheatInstance->getAbilitiesAddress());
		ImGui::Text("Inventory: %x", cheatInstance->getInventoryAddress());
		ImGui::Text("Assets: %x", cheatInstance->getAssets());
	}

	// Cheats end
	ImGui::End();
}

void gui::ImGuiCursor() noexcept
{
	ImGui::SetMouseCursor(ImGuiMouseCursor_None);

	if (!gui::bIsOpen)
	{
		return;
	}
	auto mousePos = ImGui::GetMousePos();
	auto cursorTip1 = ImVec2(mousePos.x + 0.f, mousePos.y + 26.f);
	auto cursorTip2 = ImVec2(mousePos.x + 20.f, mousePos.y + 18.f);
	auto cursorTip3 = ImVec2(cursorTip2.x - 13.f, cursorTip2.y - 3.f);
	ImVec2 cursorShape[] = {cursorTip1, mousePos, cursorTip2, cursorTip3};
	ImGui::GetForegroundDrawList()->AddPolyline(cursorShape, 4, ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.9f, 0.9f, .8f)), ImGuiPresets::Cursor, 7.f);
	ImGui::GetForegroundDrawList()->AddPolyline(cursorShape, 4, ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.2f, 0.24f, 1.f)), ImGuiPresets::Cursor, 5.f);
}

LRESULT CALLBACK WindowProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// toggle
	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		gui::bIsOpen = !gui::bIsOpen;
	}

	//if (gui::bIsOpen && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return 1L;
	}

	return CallWindowProc(gui::originalWindowProcess, hWnd, msg, wParam, lParam);
}