#include "shader.h"
#include <stdio.h>

// Include the shader sources
#include "circular.frag.h"
#include "circular.vert.h"
#include "spline.frag.h"
#include "spline.vert.h"

GLuint compile_shader(GLenum type, const char *source, GLint length)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }
    return shader;
}

GLuint create_shader_program(const char *vertex_source, GLint vertex_len, const char *fragment_source,
                             GLint fragment_len)
{
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source, vertex_len);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source, fragment_len);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    GLint success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

void construct_projection_matrix(float matrix[16], float left, float right, float bottom, float top, float near,
                                 float far)
{
    for (int i = 0; i < 16; i++)
    {
        matrix[i] = 0.0f;
    }

    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -2.0f / (far - near);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(far + near) / (far - near);
    matrix[15] = 1.0f;
}
