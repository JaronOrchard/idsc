#version 150
const vec4 WIRE_COL = vec4(1.0,0.0,1.0,1.0);
const vec4 FILL_COL = vec4(1.0,0.0,1.0,0.0);

in vec3 n;
noperspective in vec3 dist;
out vec4 fragColor;

void main(void) {
    float d = min(dist[0], min(dist[1], dist[2]));
    float I = exp2(-1000*d*d);

    fragColor = I * WIRE_COL + (1.0 - I) * FILL_COL;
}

