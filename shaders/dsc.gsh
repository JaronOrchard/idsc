#version 150
layout(triangles) in;
in int vert_status[];
layout(triangle_strip, max_vertices=3) out;

uniform mat4 view_transform;
uniform mat4 perspective_transform;

flat out int fragment_status;
noperspective out vec3 dist;

// This is copied from tetmesh.cpp
int get_face_status() {
    if (vert_status[0] == 0 || vert_status[1] == 0 || vert_status[2] == 0) {
        return 0; // Tet is inside
    } else if (vert_status[0] == 1 || vert_status[1] == 1 || vert_status[2] == 1) {
        return 1; // Tet is outside
    } else {
        return 2; // Tet is on the boundary
    }
}

void main() {
    fragment_status = get_face_status();
    vec3 ab = vec3(gl_in[0].gl_Position - gl_in[1].gl_Position);
    vec3 ac = vec3(gl_in[0].gl_Position - gl_in[2].gl_Position);
    vec3 ba = vec3(gl_in[1].gl_Position - gl_in[0].gl_Position);
    vec3 bc = vec3(gl_in[1].gl_Position - gl_in[2].gl_Position);

    float ab_len = length(bc);
    float ac_len = length(ac);
    float bc_len = length(bc);

    float max_len = max(ab_len, max(ac_len, bc_len));

    dist = vec3(length(cross(ab, ac)) / (bc_len * max_len), 0, 0);
    gl_Position = perspective_transform * view_transform * gl_in[0].gl_Position;
    EmitVertex();

    dist = vec3(0, length(cross(ba, bc)) / (ac_len * max_len), 0);
    gl_Position = perspective_transform * view_transform * gl_in[1].gl_Position;
    EmitVertex();

    dist = vec3(0, 0, length(cross(ac, bc)) / (ab_len * max_len));
    gl_Position = perspective_transform * view_transform * gl_in[2].gl_Position;
    EmitVertex();
}
