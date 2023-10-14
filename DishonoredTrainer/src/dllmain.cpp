// dllmain.cpp : Defines the entry point for the DLL application.
#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>
#include <thread>
#include <cstdint>

#include <iostream>
#include <fstream>

#include "hooks.h"

void Setup(const HMODULE hmodule)
{
	try
	{
		gui::Setup();
		hooks::Setup();
	}
	catch (const std::exception& error)
	{
		MessageBeep(MB_ICONERROR);
		MessageBox(0, error.what(), "Hack error!", MB_OK | MB_ICONEXCLAMATION);
		goto UNLOAD;
	}

	//while (!GetAsyncKeyState(VK_END))
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(20));
	}

UNLOAD:
	hooks::Destroy();
	gui::DestroyImGuiMenu();
	FreeLibraryAndExitThread(hmodule, 0);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);

		auto thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Setup), hModule, 0, nullptr);

		if (thread)
		{
			CloseHandle(thread);
		}

		//MessageBox(0, TEXT("ASI Loader works correctly."), TEXT("ASI Loader Test Plugin"), MB_ICONWARNING);
	}

    return TRUE;
}

