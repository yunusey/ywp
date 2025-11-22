#pragma once

#define INPUT_FIFO 1
#define INPUT_PULSE 2
#define INPUT_ALSA 3

#ifndef INPUT_AUDIO_METHOD
#error "You must define INPUT_AUDIO_METHOD at compile time."
#endif

#include "input/common.h"

void create_input_thread(pthread_t *p_thread, struct audio_data *audio, int sample_rate,
                         int sample_bits);
