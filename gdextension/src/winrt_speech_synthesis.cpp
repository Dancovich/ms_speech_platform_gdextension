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

		m_SpeechSynth = std::make_shared<SpeechSynthesizer>(SpeechSynthesizer());
		m_MediaPlayer = std::make_shared<MediaPlayer>(MediaPlayer());
		m_IsSpeaking = false;
		m_IsPaused = false;

		// Creates a weak reference to the media player. It is used to detect
		// if we have been disposed (in which case the media player will be closed
		// and its reference lost) before async events complete. This avoids using dead
		// references after async boundaries.
		std::weak_ptr<MediaPlayer> mediaPlayerWeakPtr = m_MediaPlayer;

		m_MediaPlayer->MediaFailed([this, mediaPlayerWeakPtr](MediaPlayer const& sender, MediaPlayerFailedEventArgs const& args)
			{
				if (!mediaPlayerWeakPtr.expired())
				{
					call_deferred("emit_signal", "speech_ended");
					m_IsSpeaking = false;
					m_IsPaused = false;

					std::string errorMessage = std::string("Error playing back generated speech [") + winrt::to_string(args.ErrorMessage());
					errorMessage += "]";
					UtilityFunctions::push_warning(String(errorMessage.c_str()), __func__, __FILE__, __LINE__);
				}
			});

		m_MediaPlayer->MediaEnded([this, mediaPlayerWeakPtr](MediaPlayer const& sender, IInspectable const& args)
			{
				if (!mediaPlayerWeakPtr.expired())
				{
					call_deferred("emit_signal", "speech_ended");
					m_IsSpeaking = false;
					m_IsPaused = false;
				}
			});
	}

	WinRtSpeechSynthesizer::~WinRtSpeechSynthesizer()
	{
		UtilityFunctions::print_verbose("Disposing WinRT speech synthesizer");
		m_SpeechSynth->Close();
		m_MediaPlayer->Close();
	}

	TypedArray<Dictionary> WinRtSpeechSynthesizer::AllVoices() const
	{
		IVectorView<VoiceInformation> voices = m_SpeechSynth->AllVoices();
		TypedArray<Dictionary> voiceList;

		for (auto iterator = voices.begin(); iterator != voices.end(); ++iterator)
		{
			VoiceInformation voice = *iterator;
			Dictionary newVoice = {};

			newVoice["id"] = String(winrt::to_string(voice.Id()).c_str());
			newVoice["name"] = String(winrt::to_string(voice.DisplayName()).c_str());
			newVoice["description"] = String(winrt::to_string(voice.Description()).c_str());
			newVoice["language"] = String(winrt::to_string(voice.Language()).c_str());

			VoiceGender gender = voice.Gender();
			newVoice["gender"] = gender == VoiceGender::Male ? String("male") : String("female");

			voiceList.append(newVoice);
		}

		voiceList.make_read_only();
		return voiceList;
	}

	bool WinRtSpeechSynthesizer::IsSpeaking() const
	{
		return m_IsSpeaking;
	}

	bool WinRtSpeechSynthesizer::IsPaused() const
	{
		return m_IsPaused;
	}

	bool WinRtSpeechSynthesizer::Pause()
	{
		if (m_IsPaused)
		{
			return false;
		}

		MediaPlaybackSession session = m_MediaPlayer->PlaybackSession();
		if (session.PlaybackState() == MediaPlaybackState::Playing && session.CanPause())
		{
			m_IsPaused = true;
			m_MediaPlayer->Pause();
			emit_signal("speech_paused");
			return true;
		}

		return false;
	}

	bool WinRtSpeechSynthesizer::Resume()
	{
		if (!m_IsPaused)
		{
			return false;
		}

		m_IsPaused = false;
		MediaPlaybackSession session = m_MediaPlayer->PlaybackSession();
		if (session.PlaybackState() == MediaPlaybackState::Paused)
		{
			m_MediaPlayer->Play();
			emit_signal("speech_resumed");
			return true;
		}

		return false;
	}

	void WinRtSpeechSynthesizer::Stop()
	{
		MediaPlaybackSession session = m_MediaPlayer->PlaybackSession();
		if (session.CanPause())
		{
			m_MediaPlayer->Pause();
		}

		m_IsPaused = false;
		m_IsSpeaking = false;
		m_MediaPlayer->Source(nullptr);
		emit_signal("speech_ended");
	}

	void WinRtSpeechSynthesizer::SpeakWithVoice(const String& text, const String& voiceId, const double& volume, const double& pitch, const double& rate)
	{
		if (voiceId == "")
		{
			Speak(text, volume, pitch, rate);
			return;
		}

		UtilityFunctions::print_verbose(String("Searching for voice with ID ") + voiceId);

		IVectorView<VoiceInformation> voices = m_SpeechSynth->AllVoices();
		VoiceInformation usedVoice = m_SpeechSynth->Voice();
		std::string targetVoiceId = std::string(voiceId.utf8().get_data());
		bool found = false;

		for (auto iterator = begin(voices); iterator != end(voices); ++iterator)
		{
			const VoiceInformation& voice = *iterator;
			if (winrt::to_string(voice.Id()) == targetVoiceId)
			{
				usedVoice = voice;
				UtilityFunctions::print_verbose("Found voice with requested ID");
				found = true;
				break;
			}
		}

		if (!found)
		{
			UtilityFunctions::print_verbose("Could not find voice with the requested ID");
		}

		m_SpeechSynth->Voice(usedVoice);
		Speak(text, volume, pitch, rate);
	}

	void WinRtSpeechSynthesizer::Speak(const String& text, const double& volume, const double& pitch, const double& rate)
	{
		if (m_IsSpeaking)
		{
			emit_signal("speech_ended");
		}

		m_IsSpeaking = true;
		m_IsPaused = false;
		emit_signal("speech_started");

		m_SpeechSynth->Options().AudioVolume(std::clamp(volume, 0.0, 1.0));
		m_SpeechSynth->Options().AudioPitch(std::clamp(pitch, 0.0, 2.0));
		m_SpeechSynth->Options().SpeakingRate(std::clamp(rate, 0.5, 6.0));

		hstring convertedText = to_hstring(text.utf8().get_data());

		IAsyncOperation<SpeechSynthesisStream> generateStreamTask = m_SpeechSynth->SynthesizeTextToStreamAsync(convertedText);		
		
		// See explanation on constructor about async boundaries. In summary,
		// this weak reference allows us to detect if we have been killed
		// between starting a text to speech conversion and the speech actually
		// starting, in which case we might lose the reference to the media
		// player used to playback the generated speech
		std::weak_ptr<MediaPlayer> mediaPlayerWeakPtr = m_MediaPlayer;
		generateStreamTask.Completed([this, mediaPlayerWeakPtr](IAsyncOperation<SpeechSynthesisStream> const& res, AsyncStatus const status)
			{
				if (std::shared_ptr<MediaPlayer> mediaPlayer = mediaPlayerWeakPtr.lock())
				{
					if (status != AsyncStatus::Completed)
					{
						UtilityFunctions::push_warning("Error synthesizing texte to speech", __func__, __FILE__, __LINE__);
						return;
					}

					SpeechSynthesisStream generatedStream = res.GetResults();
					hstring cType = L"Audio";
					MediaSource source = MediaSource::CreateFromStream(generatedStream, cType);

					mediaPlayer->Source(source);
					mediaPlayer->Play();
				}
			});
	}

	void WinRtSpeechSynthesizer::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("list_voices"), &WinRtSpeechSynthesizer::AllVoices);
		ClassDB::bind_method(D_METHOD("speak", "text", "voice_id", "volume", "pitch", "speech_rate"), &WinRtSpeechSynthesizer::SpeakWithVoice, DEFVAL(""), DEFVAL(1.0), DEFVAL(1.0), DEFVAL(1.0));
		ClassDB::bind_method(D_METHOD("is_speaking"), &WinRtSpeechSynthesizer::IsSpeaking);
		ClassDB::bind_method(D_METHOD("is_paused"), &WinRtSpeechSynthesizer::IsPaused);
		ClassDB::bind_method(D_METHOD("pause"), &WinRtSpeechSynthesizer::Pause);
		ClassDB::bind_method(D_METHOD("resume"), &WinRtSpeechSynthesizer::Resume);
		ClassDB::bind_method(D_METHOD("stop"), &WinRtSpeechSynthesizer::Stop);

		ADD_SIGNAL(MethodInfo("speech_started"));
		ADD_SIGNAL(MethodInfo("speech_ended"));
		ADD_SIGNAL(MethodInfo("speech_paused"));
		ADD_SIGNAL(MethodInfo("speech_resumed"));
	}
}
