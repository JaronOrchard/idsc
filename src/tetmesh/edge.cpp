#include "edge.h"

Edge::Edge(unsigned int vertex1, unsigned int vertex2) {
    if (vertex1 < vertex2) {
        v1 = vertex1;
        v2 = vertex2;
    } else {
        v1 = vertex2;
        v2 = vertex1;
    }
}

edge_id Edge::getEdgeId() {
    edge_id id = 0; // edge_id is an unsigned long long
    id |= (uint32_t)v1;
    id = id << 32;
    id |= (uint32_t)v2;
    return id;
}
