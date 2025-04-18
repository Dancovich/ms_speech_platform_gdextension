# Microsoft Speech Platform API GDExtension

This GDExtension integrates your Godot project with [Microsoft Speech Platform API for TTS (Text-To-Speech)](https://learn.microsoft.com/pt-br/uwp/api/windows.media.speechsynthesis?view=winrt-26100).

This API was introduced in Windows 8 and is the new API for TTS and speech recognition in newer versions of Windows. While the older Microsoft Speech API (SAPI) can still be used, voice engines created for this new API can't be used in the older one. On the other hand, voices created for SAPI can be used in this new API without issues.

The Windows version of Godot integrates with SAPI from factory, but can't use voices designed for the Speech Platform API. This GDExtension provides support for the new API.

By definition, this GDExtension is Windows only (Windows 10 or newer). If your Godot game or app is multiplatform, it is recommended to use Godot's own API for Text-to-speech.

## How to install

Clone this repository and copy the `addons` folder to the root folder of your Godot project.

## How to use

When installed as an addon, this library will give you access to the `WinRtSpeechSynthesizer` singleton.

You can access the methods in this singleton directly. Check the docs to see which methods are available.

Example:

```gdscript
# [myscript.gd]

func _ready() -> void:
    # Connect to know when a new text to speech conversion started
    WinRtSpeechSynthesizer.speech_started.connect(_on_speech_started)
	
    # Connect to know when the currently playing speech ends
    WinRtSpeechSynthesizer.speech_ended.connect(_on_speech_ended)
	
    # Connect to know when the currently playing speech is paused
    WinRtSpeechSynthesizer.speech_paused.connect(_on_speech_paused)
	
    # Connect to know when the currently playing speech is resumed
    WinRtSpeechSynthesizer.speech_resumed.connect(_on_speech_resumed)


func text_to_speech() -> void:
    # Lists all available voices to use
    var voices := WinRtSpeechSynthesizer.list_voices()

    # Select a voice. To use the default one, pass an empty string
    var selected_voice: String = voices[0] if voices.size() > 0 else ""

    # Starts the text to speech conversion and plays the resulting speech. Stops any previously playing speeches.
    WinRtSpeechSynthesizer.speak("Testing text to speech conversion", selected_voice, 1.0, 1.0, 1.0)
```

## Compiling from source

- Clone this repository
- [Follow the guide](https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_windows.html) on how to compile Godot for Windows, up until but not including the section [Downloading Godot's Source](https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_windows.html#downloading-godot-s-source).
- Install Windows SDK (If you installed Visual Studio Community on step two, you probably already have this).
- Open the file `gdextension/SConstruct` and edit the path to the static libraries of the Windows Runtime API (`WindowsApp.lib`). Follow the comment on the `SConstruct` file to know where set this path.
- Run `scons` inside the `gdextension` folder to build a debug version of the library. Pass `target=template_release` to build the release version.
- If all is well, files will be placed in `addons/winrt-speech-synthesis` folder.

## Remarks

This is a work in progress. This library was created as a means for learning about C++ and Godot development, so there are no guarantees that the code is 100% correct or that it won't cause any issues. Also, most probably the code violates GDExtension and C++ coding styles and best practices.

## Contributing

If you detect any mistakes or possible improvements, feel free to either open an issue or fork the project and make any adjustments you deem necessary.
