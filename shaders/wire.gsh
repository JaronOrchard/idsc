#version 150
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

noperspective out vec3 dist;

void main() {
    vec3 ab = vec3(gl_in[0].gl_Position - gl_in[1].gl_Position);
    vec3 ac = vec3(gl_in[0].gl_Position - gl_in[2].gl_Position);
    vec3 ba = vec3(gl_in[1].gl_Position - gl_in[0].gl_Position);
    vec3 bc = vec3(gl_in[1].gl_Position - gl_in[2].gl_Position);

    float ab_len = length(bc);
    float ac_len = length(ac);
    float bc_len = length(bc);

    float max_len = max(ab_len, max(ac_len, bc_len));

    dist = vec3(length(cross(ab, ac)) / (bc_len * max_len), 0, 0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    dist = vec3(0, length(cross(ba, bc)) / (ac_len * max_len), 0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    dist = vec3(0, 0, length(cross(ac, bc)) / (ab_len * max_len));
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}
