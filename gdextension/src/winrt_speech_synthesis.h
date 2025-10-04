#ifndef WINRT_SPEECH_SYNTHESIS_H
#define WINRT_SPEECH_SYNTHESIS_H

#include <winrt/windows.media.speechsynthesis.h>
#include <winrt/windows.media.playback.h>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot
{
	using namespace winrt;
	using namespace Windows::Foundation;
	using namespace Windows::Media::SpeechSynthesis;
	using namespace Windows::Media::Playback;

	class WinRtSpeechSynthesizer : public Node
	{
		GDCLASS(WinRtSpeechSynthesizer, Node)

	public:
		void WinRtSpeechSynthesizer::_ready() override;
		void WinRtSpeechSynthesizer::_process(double delta) override;
		TypedArray<Dictionary> list_voices() const;
		void speak(const String& text, const String& voiceId, const float& volume, const float& pitch, const float& rate, const bool& interrupt, const int& utterance_id);
		void pause();
		void resume();
		void stop();
		bool is_speaking() const;
		bool is_paused() const;

		WinRtSpeechSynthesizer();
		~WinRtSpeechSynthesizer();


	private:
		struct TTSUtterance {
			String text;
			String voice;
			float volume = 1.f;
			float pitch = 1.f;
			float rate = 1.f;
			int id = 0;
		};

		const int NO_UTTERANCE_ID = std::numeric_limits<int>::min();

		SpeechSynthesizer synthesizer;
		MediaPlayer media_player;
		
		bool paused = false;
		int pending_utterance_id = NO_UTTERANCE_ID;
		List<TTSUtterance> queue;

		void _process_queue();
		bool WinRtSpeechSynthesizer::is_ssml(TTSUtterance& message) const;


	protected:
		static void _bind_methods();
	};
}

#endif