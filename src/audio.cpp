
#define TSF_IMPLEMENTATION
#include "TinySoundFont/tsf.h"

#define TML_IMPLEMENTATION
#include "TinySoundFont/tml.h"

#include "TinySoundFont/examples/minisdl_audio.h"
#include "audio.h"
//
// Holds the global instance pointer
static tsf* g_TinySoundFont;

// Holds global MIDI playback state
static double g_Msec;               //current playback time
static double off_Msec;
static double g_framecount;               //current playback time
// static double g_delay;               //current playback delay
static tml_message* g_MidiMessage;  //next message to be played

static void AudioCallback(void* data, Uint8 *stream, int len)
{
	//Number of samples to process
	int SampleBlock, SampleCount = (len / (2 * sizeof(float))); //2 output channels
	double factor = (g_Msec+(SampleCount * 1000.0 / 44100.0) - (g_framecount * 1000.0 / 60.0)) / ((1000.0 / 44100.0) * SampleCount); // 60fps
	//double factor = (((g_Msec / 1000.0) * 44100.0) / SampleCount) + 1 - ((g_framecount / 60.0) * 44100.0 / SampleCount));
	// TODO: This is probably wrong
	if (factor < 0) { factor = 1.0 / (-factor); }
	// printf("factor: %lf\n", factor);
	// printf("render block at %lf\n", g_Msec);
	for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, stream += (SampleBlock * (2 * sizeof(float))))
	{
		//We progress the MIDI playback and then process TSF_RENDER_EFFECTSAMPLEBLOCK samples at once
		if (SampleBlock > SampleCount) SampleBlock = SampleCount;

		int note_max = 16;
		//Loop through all MIDI messages which need to be played up until the current playback time
		for (g_Msec += SampleBlock * (1000.0 / 44100.0) / factor; g_MidiMessage && g_Msec >= g_MidiMessage->time + off_Msec; g_MidiMessage = g_MidiMessage->next)
		{
			switch (g_MidiMessage->type)
			{
				case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
					tsf_channel_set_presetnumber(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->program, (g_MidiMessage->channel == 9));
					break;
				case TML_NOTE_ON: //play a note
					tsf_channel_note_on(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key, g_MidiMessage->velocity / 127.0f);
					note_max -= 1;
					break;
				case TML_NOTE_OFF: //stop a note
					tsf_channel_note_off(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key);
					break;
				case TML_PITCH_BEND: //pitch wheel modification
					tsf_channel_set_pitchwheel(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->pitch_bend);
					break;
				case TML_CONTROL_CHANGE: //MIDI controller messages
					tsf_channel_midi_control(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->control, g_MidiMessage->control_value);
					break;
			}
			if (note_max == 0) break;
		}
		while (g_MidiMessage != NULL && g_Msec >= g_MidiMessage->time + off_Msec) {
			g_MidiMessage = g_MidiMessage->next;
		}

		// Render the block of audio samples in float format
		tsf_render_float(g_TinySoundFont, (float*)stream, SampleBlock, 0);
	}
	// g_Msec_Last = g_Msec - g_delay;
}
extern DECLSPEC const char *SDLCALL SDL_GetError(void);
Audio::Audio(double start_msec) {
	g_Msec = 0;
	off_Msec = start_msec;
    tml_message* TinyMidiLoader = NULL;

	// Define the desired audio output format we request
	SDL_AudioSpec OutputAudioSpec;
	OutputAudioSpec.freq = 44100;
	OutputAudioSpec.format = AUDIO_F32;
	OutputAudioSpec.channels = 2;
	OutputAudioSpec.samples = 4096;
	OutputAudioSpec.callback = AudioCallback;

	// Initialize the audio system
	if (SDL_AudioInit(TSF_NULL) < 0)
	{
		fprintf(stderr, "Could not initialize audio hardware or driver\n");
		return;
	}

	//Venture (Original WIP) by Ximon
	//https://musescore.com/user/2391686/scores/841451
	//License: Creative Commons copyright waiver (CC0)
	TinyMidiLoader = tml_load_filename("song.mid");
	if (!TinyMidiLoader)
	{
		fprintf(stderr, "Could not load MIDI file song.mid\n");
		return;
	}

	//Set up the global MidiMessage pointer to the first MIDI message
	g_MidiMessage = TinyMidiLoader;

	// Load the SoundFont from a file
	g_TinySoundFont = tsf_load_filename("FluidR3_GM.sf2");
	if (!g_TinySoundFont)
	{
		fprintf(stderr, "Could not load SoundFont FluidR3_GM.sf2\n");
		fprintf(stderr, "If you would like audio, read the README.\n");
		return;
	}

	//Initialize preset on special 10th MIDI channel to use percussion sound bank (128) if available
	tsf_channel_set_bank_preset(g_TinySoundFont, 9, 128, 0);

	// Set the SoundFont rendering output mode
	tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, OutputAudioSpec.freq, 0.0f);

	// Request the desired audio output format
	if (SDL_OpenAudio(&OutputAudioSpec, TSF_NULL) < 0)
	{
		puts(SDL_GetError());
		fprintf(stderr, "Could not open the audio hardware or the desired audio output format\n");
		return;
	}

	// Start the actual audio playback here
	// The audio thread will begin to call our AudioCallback function
	SDL_PauseAudio(0);
	_init = true;

}

void Audio::count_frames() {
	g_framecount++;
}
