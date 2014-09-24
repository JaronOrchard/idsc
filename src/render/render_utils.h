
#ifndef _RENDER_UTILS_H
#define _RENDER_UTILS_H

#include <GL/glew.h>

inline void _check_gl_error(const char *file, int line) {
    GLenum err = glGetError();

    while (err != GL_NO_ERROR) {
        char * error;

        switch(err) {
            case GL_INVALID_OPERATION:
                error="GL_INVALID_OPERATION";
                break;
            case GL_INVALID_ENUM:
                error="GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error="GL_INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                error="GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error="GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        fprintf(stderr, "%s - %s:%i\n", error, file, line);
        err=glGetError();
    }
}

#define check_gl_error() _check_gl_error(__FILE__,__LINE__)

#endif
