
#version 330 core

in vec3 vertex_position;
in float vertex_status;
uniform mat4 MVP;
out int vert_status;

void main() {
    gl_Position = MVP * vec4(vertex_position, 1.0);
    gl_Position = gl_Position / gl_Position.w;
    vert_status = int(vertex_status);
}
