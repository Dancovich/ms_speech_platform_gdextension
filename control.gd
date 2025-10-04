extends Control

@onready var play_pause_button: Button = $PlayPause
@onready var stop_button: Button = $Stop
@onready var voice_selection: OptionButton = $VoiceSelection
@onready var volume: HSlider = $Volume
@onready var pitch: HSlider = $Pitch
@onready var rate: HSlider = $Rate
@onready var synthesizer: WinRtSpeechSynthesizer = $Synthesizer

var available_voices: Array[Dictionary]
var current_voice_index := -1

func _ready() -> void:
	current_voice_index = -1
	available_voices = synthesizer.list_voices()
	var index := 0
	if available_voices.size() > 0:
		current_voice_index = 0
		for voice in available_voices:
			print("Found voice: " + str(voice))
			voice_selection.add_item("%s - (%s, %s)" % [voice["name"], voice["gender"],voice["language"]], index)
			index += 1


func _on_play_pause_pressed() -> void:
	if synthesizer.is_speaking():
		if not synthesizer.is_paused():
			synthesizer.pause()
			play_pause_button.text = "Resume"
		else:
			synthesizer.resume()
			play_pause_button.text = "Pause"
	else:
		var v := clampf(volume.value, 0, 100) / 100.0
		var p := clampf(pitch.value, 0, 200) / 100.0
		var r := clampf(rate.value, 5, 600) / 100.0
		var selected_voice: Dictionary = available_voices[current_voice_index] if current_voice_index > -1 else {"id": "", "language": ""}
		var phrase := "Testando a integração com síntese de fala do Windows 10." if selected_voice["language"] == "pt-BR" else "Testing Windows 10 speech synthesis integration."
		synthesizer.speak(phrase, selected_voice["id"], v, p, r)


func _on_play_second_audio_pressed() -> void:
	var v := clampf(volume.value, 0, 100) / 100.0
	var p := clampf(pitch.value, 0, 200) / 100.0
	var r := clampf(rate.value, 5, 600) / 100.0
	var selected_voice: String = available_voices[current_voice_index]["id"] if current_voice_index > -1 else ""
	synthesizer.speak("Second speech requested.", selected_voice, v, p, r)


func _on_stop_pressed() -> void:
	synthesizer.stop()
	pass


func _on_voice_selection_item_selected(index: int) -> void:
	current_voice_index = voice_selection.get_item_id(index)
	print("Index %d" % current_voice_index)


func _on_default_values_pressed() -> void:
	volume.value = 100.0
	pitch.value = 100.0
	rate.value = 100.0


@warning_ignore("unused_parameter")
func _on_synthesizer_speech_canceled(utterance_id: int) -> void:
	print("Canceled speaking")
	play_pause_button.text = "Test speech"
	stop_button.disabled = true


@warning_ignore("unused_parameter")
func _on_synthesizer_speech_ended(utterance_id: int) -> void:
	print("Finished speaking")
	play_pause_button.text = "Test speech"
	stop_button.disabled = true


@warning_ignore("unused_parameter")
func _on_synthesizer_speech_started(utterance_id: int) -> void:
	print("Started to speak")
	play_pause_button.text = "Pause"
	stop_button.disabled = false


@warning_ignore("unused_parameter")
func _on_synthesizer_speech_word_boundary_reached(utterance_id: int, index: int) -> void:
	print("Word reached at %d" % index)
