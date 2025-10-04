#include "winrt_speech_synthesis.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Core.h>
#include <godot_cpp/core/class_db.hpp>
#include <memory>

namespace godot
{
	using namespace Windows::Media::SpeechSynthesis;
	using namespace Windows::Foundation::Collections;
	using namespace Windows::Media::Core;

	WinRtSpeechSynthesizer::WinRtSpeechSynthesizer()
	{
		UtilityFunctions::print_verbose("Initializing WinRT speech synthesizer");

		synthesizer = SpeechSynthesizer();
		if (synthesizer == nullptr) {
			UtilityFunctions::print_verbose("WinRT speech synthesizer: Cannot initialize ISpeechSynthesizer!");
			return;
		}

		synthesizer.Options().IncludeWordBoundaryMetadata(true);
		synthesizer.Options().IncludeSentenceBoundaryMetadata(false);

		media_player = MediaPlayer();
		if (media_player == nullptr) {
			print_verbose("WinRT speech synthesizer: Cannot initialize MediaPlayer!");
			return;
		}
		
		paused = false;
		pending_utterance_id = NO_UTTERANCE_ID;

		media_player.MediaFailed([this](MediaPlayer const& sender, MediaPlayerFailedEventArgs const& args)
			{
				if (pending_utterance_id != NO_UTTERANCE_ID) {
					call_deferred("emit_signal", "speech_canceled", pending_utterance_id);
					pending_utterance_id = NO_UTTERANCE_ID;
				}
			});

		media_player.MediaEnded([this](MediaPlayer const& sender, IInspectable const& args)
			{
				if (pending_utterance_id != NO_UTTERANCE_ID) {
					call_deferred("emit_signal", "speech_ended", pending_utterance_id);
					pending_utterance_id = NO_UTTERANCE_ID;
				}
			});
		
		UtilityFunctions::print_verbose("WinRT speech synthesizer initialized");
	}

	WinRtSpeechSynthesizer::~WinRtSpeechSynthesizer()
	{
		UtilityFunctions::print_verbose("Disposing WinRT speech synthesizer");
		synthesizer.Close();
		media_player.Close();
	}

	void WinRtSpeechSynthesizer::_ready() {
		set_process(false);
	}

	void WinRtSpeechSynthesizer::_process(double delta) {
		_process_queue();
	}

	TypedArray<Dictionary> WinRtSpeechSynthesizer::list_voices() const
	{
		IVectorView<VoiceInformation> voices = synthesizer.AllVoices();
		TypedArray<Dictionary> voiceList;

		for (auto iterator = voices.begin(); iterator != voices.end(); ++iterator)
		{
			VoiceInformation voice = *iterator;
			Dictionary newVoice = {};

			wchar_t const* voice_id = winrt::to_hstring(voice.Id()).c_str();
			wchar_t const* voice_name = winrt::to_hstring(voice.DisplayName()).c_str();
			wchar_t const* voice_language = winrt::to_hstring(voice.Language()).c_str();
			wchar_t const* voice_description = winrt::to_hstring(voice.Description()).c_str();

			newVoice["id"] = String::utf16((const char16_t*)voice_id);
			newVoice["name"] = String::utf16((const char16_t*)voice_name);
			newVoice["language"] = String::utf16((const char16_t*)voice_language);
			newVoice["description"] = String::utf16((const char16_t*)voice_description);

			VoiceGender gender = voice.Gender();
			newVoice["gender"] = gender == VoiceGender::Male ? String("male") : String("female");

			voiceList.append(newVoice);
		}

		voiceList.make_read_only();
		return voiceList;
	}

	bool WinRtSpeechSynthesizer::is_speaking() const
	{
		ERR_FAIL_NULL_V(media_player, false);
		return media_player.PlaybackSession().PlaybackState() == MediaPlaybackState::Playing || pending_utterance_id != NO_UTTERANCE_ID;
	}

	bool WinRtSpeechSynthesizer::is_paused() const
	{
		return paused;
	}

	void WinRtSpeechSynthesizer::pause()
	{
		ERR_FAIL_NULL(media_player);
		if (!paused) {
			MediaPlaybackSession session = media_player.PlaybackSession();
			if (session.CanPause()) {
				media_player.Pause();
				paused = true;
			}
		}
	}

	void WinRtSpeechSynthesizer::resume()
	{
		ERR_FAIL_NULL(media_player);
		if (paused && media_player.Source() != nullptr) {
			media_player.Play();
			paused = false;
		}
	}

	void WinRtSpeechSynthesizer::stop()
	{
		ERR_FAIL_NULL(media_player);

		MediaPlaybackSession session = media_player.PlaybackSession();
		if (session.CanPause()) {
			media_player.Pause();
		}

		for (TTSUtterance& message : queue) {
			call_deferred("emit_signal", "speech_canceled", message.id);
		}

		queue.clear();

		int utterance_id = pending_utterance_id;
		if (utterance_id != NO_UTTERANCE_ID) {
			call_deferred("emit_signal", "speech_canceled", utterance_id);
			pending_utterance_id = NO_UTTERANCE_ID;
		}

		media_player.Source(nullptr);
		paused = false;
	}

	void WinRtSpeechSynthesizer::speak(const String& text, const String& voiceId, const float& volume, const float& pitch, const float& rate, const bool& interrupt, const int& utterance_id)
	{
		ERR_FAIL_NULL(media_player);

		if (text.is_empty()) {
			UtilityFunctions::print_verbose("Empty text, skipping...");
			return;
		}

		if (interrupt)
		{
			stop();
		}

		TTSUtterance message;
		message.text = text;
		message.voice = voiceId;
		message.volume = CLAMP(volume, 0.f, 1.f);
		message.pitch = CLAMP(pitch, 0.f, 2.f);
		message.rate = CLAMP(rate, 0.5f, 6.f);
		message.id = utterance_id;
		
		queue.push_back(message);

		if (paused) {
			resume();
		}

		set_process(true);
	}

	void WinRtSpeechSynthesizer::_process_queue()
	{
		ERR_FAIL_NULL(synthesizer);
		ERR_FAIL_NULL(media_player);

		if (!paused && queue.size() > 0 && !is_speaking()) {
			TTSUtterance& message = queue.front()->get();

			IVectorView<VoiceInformation> available_voices = synthesizer.AllVoices();
			bool found_voice = false;
			for (impl::fast_iterator<IVectorView<VoiceInformation>> iterator = available_voices.begin(); iterator != available_voices.end(); ++iterator) {
				VoiceInformation current_voice = *iterator;

				wchar_t const* voice_id_utf16 = winrt::to_hstring(current_voice.Id()).c_str();
				if (message.voice == String::utf16((const char16_t*)voice_id_utf16)) {
					synthesizer.Voice(current_voice);
					found_voice = true;
					break;
				}
			}

			if (!found_voice) {
				VoiceInformation default_voice = synthesizer.DefaultVoice();
				synthesizer.Voice(default_voice);
			}

			synthesizer.Options().AudioVolume(message.volume);
			synthesizer.Options().AudioPitch(message.pitch);
			synthesizer.Options().SpeakingRate(message.rate);

			const char16_t* utf16_data = message.text.utf16().get_data();
			size_t utf16_length = message.text.length();
			hstring converted_text(reinterpret_cast<const wchar_t*>(utf16_data), utf16_length);

			pending_utterance_id = message.id;
			IAsyncOperation<SpeechSynthesisStream> synthesize_task = is_ssml(message)
				? synthesizer.SynthesizeSsmlToStreamAsync(converted_text)
				: synthesizer.SynthesizeTextToStreamAsync(converted_text);
			synthesize_task.Completed([this](IAsyncOperation<SpeechSynthesisStream> const& res, AsyncStatus const status) {
				int utterance_id = pending_utterance_id;
				if (utterance_id == NO_UTTERANCE_ID) {
					return;
				}

				if (media_player == nullptr) {
					print_error("Could not synthesize text to speech - media_player != null is false");
					call_deferred("emit_signal", "speech_canceled", pending_utterance_id);

					pending_utterance_id = NO_UTTERANCE_ID;
				}
				else if (status != AsyncStatus::Completed) {
					if (status == AsyncStatus::Error) {
						print_error(vformat("Could not synthesize text to speech - error processing text (marlfomed SSML?)"));
					}
					else if (status == AsyncStatus::Canceled) {
						print_error("Could not synthesize text to speech - operation was canceled");
					}
					else {
						print_error("Could not synthesize text to speech - unknown error");
					}

					call_deferred("emit_signal", "speech_canceled", pending_utterance_id);
					pending_utterance_id = NO_UTTERANCE_ID;
				}
				else {
					SpeechSynthesisStream generated_stream = res.GetResults();

					MediaSource source = MediaSource::CreateFromStream(generated_stream, L"Audio");
					MediaPlaybackItem media_item = MediaPlaybackItem(source);
					IVectorView<TimedMetadataTrack> tracks = media_item.TimedMetadataTracks();

					for (size_t index = 0; index < tracks.Size(); index++) {
						TimedMetadataTrack track = tracks.GetAt(index);

						if (track.TimedMetadataKind() == TimedMetadataKind::Speech) {
							track.CueEntered([this, utterance_id](TimedMetadataTrack const& sender, MediaCueEventArgs const& args) {
								SpeechCue cue = args.Cue().as<SpeechCue>();
								if (cue != nullptr) {
									hstring cue_type = sender.Label();

									if (cue_type == L"SpeechWord") {
										int position_in_input = cue.StartPositionInInput().as<int>();
										call_deferred("emit_signal", "speech_word_boundary_reached", pending_utterance_id, position_in_input);
									}
								}
								});

							media_item.TimedMetadataTracks().SetPresentationMode(index, TimedMetadataTrackPresentationMode::Hidden);
						}
					}

					media_player.Source(media_item);
					media_player.Play();
					call_deferred("emit_signal", "speech_started", pending_utterance_id);
				}
				});

			queue.pop_front();
		}

		if (queue.size() == 0) {
			set_process(false);
		}
	}

	bool WinRtSpeechSynthesizer::is_ssml(TTSUtterance& message) const {
		// Checks for the "<speak" tag at the start of the text, which should be true for all
		// valid SSML 1.0 strings. Doesn't check if the full string is valid SSML 1.0 according
		// to WinRT, so messages that pass this test can still fail to be synthesized.
		String text = message.text.strip_edges(true, false).substr(0, 6).to_lower();
		return text == "<speak";
	}

	void WinRtSpeechSynthesizer::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("list_voices"), &WinRtSpeechSynthesizer::list_voices);
		ClassDB::bind_method(D_METHOD("speak", "text", "voice_id", "volume", "pitch", "speech_rate", "interrupt", "utterance_id"), &WinRtSpeechSynthesizer::speak, DEFVAL(""), DEFVAL(1.0), DEFVAL(1.0), DEFVAL(1.0), DEFVAL(false), DEFVAL(0));
		ClassDB::bind_method(D_METHOD("is_speaking"), &WinRtSpeechSynthesizer::is_speaking);
		ClassDB::bind_method(D_METHOD("is_paused"), &WinRtSpeechSynthesizer::is_paused);
		ClassDB::bind_method(D_METHOD("pause"), &WinRtSpeechSynthesizer::pause);
		ClassDB::bind_method(D_METHOD("resume"), &WinRtSpeechSynthesizer::resume);
		ClassDB::bind_method(D_METHOD("stop"), &WinRtSpeechSynthesizer::stop);

		ADD_SIGNAL(MethodInfo("speech_started", PropertyInfo(Variant::INT, "utterance_id")));
		ADD_SIGNAL(MethodInfo("speech_ended", PropertyInfo(Variant::INT, "utterance_id")));
		ADD_SIGNAL(MethodInfo("speech_canceled", PropertyInfo(Variant::INT, "utterance_id")));
		ADD_SIGNAL(MethodInfo("speech_word_boundary_reached", PropertyInfo(Variant::INT, "utterance_id"), PropertyInfo(Variant::INT, "index")));
	}
}
