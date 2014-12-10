
#include "geometry.h"

Edge::Edge(unsigned int vertex1, unsigned int vertex2) {
    if (vertex1 < vertex2) {
        v1 = vertex1;
        v2 = vertex2;
    } else {
        v1 = vertex2;
        v2 = vertex1;
    }
}

Face::Face(unsigned int vertex1, unsigned int vertex2, unsigned int vertex3) {
    v1 = vertex1;
    v2 = vertex2;
    v3 = vertex3;
    unsigned int temp;
    if (v1 > v2) {
        temp = v1;
        v1 = v2;
        v2 = temp;
    }
    if (v1 > v3) {
        temp = v1;
        v1 = v3;
        v3 = temp;
    }
    if (v2 > v3) {
        temp = v2;
        v2 = v3;
        v3 = temp;
    }
}
