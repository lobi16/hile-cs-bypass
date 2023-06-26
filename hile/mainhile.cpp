/*
*	OXWARE developed by oxiKKK
*	Copyright (c) 2023
* 
*	This program is licensed under the MIT license. By downloading, copying, 
*	installing or using this software you agree to this license.
*
*	License Agreement
*
*	Permission is hereby granted, free of charge, to any person obtaining a 
*	copy of this software and associated documentation files (the "Software"), 
*	to deal in the Software without restriction, including without limitation 
*	the rights to use, copy, modify, merge, publish, distribute, sublicense, 
*	and/or sell copies of the Software, and to permit persons to whom the 
*	Software is furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included 
*	in all copies or substantial portions of the Software. 
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
*	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
*	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
*	IN THE SOFTWARE.
*/

#include "precompiled.h"

// UI
IGUIWidgets* g_gui_widgets_i = nullptr;
IGUIFontManager* g_gui_fontmgr_i = nullptr;
IImGUIPlatformLayer* g_imgui_platform_layer_i = nullptr;
IGUIWindowRendering* g_gui_window_rendering_i = nullptr;
IGUIThemeManager* g_gui_thememgr_i = nullptr;

// Util
IImportBank* g_importbank_i = nullptr;
IRegistry* g_registry_i = nullptr;
IFileSystem* g_filesystem_i = nullptr;
IUserInput* g_user_input_i = nullptr;
IWindowMsgHandler* g_window_msg_handler_i = nullptr;
IConfigManager* g_config_mgr_i = nullptr;
IAppDataManager* g_appdata_mgr_i = nullptr;
IVariableManager* g_variablemgr_i = nullptr;
ICodePerfProfiler* g_code_perf_profiler_i = nullptr;
IBytePatternBank* g_bytepattern_bank_i = nullptr;
IBindManager* g_bindmgr_i = nullptr;

BaseCommand unload_cheat(
	"unload_cheat", "Unloads the cheat. Same as unloading the cheat through menu.",
	[&](BaseCommand* cmd, const CmdArgs& args)
	{
		CoXWARE::the().end_cheat_execution();
	}
);

VarBoolean debug("debug", "Enables debug mode", false);

bool CoXWARE::run(injector_information_package_t* ifp)
{
	m_ifp = ifp;

	if (!initialize())
	{
		// if we have loaded some modules, we have to free all of them
		shutdown();
		return false;
	}

	// tell injector we've initialized.
	CInjectedDllIPCLayerClient::the().write_code(C2I_Init_OK);

	while (run_frame()) {}

	m_unloading_cheat = true;

	shutdown();

	// if this is true, some other thread will write this code nonetheless.
	if (!m_dont_write_unloading_code)
	{
		// tell injector we're done destroying and we can unload
		CInjectedDllIPCLayerClient::the().write_code(C2I_Unloading);
	}

	// don't call anything here. The injector is unloading us here.

	return true;
}

bool CoXWARE::initialize_phase2()
{
	if (!initialize_hook_managers())
	{
		return false;
	}

	g_variablemgr_i->provide_hl_execute_cmd_pfn((m_hl_execute_cmd_pfn_t)CMemoryHookMgr::the().cl_enginefuncs()->pfnClientCmd);
	
	CFeatureManager::the().initialize();

	CModelChams::the().initialize();

#ifdef OX_ENABLE_CODE_PROFILE
	g_code_perf_profiler_i->register_profile_report(&g_module_profile_report);
#endif

	CConsole::the().info("Cheat module fully initialized.");
	m_fully_initialized = true;
	return true;
}

const wchar_t* CoXWARE::get_engine_fs_module_name() const
{
	uintptr_t module = g_libloader_i->get_target_loaded_dll_base_address(L"filesystem_stdio.dll");
	if (module != NULL)
	{
		return L"filesystem_stdio.dll";
	}

	return L"filesystem_steam.dll";
}

bool CoXWARE::initialize()
{
	CConsole::the().initialize(EOutputModule::CHEAT, g_devconsole_i);

	CInjectedDllIPCLayerClient::the().establish_communication(m_ifp->m_ipc_block_ptr);

	if (get_injection_technique() == INJECT_MANUALMAP)
	{
		// if we're manually mapped module, we need to provide the libraryloader interface our
		// exported functions / interfaces, so that other modules can load them.
		g_libloader_i->register_manualmapped_module(WMODULE_CHEAT, get_cheat_dll_base_address());
	}

	// load modules
	if (!load_and_initialize_dependencies())
	{
		CInjectedDllIPCLayerClient::the().report_error("Couldn't load and initialize dependencies.");
		return false;
	}

	if (!g_appdata_mgr_i->initialize_directory_hirearchy())
	{
		CInjectedDllIPCLayerClient::the().report_error("Couldn't initialize directory hirearchy.");
		return false;
	}

	g_devconsole_i->enable_tooltip(true);

	// keyboard/mouse I/O
	g_user_input_i->initialize();

	g_bindmgr_i->initialize();
	g_in_commands_i->initialize();

	// call all interface initialization routines that provide load&export callbacks before config manager initializes, 
	// because otherwise those callbacks wouldn't be provided and the cfgmgr would load configs without these callbacks.
	g_config_mgr_i->initialize();

	check_for_clientside_protectors();

	// see for the renderer - before hooks! (because of hw.dll may missing, and we need it inside hook managers.)
	if (!is_hardware())
	{
		return false;
	}

	// see for engine build
	if (!validate_engine_build())
	{
		return false;
	}

	if (!g_bytepattern_bank_i->initialize(m_gs_build_number))
	{
		return false;
	}

	// see if we're acutally cstrike
	if (!is_valid_game())
	{
		return false;
	}

	//
	// We, as a cheat dll, are reliant on the game. Hence, we need to wait till the game fully initializes.
	// We do this by loading our module first, then initialize game-independent subsystems for the module, and
	// then we hook the host_initialized globalvar and then we wait for the engine to execute it. After it is executed, 
	// the cheat initialization continues for subsystems that are reliant on the game. (the main cheat initialization
	// starts there..)
	// 
	// For example the engine dll loads up client.dll or vgui2.dll, but since our module is loaded before that, 
	// we cannot hook from those etc. So we need to wait for the engine to initialize.
	//

	auto& host_initialized = CMemoryHookMgr::the().host_initialized();
	if (!host_initialized.install())
	{
		CInjectedDllIPCLayerClient::the().report_error("Fatal error, couldn't find the 'host_initialized' crucial global variable.");
		return false;
	}

	int tries = 0;
	static int tries_max = 20;
	static const auto wait_time = 0.5s;
	while (!*host_initialized.get())
	{
		CConsole::the().warning("Waiting for the engine to initialize...{}", tries != 0 ? std::format(" ({}x)", tries) : "");

		std::this_thread::sleep_for(wait_time);
		if (++tries >= tries_max) // don't run this infinitely
		{
			CInjectedDllIPCLayerClient::the().report_error("Fatal error, tried {}x but the engine didn't initialize after launching.", tries_max);
			return false;
		}
	}

	initialize_phase2();

	return true;
}

void CoXWARE::shutdown()
{
	CConsole::the().info("Shutting down cheat module subsystems...");

	if (g_config_mgr_i)
	{
		g_config_mgr_i->shutdown();
	}

	CFeatureManager::the().shutdown();

	CIngameScreenRendering::the().shutdown();

	CEnvironmentalEffects::the().shutdown();

	CEngineSoundPlayer::the().stop_all_ambient_sounds();

	COxWareUI::the().destroy();

	CGoldSrcCommandMgr::the().shutdown();

	shutdown_hook_managers();

	unload_dependencies();

	m_shutted_down = true;
}

bool CoXWARE::load_and_initialize_dependencies()
{
	assert(get_injection_technique() != INJECT_UNINITIALIZED); // just sanity checks
	ELoadType type = (get_injection_technique() == INJECT_MANUALMAP) ? LOADTYPE_MANUALMAP : LOADTYPE_NATIVE;

	auto loader_path = FilePath_t(m_ifp->m_loader_path);

	// Util
	if (!CDependencyLoader::the().load_and_initialize_module(type, loader_path, WMODULE_UTIL, [&](const auto& mod)
	{
		g_importbank_i = mod.get_interface<IImportBank>(IIMPORTBANK_INTERFACEID);
		g_registry_i = mod.get_interface<IRegistry>(IREGISTRY_INTERFACEID);
		g_filesystem_i = mod.get_interface<IFileSystem>(IFILESYSTEM_INTERFACEID);
		g_user_input_i = mod.get_interface<IUserInput>(IUSERINPUT_INTERFACEID);
		g_window_msg_handler_i = mod.get_interface<IWindowMsgHandler>(IWINDOWMSGHANDLER_INTERFACEID);
		g_config_mgr_i = mod.get_interface<IConfigManager>(ICONFIGMANAGER_INTERFACEID);
		g_appdata_mgr_i = mod.get_interface<IAppDataManager>(IAPPDATAMANAGER_INTERFACEID);
		g_variablemgr_i = mod.get_interface<IVariableManager>(IVARIABLEMANAGER_INTERFACEID);
		g_code_perf_profiler_i = mod.get_interface<ICodePerfProfiler>(ICODEPERFPROFILER_INTERFACEID);
		g_bytepattern_bank_i = mod.get_interface<IBytePatternBank>(IBYTEPATTERNBANK_INTERFACEID);
		g_bindmgr_i = mod.get_interface<IBindManager>(IBINDMANAGER_INTERFACEID);

		return g_importbank_i && g_registry_i && g_filesystem_i && g_user_input_i && g_window_msg_handler_i && g_config_mgr_i &&
			g_appdata_mgr_i && g_variablemgr_i && g_code_perf_profiler_i && g_bytepattern_bank_i && g_bindmgr_i;
	}))
	{
		return false;
	}

	// UI
	if (!CDependencyLoader::the().load_and_initialize_module(type, loader_path, WMODULE_GUI, [&](const auto& mod)
	{
		g_gui_widgets_i = mod.get_interface<IGUIWidgets>(IGUIWIDGETS_INTERFACEID);
		g_gui_fontmgr_i = mod.get_interface<IGUIFontManager>(IGUIFONTMANAGER_INTERFACEID);
		g_imgui_platform_layer_i = mod.get_interface<IImGUIPlatformLayer>(IIMGUIPLATFORMLAYER_INTERFACEID);
		g_gui_window_rendering_i = mod.get_interface<IGUIWindowRendering>(IGUIWINDOWRENDERING_INTERFACEID);
		g_gui_thememgr_i = mod.get_interface<IGUIThemeManager>(IGUITHEMEMANAGER_INTERFACEID);

		return g_gui_widgets_i && g_gui_fontmgr_i && g_imgui_platform_layer_i && g_gui_window_rendering_i && g_gui_thememgr_i;
	}))
	{
		return false;
	}

	g_variablemgr_i->register_variables_and_commands_per_module(&g_static_variable_container, &g_static_command_container, MODULE_CHEAT);

	// after all modules have been loaded
	if (!g_variablemgr_i->initialize())
	{
		return false;
	}

	return true;
}

bool CoXWARE::run_frame()
{
	if (!can_update_frame())
	{
		return true;
	}
	else
	{
		m_main_frame_update_ms = GetTickCount();
	}

	// keep communication with the injector on
	if (CInjectedDllIPCLayerClient::the().dispatch() != k_IPCLayerStatus_Ok)
	{
		return false;
	}

	// someone requested to close the cheat.
	if (is_exit_requested())
	{
		return false;
	}

	//-------------------------------------------------------------------------
	// Update code

#ifdef OX_ENABLE_CODE_PROFILE
	g_code_perf_profiler_i->update();
#endif

	g_config_mgr_i->update();
	
	if (!CDependencyLoader::the().update())
	{
		return false;
	}

#if 0
	if (CMemoryFnHookMgr::the().VGuiWrap2_ConPrintf().is_installed())
	{
		CMemoryFnHookMgr::the().VGuiWrap2_ConPrintf().call("tick\n");
	}
#endif

	return true;
}

void CoXWARE::unload_dependencies()
{
	if (g_registry_i)
	{
		g_registry_i->shutdown();
	}

	CConsole::the().destroy();

	CDependencyLoader::the().unload_all();

	// At this point all of the interfaces we obtained are gone.
	g_gui_widgets_i = nullptr;
	g_gui_fontmgr_i = nullptr;
	g_imgui_platform_layer_i = nullptr;
	g_gui_window_rendering_i = nullptr;
	g_gui_thememgr_i = nullptr;

	// Util
	g_importbank_i = nullptr;
	g_registry_i = nullptr;
	g_filesystem_i = nullptr;
	g_user_input_i = nullptr;
	g_window_msg_handler_i = nullptr;
	g_config_mgr_i = nullptr;
	g_appdata_mgr_i = nullptr;
	g_variablemgr_i = nullptr;
	g_code_perf_profiler_i = nullptr;
	g_bytepattern_bank_i = nullptr;
	g_bindmgr_i = nullptr;
}

bool CoXWARE::initialize_hook_managers()
{
	// security module hook
	if (!CSecurityModuleHook::the().install_hooks())
	{
		CConsole::the().error("Failed to install security module hooks.");
		return false;
	}

	// memory hook
	if (!CMemoryHookMgr::the().install_hooks())
	{
		CConsole::the().error("Failed to install hooks.");
		return false;
	}

	CVideoModeUtil::the().initialize();

	CGoldSrcCommandMgr::the().initialize();

	// HL interface hook
	if (!CHLInterfaceHook::the().install_hooks())
	{
		CConsole::the().error("Failed to install HL interface hooks.");
		return false;
	}

	// memory function hook
	if (!CMemoryFnHookMgr::the().install_hooks())
	{
		CConsole::the().error("Failed to install function hooks.");
		return false;
	}

	// memory function detour
	if (!CMemoryFnDetourMgr::the().install_hooks())
	{
		CConsole::the().error("Failed to install detour hooks.");
		return false;
	}

	// CBase clientside class hook
	if (!CMemoryHookCBaseStuff::the().install_hooks())
	{
		CConsole::the().error("Failed to install CBase stuff hooks.");
		return false;
	}

	// HLNetMessageIO hooks
	if (!CHLNetMessageIO::the().install_hooks())
	{
		CConsole::the().error("Failed to install NetMessage hooks.");
		return false;
	}

	// SVC hook
	if (!CSVCFuncDetourMgr::the().install_hooks())
	{
		CConsole::the().error("Failed to install SVC hooks.");
		return false;
	}

	// UserMSG hook
	if (!CUserMSGDetourMgr::the().install_hooks())
	{
		CConsole::the().error("Failed to install UserMSG hooks.");
		return false;
	}

	// Half-Life commands hook
	if (!CHLCommandsDetourMgr::the().install_hooks())
	{
		CConsole::the().error("Failed to install HL commands hooks.");
		return false;
	}

	CConsole::the().info("Initialized all hook managers.");

#ifdef OX_ENABLE_HOOK_TESTING
	CHookTests::the().execute_tests();
#endif

	return true;
}

void CoXWARE::shutdown_hook_managers()
{
	// we uninstall hooks only that ch ange memory.
	CSecurityModuleHook::the().uninstall_hooks();
	CMemoryHookMgr::the().uninstall_hooks();
	CMemoryFnDetourMgr::the().uninstall_hooks();
	CSVCFuncDetourMgr::the().uninstall_hooks();
	CUserMSGDetourMgr::the().uninstall_hooks();
	CHLCommandsDetourMgr::the().uninstall_hooks();
}

void CoXWARE::check_for_clientside_protectors()
{
	// https://github.com/2010kohtep/CSXGuard
	bool is_xguard = g_libloader_i->is_dll_loaded(L"xguard.dll");
	if (is_xguard)
	{
		CMessageBox::display_warning("You are using xguard. Be aware that this cheat isn't compatible with xguard or any similar software."
									 " This means that some parts of this cheat may not work properly or worse, can even crash your game.");
		return;
	}

	// Counter Strike Protector by 'quckly'
	bool is_cs_protector = g_libloader_i->is_dll_loaded(L"client_save.dll");
	if (is_cs_protector)
	{
		CMessageBox::display_warning("You are using CS protector. Be aware that this cheat isn't compatible with xguard or any similar software."
									 " This means that some parts of this cheat may not work properly or worse, can even crash your game.");
	}
}

bool CoXWARE::is_valid_game()
{
	auto app_id = CGameUtil::the().get_current_app_id();
	if (app_id != 10) // cs
	{
		CInjectedDllIPCLayerClient::the().report_error("You are not running Counter-Strike game. Your appid={}", app_id);
		return false;
	}

	CConsole::the().info("Running game Counter-Strike");

	return true;
}

bool CoXWARE::is_hardware()
{
	// we check if we're running the hardware engine by looking whenever the hw.dll is loaded in our process.
	// 
	// we have to check before we isntall any hooks, because we're targeting hw.dll inside these, and hence
	// if we're sw, there will be no hwdll...

	bool is_hw = g_libloader_i->is_dll_loaded(L"hw.dll");

	if (!is_hw) // software
	{
		CInjectedDllIPCLayerClient::the().report_error("You must run the game using the OpenGL renderer (-gl), not the software renderer.");
		return false;
	}

	CConsole::the().info("Running on the OpenGL/D3D renderer of the game.");

	return is_hw;
}

bool CoXWARE::validate_engine_build()
{
	m_gs_build_number = CGameUtil::the().get_build_number();

	if (!m_gs_build_number)
	{
		CInjectedDllIPCLayerClient::the().report_error("Couldn't get the engine build number at all! You must be using corrupted or ancient version of cs!");
		return false;
	}

	CConsole::the().info("Your build number is {}.", m_gs_build_number);
	return true;
}
