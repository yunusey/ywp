#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cavacore.h"
#include "input_methods.h"
#include "platform.h"
#include "shader.h"

#define TARGET_FPS 30

float get_monotonic_time()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (float)ts.tv_sec + (float)ts.tv_nsec / 1e9f;
}

int main(int argc, char **argv)
{
    init_platform();

    struct audio_data audio_data = {0};
    memset(&audio_data, 0, sizeof(struct audio_data));

    pthread_t audio_thread;
    create_input_thread(&audio_thread, &audio_data, 44100, 16);

    int bars_per_channel = 8;

    struct cava_plan *plan = cava_init(bars_per_channel, audio_data.rate, audio_data.channels, 0, 0.77, 50, 8000);
    if (plan->status != 0)
    {
        printf("Error initializing cava: %s\n", plan->error_message);
        return -1;
    }

    // char *vertex_source = (char *)shaders_spline_vert;
    // GLint vertex_len = (GLint)shaders_spline_vert_len;
    // char *fragment_source = (char *)shaders_spline_frag;
    // GLint fragment_len = (GLint)shaders_spline_frag_len;

    char *vertex_source = (char *)shaders_circular_vert;
    GLint vertex_len = (GLint)shaders_circular_vert_len;
    char *fragment_source = (char *)shaders_circular_frag;
    GLint fragment_len = (GLint)shaders_circular_frag_len;

    GLuint shader_program = create_shader_program(vertex_source, vertex_len, fragment_source, fragment_len);
    glUseProgram(shader_program);

    float projection_matrix[16];
    construct_projection_matrix(projection_matrix, 0.0f, (float)core.window_size.width, 0.0f,
                                (float)core.window_size.height, -1.0f, 1.0f);
    GLint proj_location = glGetUniformLocation(shader_program, "u_projection");
    glUniformMatrix4fv(proj_location, 1, GL_FALSE, projection_matrix);

    GLint viewport_location = glGetUniformLocation(shader_program, "u_viewport");
    glUniform2f(viewport_location, (float)core.window_size.width, (float)core.window_size.height);

    GLint num_bars_location = glGetUniformLocation(shader_program, "u_num_bars");
    glUniform1i(num_bars_location, bars_per_channel * audio_data.channels);

    GLint time_location = glGetUniformLocation(shader_program, "u_time");

    // Our spline buffer
    // We assume that the number of bars per channel is known and fixed so that
    // we can allocate the buffer once
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(double) * bars_per_channel * audio_data.channels, NULL,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    double *cava_out = (double *)malloc(sizeof(double) * bars_per_channel * audio_data.channels);
    assert(cava_out != NULL);

    const float frame_time = 1.0f / TARGET_FPS;
    const float start_time = get_monotonic_time();
    float last_iteration_time = 0.0f;
    while (true)
    {
        wl_display_dispatch_pending(platform.display);
        wl_display_flush(platform.display);

        pthread_mutex_lock(&audio_data.lock);
        cava_execute(audio_data.cava_in, audio_data.samples_counter, cava_out, plan);
        if (audio_data.samples_counter > 0)
            audio_data.samples_counter = 0;
        pthread_mutex_unlock(&audio_data.lock);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(core.window_size.width, 0.0f);
        glVertex2f(core.window_size.width, core.window_size.height);
        glVertex2f(0.0f, core.window_size.height);
        glEnd();

        // Update SSBO with cava_out data
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(double) * bars_per_channel * audio_data.channels, cava_out);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Update time uniform
        float current_time = get_monotonic_time() - start_time;
        printf("Time: %f\n", current_time);
        glUniform1f(time_location, current_time);

        eglSwapBuffers(platform.egl.device, platform.egl.surface);

        // We want to run at TARGET_FPS
        current_time = get_monotonic_time() - start_time;
        float elapsed_time = current_time - last_iteration_time;
        if (elapsed_time < frame_time)
        {
            float time_to_sleep = frame_time - elapsed_time;
            usleep((useconds_t)(time_to_sleep * 1e6f));
        }
        last_iteration_time = current_time;
    }
    close_platform();

    pthread_mutex_lock(&audio_data.lock);
    audio_data.terminate = 1;
    pthread_mutex_unlock(&audio_data.lock);
    pthread_join(audio_thread, NULL);

    free(audio_data.source);
    free(audio_data.cava_in);

    cava_destroy(plan);
    free(plan);
    free(cava_out);

    return 0;
}
