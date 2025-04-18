extends Control

@onready var play_pause_button: Button = $PlayPause
@onready var stop_button: Button = $Stop
@onready var voice_selection: OptionButton = $VoiceSelection
@onready var volume: HSlider = $Volume
@onready var pitch: HSlider = $Pitch
@onready var rate: HSlider = $Rate

var available_voices: Array[Dictionary]
var current_voice_index := -1

func _ready() -> void:
	WinRtSpeechSynthesizer.speech_started.connect(_on_speech_started)
	WinRtSpeechSynthesizer.speech_ended.connect(_on_speech_ended)
	WinRtSpeechSynthesizer.speech_paused.connect(_on_speech_paused)
	WinRtSpeechSynthesizer.speech_resumed.connect(_on_speech_resumed)

	current_voice_index = -1
	available_voices = WinRtSpeechSynthesizer.list_voices()
	var index := 0
	if available_voices.size() > 0:
		current_voice_index = 0
		for voice in available_voices:
			print("Found voice: " + str(voice))
			voice_selection.add_item("%s - (%s, %s)" % [voice["name"], voice["gender"],voice["language"]], index)
			index += 1


func _on_play_pause_pressed() -> void:
	if WinRtSpeechSynthesizer.is_speaking():
		@warning_ignore("standalone_ternary")
		WinRtSpeechSynthesizer.pause() if not WinRtSpeechSynthesizer.is_paused() else WinRtSpeechSynthesizer.resume()
	else:
		var v := clampf(volume.value, 0, 100) / 100.0
		var p := clampf(pitch.value, 0, 200) / 100.0
		var r := clampf(rate.value, 5, 600) / 100.0
		var selected_voice: Dictionary = available_voices[current_voice_index] if current_voice_index > -1 else {"id": "", "language": ""}
		var phrase := "Testando a integração com síntese de fala do Windows 10." if selected_voice["language"] == "pt-BR" else "Testing Windows 10 speech synthesis integration."
		WinRtSpeechSynthesizer.speak(phrase, selected_voice["id"], v, p, r)


func _on_play_second_audio_pressed() -> void:
	var v := clampf(volume.value, 0, 100) / 100.0
	var p := clampf(pitch.value, 0, 200) / 100.0
	var r := clampf(rate.value, 5, 600) / 100.0
	var selected_voice: String = available_voices[current_voice_index]["id"] if current_voice_index > -1 else ""
	WinRtSpeechSynthesizer.speak("Second speech requested.", selected_voice, v, p, r)


func _on_stop_pressed() -> void:
	WinRtSpeechSynthesizer.stop()


func _on_speech_started() -> void:
	print("Started to speak")
	play_pause_button.text = "Pause"
	stop_button.disabled = false


func _on_speech_ended() -> void:
	print("Finished speaking")
	play_pause_button.text = "Test speech"
	stop_button.disabled = true


func _on_speech_paused() -> void:
	print("Paused")
	play_pause_button.text = "Resume"


func _on_speech_resumed() -> void:
	print("Resumed")
	play_pause_button.text = "Pause"


func _on_voice_selection_item_selected(index: int) -> void:
	current_voice_index = voice_selection.get_item_id(index)
	print("Index %d" % current_voice_index)


func _on_default_values_pressed() -> void:
	volume.value = 100.0
	pitch.value = 100.0
	rate.value = 100.0
