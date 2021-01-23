#define NOMINMAX
#include <Windows.h>
#include <chrono>
#include <thread>

#include "settings/config.h"
#include "settings/globals.h"
#include "settings/options.hpp"

#include "helpers/input.h"
#include "helpers/utils.h"
#include "helpers/console.h"
#include "helpers/notifies.h"

#include "hooks/hooks.h"
#include "render/render.h"
#include "valve_sdk/sdk.hpp"
#include "features/features.h"
#include "helpers/imdraw.h"
#include "valve_sdk/netvars.hpp"

bool is_unhookable = true;

void wait_for_modules()
{
	auto modules = std::vector<std::string>
	{
		xorstr_("engine.dll"),
		xorstr_("shaderapidx9.dll"),
		xorstr_("serverbrowser.dll"),
		xorstr_("materialsystem.dll"),
		xorstr_("client.dll"),
	};

	for (auto& module : modules)
		while (!utils::get_module(module))
			LI_FN(Sleep).cached()(10);
}

void setup_hotkeys(LPVOID base)
{
	input_system::register_hotkey(VK_INSERT, []()
		{
			render::menu::toggle();

			render::switch_hwnd();
		});

	if (is_unhookable)
	{
		bool is_active = true;
		input_system::register_hotkey(VK_DELETE, [&is_active]()
			{
				hooks::destroy();
				if (render::menu::is_visible())
				{
					render::menu::toggle();
					render::switch_hwnd();
				}
				is_active = false;
			});

		while (is_active)
			Sleep(500);

		LI_FN(FreeLibraryAndExitThread)(static_cast<HMODULE>(base), 1);
	}
}

DWORD __stdcall on_attach(LPVOID base)
{
	wait_for_modules();

#ifdef _DEBUG
	console::attach();
#endif
	g::initialize();
	input_system::initialize();
	render::initialize();
	hooks::init();
	//netvar_manager::get(); //Uncomment this to dump netvars.
	skins::load_statrack();
	skins::initialize_kits();
	skins::load();
	globals::load();

	config::cache("configs");

	notifies::push("Injection Succesful", notify_state_s::debug_state);

	/*g::cvar->ConsoleColorPrintf(Color::Green, "This is ConsoleColorPrintf\n\n"); //TODO: Do console log system.
	g::cvar->ConsolePrintf("This is ConsolePrintf\n\n");
	g::cvar->ConsoleDPrintf("This is ConsolDPrintf\n\n");*/

	setup_hotkeys(base);

	return TRUE;
}

void on_detach()
{
#ifdef _DEBUG
	console::detach();
#endif
	render::destroy();
	hooks::destroy();
	input_system::destroy();
}

BOOL __stdcall DllMain(_In_ HINSTANCE instance, _In_ DWORD fdwReason, _In_opt_ LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		if (instance)
			LI_FN(DisableThreadLibraryCalls)(instance);

		if (is_unhookable = (strstr(GetCommandLineA(), "-insecure") || !utils::get_module(xorstr_("serverbrowser.dll"))))
			LI_FN(CreateThread)(nullptr, 0, on_attach, instance, 0, nullptr);
		else
			on_attach(instance);
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
		on_detach();

	return TRUE;
}