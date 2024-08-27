#ifndef WINRT_SPEECH_SYNTHESIS_H
#define WINRT_SPEECH_SYNTHESIS_H

#include <winrt/windows.media.speechsynthesis.h>
#include <winrt/windows.media.playback.h>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot
{
	using namespace winrt;
	using namespace Windows::Foundation;
	using namespace Windows::Media::SpeechSynthesis;
	using namespace Windows::Media::Playback;

	class WinRtSpeechSynthesizer : public Object
	{
		GDCLASS(WinRtSpeechSynthesizer, Object)

	public:
		TypedArray<Dictionary> AllVoices() const;
		void SpeakWithVoice(const String& text, const String& voiceId, const double& volume, const double& pitch, const double& rate);
		bool Pause();
		bool Resume();
		void Stop();
		bool IsSpeaking() const;
		bool IsPaused() const;

		WinRtSpeechSynthesizer();
		~WinRtSpeechSynthesizer();


	private:
		std::shared_ptr<SpeechSynthesizer> m_SpeechSynth;
		std::shared_ptr<MediaPlayer> m_MediaPlayer;
		bool m_IsSpeaking;
		bool m_IsPaused;

		void Speak(const String& text, const double& volume, const double& pitch, const double& rate);

	protected:
		static void _bind_methods();
	};
}

#endif