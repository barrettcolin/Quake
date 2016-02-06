
#include <stdio.h>
#include "SDL_audio.h"
#include "SDL_byteorder.h"
#include "quakedef.h"

static int snd_inited;

static int dma_sample_bytes, dma_pos, dma_size;
static unsigned char *dma_buffer;

extern int desired_speed;
extern int desired_bits;

static unsigned next_power_of_two(unsigned n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;

	return n;
}

static void paint_audio(void *unused, Uint8 *stream, int num_bytes)
{
	int current_byte = dma_pos * dma_sample_bytes;
	int num_bytes_to_end_of_buffer = dma_size - current_byte;

	int bytes1 = (num_bytes < num_bytes_to_end_of_buffer)
		? num_bytes
		: num_bytes_to_end_of_buffer;

	Q_memcpy(stream, shm->buffer + current_byte, bytes1);
	dma_pos += (bytes1 / dma_sample_bytes);

	if (bytes1 < num_bytes)
	{
		int bytes2 = num_bytes - bytes1;
		Q_memcpy(stream + bytes1, shm->buffer, bytes2);
		dma_pos = (bytes2 / dma_sample_bytes);
	}
}

qboolean SNDDMA_Init(void)
{
	SDL_AudioSpec desired = { 0 }, obtained = { 0 };

	snd_inited = 0;

	/* Set up the desired format */
	desired.freq = desired_speed;
	switch (desired_bits) {
		case 8:
			desired.format = AUDIO_U8;
			break;
		case 16:
			if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
				desired.format = AUDIO_S16MSB;
			else
				desired.format = AUDIO_S16LSB;
			break;
		default:
        		Con_Printf("Unknown number of audio bits: %d\n",
								desired_bits);
			return 0;
	}
	desired.channels = 2;
	desired.samples = 512;
	desired.callback = paint_audio;

	/* Open the audio device */
	if ( SDL_OpenAudio(&desired, &obtained) < 0 ) {
        	Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
		return 0;
	}

	/* Make sure we can support the audio format */
	switch (obtained.format) {
		case AUDIO_U8:
			/* Supported */
			break;
		case AUDIO_S16LSB:
		case AUDIO_S16MSB:
			if ( ((obtained.format == AUDIO_S16LSB) &&
			     (SDL_BYTEORDER == SDL_LIL_ENDIAN)) ||
			     ((obtained.format == AUDIO_S16MSB) &&
			     (SDL_BYTEORDER == SDL_BIG_ENDIAN)) ) {
				/* Supported */
				break;
			}
			/* Unsupported, fall through */;
		default:
			/* Not supported -- force SDL to do our bidding */
			SDL_CloseAudio();
			if ( SDL_OpenAudio(&desired, NULL) < 0 ) {
        			Con_Printf("Couldn't open SDL audio: %s\n",
							SDL_GetError());
				return 0;
			}
			memcpy(&obtained, &desired, sizeof(desired));
			break;
	}

	/* Fill the audio DMA information block */
	shm = &sn;
	shm->splitbuffer = 0;
	shm->samplebits = (obtained.format & 0xFF);
	shm->speed = obtained.freq;
	shm->channels = obtained.channels;
	shm->samples = next_power_of_two(shm->speed * shm->channels);
	shm->submission_chunk = 1;

	dma_sample_bytes = shm->samplebits / 8;
	dma_size = (shm->samples * dma_sample_bytes);
	shm->buffer = dma_buffer = calloc(1, dma_size);

	SDL_PauseAudio(0);

	snd_inited = 1;
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	return dma_pos;
}

void SNDDMA_Shutdown(void)
{
	if (snd_inited)
	{
		SDL_PauseAudio(1);
		SDL_CloseAudio();

		free(dma_buffer);
		dma_size = dma_pos = 0;

		snd_inited = 0;
	}
}

void SNDDMA_BeginPainting(void)
{
	SDL_LockAudio();
}

void SNDDMA_Submit(void)
{
	SDL_UnlockAudio();
}

