/*
	Thank you for contributing your code to the project!


	Hi there!
	If you are a shader developer, you likely found yourself in the wrong place. Get the actual addon from Releases!
	If you are an addon developer, please keep in mind that this project should not be taken as a reference, rather as a possible implementation.
	This does one specific thing, relatively well, and nothing more. Check in with ReShade docs and the #addon-programming chat in discord for better addon practices.

	For contributors:
		!!! IMPORTANT !!!
		Declare RESHADE_PATH env variable to the !root! of a cloned repo of ReShade. This is required to compile the project!

		In order to declare a new saving path, go to SavingStrategiesImpl.h, and inherit from ISavingStrategy.
		Make sure to override all methods and the supported field. Finally, go to StrategyManager, and find the StrategyManager init, and register accordingly.
*/
#define TINYEXR_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define ENTIRE_RESOURCE nullptr
#define EXR_f32 0
#define NOMINMAX
#define LEVELS 0
#define LAYERS 1
#define SAMPLES 1

#include <imgui.h>
#include <reshade.hpp>
#include "CallbackFunctions.h"

using namespace reshade;

extern "C" __declspec(dllexport) const char *NAME = "Texture Dump";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "Helper addon for debugging and analyzing shaders via external tools.";
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID) {
	switch (fdwReason) {
	    case DLL_PROCESS_ATTACH:
		    if (!register_addon(hModule))
			    return FALSE;

		    register_overlay(nullptr, UI);
		    register_event<addon_event::init_effect_runtime>(initialize);
		    register_event<addon_event::reshade_finish_effects>(on_finish_effects);
		    break;
	    case DLL_PROCESS_DETACH:
	        unregister_event<addon_event::init_effect_runtime>(initialize);
	        unregister_event<addon_event::reshade_finish_effects>(on_finish_effects);
		    unregister_addon(hModule);
		    break;
	}


	return TRUE;
}