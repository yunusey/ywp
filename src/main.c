#include "input_methods.h"
#include "spline.frag.h"
#include "spline.vert.h"
#include <pthread.h>
#include <assert.h>
#include "cavacore.h"
#include "platform.h"
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <stdio.h>
#include <stdlib.h>

GLuint compile_shader(GLenum type, const char *source, GLint length) {
    GLuint shader = glCreateShader(type);

    // FIX: Pass the length to glShaderSource
    // This tells it to read exactly 'length' bytes
    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }
    return shader;
}

GLuint
create_shader_program(const char *vertex_source, GLint vertex_len, const char *fragment_source, GLint fragment_len) {
    // FIX: Pass the lengths to compile_shader
    GLuint vertex_shader   = compile_shader(GL_VERTEX_SHADER, vertex_source, vertex_len);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source, fragment_len);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    GLint success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

void construct_projection_matrix(
    float *matrix, float left, float right, float bottom, float top, float near, float far) {
    // Initialize to zero
    for (int i = 0; i < 16; i++) {
        matrix[i] = 0.0f;
    }

    matrix[0]  = 2.0f / (right - left);
    matrix[5]  = 2.0f / (top - bottom);
    matrix[10] = -2.0f / (far - near);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(far + near) / (far - near);
    matrix[15] = 1.0f;
}

int main(int argc, char **argv) {
    init_platform();

    struct audio_data audio_data = {0};
    memset(&audio_data, 0, sizeof(struct audio_data));

    pthread_t audio_thread;
    create_input_thread(&audio_thread, &audio_data, 44100, 16);

    int bars_per_channel = 8;

    pthread_mutex_lock(&audio_data.lock);
    struct cava_plan *plan = cava_init(bars_per_channel, audio_data.rate, audio_data.channels, 1, 0.77, 50, 8000);
    pthread_mutex_unlock(&audio_data.lock);

    if (plan->status != 0) {
        printf("Error initializing cava: %s\n", plan->error_message);
        return -1;
    }

    char *vertex_source   = (char *) shaders_spline_vert;
    char *fragment_source = (char *) shaders_spline_frag;

    GLuint shader_program = create_shader_program(
        vertex_source, (GLint) shaders_spline_vert_len, fragment_source, (GLint) shaders_spline_frag_len);
    glUseProgram(shader_program);

    float projection_matrix[16];
    construct_projection_matrix(
        &projection_matrix[0], 0.0f, (float) core.window_size.width, 0.0f, (float) core.window_size.height, -1.0f,
        1.0f);
    GLint proj_location = glGetUniformLocation(shader_program, "u_projection");
    glUniformMatrix4fv(proj_location, 1, GL_FALSE, projection_matrix);

    GLint viewport_location = glGetUniformLocation(shader_program, "u_viewport");
    glUniform2f(viewport_location, (float) core.window_size.width, (float) core.window_size.height);

    GLint num_bars_location = glGetUniformLocation(shader_program, "u_num_bars");
    glUniform1i(num_bars_location, bars_per_channel * audio_data.channels);

    GLint thickness_location = glGetUniformLocation(shader_program, "u_thickness");
    glUniform1f(thickness_location, 10.0f);

    // Our spline buffer
    // We assume that the number of bars per channel is known and fixed so that we can allocate the buffer once
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, sizeof(double) * bars_per_channel * audio_data.channels, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    double *cava_out   = (double *) malloc(sizeof(double) * bars_per_channel * audio_data.channels);
    float  *cava_out_f = (float *) malloc(sizeof(float) * bars_per_channel * audio_data.channels);
    while (true) {
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

        for (int i = 0; i < bars_per_channel * audio_data.channels; i++) {
            cava_out_f[i] = (float) cava_out[i];
        }

        // Update SSBO with cava_out data
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferSubData(
            GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * bars_per_channel * audio_data.channels, cava_out_f);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        eglSwapBuffers(platform.egl.device, platform.egl.surface);

        usleep(10000);
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
