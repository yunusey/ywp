#include "input_methods.h"
#include <pthread.h>
#include "cavacore.h"
#include "platform.h"
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    init_platform();

    eglMakeCurrent(platform.egl.device, platform.egl.surface, platform.egl.surface, platform.egl.context);
    glViewport(0, 0, 800, 600);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 0, 600, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    struct audio_data audio_data = {0};
    memset(&audio_data, 0, sizeof(struct audio_data));

    pthread_t audio_thread;
    create_input_thread(&audio_thread, &audio_data, 44100, 16);

    pthread_mutex_lock(&audio_data.lock);
    struct cava_plan *plan
        = cava_init(64 / audio_data.channels, audio_data.rate, audio_data.channels, 1, 0.7, 50, 10000);
    pthread_mutex_unlock(&audio_data.lock);

    if (plan->status != 0) {
        printf("Error initializing cava: %s\n", plan->error_message);
        return -1;
    }

    double *cava_out = (double *) malloc(sizeof(double) * plan->number_of_bars);
    while (true) {
        wl_display_dispatch_pending(platform.display);
        // wl_display_flush(platform.display);

        pthread_mutex_lock(&audio_data.lock);
        cava_execute(audio_data.cava_in, audio_data.samples_counter, cava_out, plan);
        if (audio_data.samples_counter > 0)
            audio_data.samples_counter = 0;
        pthread_mutex_unlock(&audio_data.lock);

        // Randomize cava_out for testing
        // for (int i = 0; i < plan->number_of_bars; i++) {
        //     cava_out[i] = rand() % 100 / 100.0;
        // }

        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Simple visualization: draw bars for each frequency bar
        int width = 800 / plan->number_of_bars;
        for (int i = 0; i < plan->number_of_bars; i++) {
            float height = (float) (cava_out[i]) * 600.0f;
            if (height > 600.0f)
                height = 600.0f;

            glBegin(GL_QUADS);
            glColor3f(0.2f, 0.7f, 0.4f);
            glVertex2f(i * width, 0);
            glVertex2f(i * width + width - 2, 0);
            glVertex2f(i * width + width - 2, height);
            glVertex2f(i * width, height);
            glEnd();
        }

        eglSwapBuffers(platform.egl.device, platform.egl.surface);

        usleep(10000);
    }
    close_platform();

    free(cava_out);

    return 0;
}
