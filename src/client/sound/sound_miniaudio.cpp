/*
Copyright (C) 2018-2019 Krzysztof Kondrak

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// music playback using Miniaudio library (https://github.com/dr-soft/miniaudio)

#ifdef _WIN32
#include <windows.h>
#endif
// force ALSA on Linux to avoid playback issues when using mixed backends
#ifdef __linux__
#  define MA_NO_PULSEAUDIO
#  define MA_NO_JACK
#endif

#define DR_FLAC_IMPLEMENTATION
#include <client/sound/miniaudio/dr_flac.h>  /* Enables FLAC decoding. */
#define DR_MP3_IMPLEMENTATION
#include <client/sound/miniaudio/dr_mp3.h>   /* Enables MP3 decoding. */
#define DR_WAV_IMPLEMENTATION
#include <client/sound/miniaudio/dr_wav.h>   /* Enables WAV decoding. */

#include <client/sound/miniaudio/stb_vorbis.c> /* Enables OGG/Vorbis decoding. */

#define MINIAUDIO_IMPLEMENTATION
#include <client/include/miniaudio.h>

#include <client/client.hpp>

static ma_decoder decoder;
static ma_device device;
static int32_t loopcounter;
static int32_t playTrack = 0;
static bool enabled = true;
static bool paused = false;
static bool playLooping = false;
static bool trackFinished = false;

static cvar_t *s_volume_music;
static cvar_t *s_loopcount;
static cvar_t *s_looptrack;
static cvar_t *no_music;

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
	if (pDecoder == NULL) {
		return;
	}

	// playback completed
	if (!ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount))
		trackFinished = true;
}

static void Miniaudio_Pause()
{
	if (!enabled || !ma_device_is_started(&device) || paused)
		return;

	ma_device_stop(&device);
	paused = true;
}

static void Miniaudio_Resume()
{
	if (!enabled || !paused)
		return;

	ma_device_start(&device);
	paused = false;
}

static void Miniaudio_f()
{
	char	*command;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv(1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		Miniaudio_Stop();
		enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		Miniaudio_Play(atoi(Cmd_Argv(2)), false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		Miniaudio_Play(atoi(Cmd_Argv(2)), true);
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		Miniaudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		Miniaudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		Miniaudio_Resume();
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		if (device.pContext)
			Com_Printf("Using %s backend. ", ma_get_backend_name(device.pContext->backend));
		else
			Com_Printf("No audio backend enabled. ");

		if (ma_device_is_started(&device))
			Com_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (paused)
			Com_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else
			Com_Printf("No music is playing.\n");
		return;
	}
}


void Miniaudio_Init()
{
	s_volume_music = Cvar_Get("s_volume_music", "1", CVAR_ARCHIVE);
	s_loopcount = Cvar_Get("s_loopcount", "4", 0);
	s_looptrack = Cvar_Get("s_looptrack", "11", 0);
	no_music = Cvar_Get("no_music", "0", 0);
	enabled = no_music->value == 0;
	paused = false;
	Cmd_AddCommand("miniaudio", Miniaudio_f);
}

static ma_result LoadTrack(const char *gamedir, int32_t track)
{
	ma_result result;
	int32_t trackExtIdx = 0;
	static const char *trackExts[] = { "ogg", "flac", "mp3", "wav" };

	do
	{
		char trackPath[64];
		snprintf(trackPath, sizeof(trackPath), "%s/music/track%s%i.%s", gamedir, track < 10 ? "0" : "", track, trackExts[trackExtIdx]);
		result = ma_decoder_init_file(trackPath, NULL, &decoder);
	} while (result != MA_SUCCESS && ++trackExtIdx < 4);

	return result;
}

void Miniaudio_Play(int32_t track, bool looping)
{
	ma_result result;
	ma_device_config deviceConfig;

	if (!enabled || playTrack == track)
		return;

	Miniaudio_Stop();

	// ignore invalid tracks
	if (track < 1)
		return;

	result = LoadTrack(FS_Gamedir(), track);

	if (result != MA_SUCCESS)
	{
		Com_Printf("Failed to open %s/music/track%s%i.[ogg/flac/mp3/wav]: error %i\n", FS_Gamedir(), track < 10 ? "0" : "", track, result);
		return;
	}

	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = decoder.outputFormat;
	deviceConfig.playback.channels = decoder.outputChannels;
	deviceConfig.sampleRate = decoder.outputSampleRate;
	deviceConfig.dataCallback = data_callback;
	deviceConfig.pUserData = &decoder;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
		Com_Printf("Failed to open playback device: error %i\n", result);
		ma_decoder_uninit(&decoder);
		return;
	}

	if (ma_device_start(&device) != MA_SUCCESS) {
		Com_Printf("Failed to start playback device: error %i\n", result);
		ma_device_uninit(&device);
		ma_decoder_uninit(&decoder);
		return;
	}

	loopcounter = 0;
	playTrack = track;
	playLooping = looping;
	paused = false;
	trackFinished = false;

	if ( Cvar_VariableValue("s_volume_music") == 0 )
		Miniaudio_Pause();

	ma_device_set_master_volume(&device, s_volume_music->value);
}

void Miniaudio_Stop()
{
	if (!enabled || !ma_device_is_started(&device))
		return;

	paused = false;
	playTrack = 0;

	ma_device_uninit(&device);
	ma_decoder_uninit(&decoder);
}

void Miniaudio_Update()
{
	if (no_music->value != 0)
		return;

	if (s_volume_music->modified)
	{
		s_volume_music->modified = false;

		if (s_volume_music->value < 0.f)
			Cvar_SetValue("s_volume_music", 0.f);
		if (s_volume_music->value > 1.f)
			Cvar_SetValue("s_volume_music", 1.f);

		ma_device_set_master_volume(&device, s_volume_music->value);
	}

	if ((s_volume_music->value == 0) != !enabled)
	{
		if (s_volume_music->value == 0)
		{
			Miniaudio_Pause();
			enabled = false;
		}
		else
		{
			enabled = true;
			int32_t track = atoi(cl.configstrings[CS_CDTRACK]);
			if (!paused || playTrack != track)
			{
				if ((playTrack == 0 && !playLooping) || (playTrack > 0 && playTrack != track))
					Miniaudio_Play(track, true);
				else
					Miniaudio_Play(playTrack, playLooping);
			}
			else
				Miniaudio_Resume();
		}
	}

	if (enabled && ma_device_is_started(&device) && trackFinished)
	{
		trackFinished = false;
		if (playLooping)
		{
			// if the track has played the given number of times,
			// go to the ambient track
			if (++loopcounter >= s_loopcount->value)
			{
				// ambient track will be played for the first time, so we need to load it
				if (loopcounter == s_loopcount->value)
				{
					Miniaudio_Stop();
					Miniaudio_Play((int32_t)s_looptrack->value, true);
				}
				else
				{
					ma_decoder_seek_to_pcm_frame(&decoder, 0);
				}
			}
			else
			{
				ma_decoder_seek_to_pcm_frame(&decoder, 0);
			}
		}
	}
}

void Miniaudio_Shutdown()
{
	Miniaudio_Stop();
	Cmd_RemoveCommand("miniaudio");
}
