#pragma once

#include "../ThirdParty/imgui/imgui.h"

namespace ImGuiPresets
{
	// Window Style
	auto Default = ImGuiWindowFlags_None;
	auto ShowTitle = ImGuiWindowFlags_NoCollapse;
	auto DisplayOnly = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	auto NoTitle = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	// Shape Draw Flags
	auto Cursor = ImDrawFlags_Closed;
};
