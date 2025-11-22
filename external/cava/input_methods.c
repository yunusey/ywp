#include "input_methods.h"
#include "input/common.h"

#ifdef __GNUC__
// curses.h or other sources may already define
#undef GCC_UNUSED
#define GCC_UNUSED __attribute__((unused))
#else
#define GCC_UNUSED /* nothing */
#endif

#if INPUT_AUDIO_METHOD == INPUT_PULSE
#include "input/pulse.h"
#endif

// Declare the backends (all exist in input/*.c)
void *input_fifo(void *arg);
void *input_pulse(void *arg);
void *input_alsa(void *arg);

void create_input_thread(pthread_t *p_thread, struct audio_data *audio, int sample_rate,
                         int sample_bits) {
    audio->format = -1;
    audio->rate = 0;
    audio->samples_counter = 0;
    audio->channels = 2;
    audio->IEEE_FLOAT = 0;
    audio->autoconnect = 0;

    audio->input_buffer_size = BUFFER_SIZE * audio->channels;
    audio->cava_buffer_size = 16384;

    audio->cava_in = (double *)malloc(audio->cava_buffer_size * sizeof(double));
    memset(audio->cava_in, 0, audio->cava_buffer_size * sizeof(double));

    audio->threadparams = 0;
    audio->terminate = 0;

    int thr_id GCC_UNUSED;

    pthread_mutex_init(&audio->lock, NULL);

#if INPUT_AUDIO_METHOD == INPUT_FIFO
    audio->rate = sample_rate;
    audio->format = sample_bits;
    thr_id = pthread_create(p_thread, NULL, input_fifo, audio);

#elif INPUT_AUDIO_METHOD == INPUT_ALSA
    thr_id = pthread_create(p_thread, NULL, input_alsa, (void *)&audio);

#elif INPUT_AUDIO_METHOD == INPUT_PULSE
    audio->format = 16;
    audio->rate = 44100;
    audio->source = strdup("auto");

    getPulseDefaultSink(audio);

    thr_id = pthread_create(p_thread, NULL, input_pulse, (void *)audio);

#endif
}
