
#include "Shader.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <streambuf>

Shader::Shader(int id) {
    this->id = id;
}

int Shader::get_id() {
    return id;
}

Shader * Shader::compile_from(char * v_file, char * f_file) {
    int id = load_shaders(v_file, NULL, f_file);
    return new Shader(id);
}

Shader * Shader::compile_from(char * v_file, char * g_file, char * f_file) {
    int id = load_shaders(v_file, g_file, f_file);
    return new Shader(id);
}

GLuint Shader::load_shader(char * file_name, GLenum shader_type) {
    GLuint id = glCreateShader(shader_type);
    std::string shader_code;
    std::ifstream file_stream(file_name);
    if (file_stream.fail()) {
        fprintf(stderr, "could not find shader file %s\n", file_name);
        throw 1;
    }

    file_stream.seekg(0, std::ios::end);
    shader_code.reserve(file_stream.tellg());
    file_stream.seekg(0, std::ios::beg);

    shader_code.assign((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());

    char const * source_pointer = shader_code.c_str();
    glShaderSource(id, 1, &source_pointer , NULL);
    glCompileShader(id);

    int log_length;
    GLint result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 1) {
        char * error_message = (char *) calloc(log_length + 1, sizeof(char));
        glGetShaderInfoLog(id, log_length, NULL, error_message);
        fprintf(stderr, "error when compiling shader file %s\n", file_name);
        fprintf(stderr, "%s\n", error_message);
        free(error_message);
        throw 1;
    }

    return id;
}

GLuint Shader::load_shaders(char * v_file, char * g_file, char * f_file) {
    GLuint program_id = glCreateProgram();

    GLuint v_id = load_shader(v_file, GL_VERTEX_SHADER);
    glAttachShader(program_id, v_id);
    GLuint g_id;
    if (g_file != NULL) {
        g_id = load_shader(g_file, GL_GEOMETRY_SHADER);
        glAttachShader(program_id, g_id);
    }
    GLuint f_id = load_shader(f_file, GL_FRAGMENT_SHADER);
    glAttachShader(program_id, f_id);

    glLinkProgram(program_id);

    int log_length;
    GLint result = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 1){
        char * error_message = (char *) calloc(log_length + 1, sizeof(char));
        glGetProgramInfoLog(program_id, log_length, NULL, error_message);
        printf("%s\n", error_message);
        free(error_message);
    }

    glDeleteShader(v_id);
    if (g_file != NULL) {
        glDeleteShader(g_id);
    }
    glDeleteShader(f_id);

    return program_id;
}
