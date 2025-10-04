#include "register_types.h"

#include "winrt_speech_synthesis.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void initialize_winrt_speech_synthesis_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    ClassDB::register_class<WinRtSpeechSynthesizer>();

    // WinRtSpeechSynthesizer *winRtSpeechSynthesizerInstance = memnew(WinRtSpeechSynthesizer);
    // _singleton_instance_id = winRtSpeechSynthesizerInstance->get_instance_id();
    // Engine::get_singleton()->register_singleton("WinRtSpeechSynthesizer", winRtSpeechSynthesizerInstance);
}

// void uninitialize_winrt_speech_synthesis_module(ModuleInitializationLevel p_level)
// {}

extern "C"
{
    // Initialization.
    GDExtensionBool GDE_EXPORT winrt_speech_synthesis_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_winrt_speech_synthesis_module);
        //init_obj.register_terminator(uninitialize_winrt_speech_synthesis_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}