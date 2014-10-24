#version 150

in vec3 n;
in vec3 color;
noperspective in vec3 dist;
out vec4 fragColor;

const float dmax = 1.8;

void main(void) {
    float d = min(dist[0], min(dist[1], dist[2]));
    float I = exp2(-2*d*d);
    if (I > dmax) {
        I = 0;
    }

    fragColor = I * vec4(color, 1.0) + (1.0 - I) * vec4(color, 0.01);
}

