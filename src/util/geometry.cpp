
#include "geometry.h"

Edge::Edge(unsigned int vertex1, unsigned int vertex2) {
    v1 = vertex1;
    v2 = vertex2;
}

Face::Face(unsigned int vertex1, unsigned int vertex2, unsigned int vertex3) {
    v1 = vertex1;
    v2 = vertex2;
    v3 = vertex3;
}
