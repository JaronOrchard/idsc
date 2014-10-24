
#version 330 core

in vec3 vertex_position;
in int vertex_status;
uniform mat4 model_transform;
out int vert_status;

void main() {
    gl_Position = model_transform * vec4(vertex_position, 1.0);
    vert_status = vertex_status;
}
