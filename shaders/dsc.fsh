#version 150
uniform vec4 WIRE_COLS[3] = {
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(0.0, 0.0, 1.0, 1.0),
};
uniform vec4 FILL_COLS[3] = {
    vec4(1.0, 0.0, 0.0, 0.0),
    vec4(0.0, 1.0, 0.0, 0.0),
    vec4(0.0, 0.0, 1.0, 0.0),
};


in vec3 n;
flat in int fragment_status;
noperspective in vec3 dist;
out vec4 fragColor;

void main(void) {
    float d = min(dist[0], min(dist[1], dist[2]));
    float I = exp2(-100000*d*d);

    fragColor = I * WIRE_COLS[fragment_status] + (1.0 - I) * FILL_COLS[fragment_status];
}

