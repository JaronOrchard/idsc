


#ifndef _SHADER_H
#define _SHADER_H

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

class Shader {
    public:
        static Shader * compile_from(char * v_file, char * f_file);
        static Shader * compile_from(char * v_file, char * g_file, char * f_file);
        int get_id();
    private:
        Shader(int id);
        static GLuint load_shader(char * file_name, GLenum shader_type);
        static GLuint load_shaders(char * v_file, char * g_file, char * f_file);
        int id;
};

#endif