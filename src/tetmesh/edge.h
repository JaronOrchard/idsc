#ifndef EDGE_H
#define EDGE_H

typedef unsigned long long edge_id;

class Edge {

public:
    Edge(unsigned int vertex1, unsigned int vertex2);

    unsigned int getV1() { return v1; }
    unsigned int getV2() { return v2; }
    edge_id getEdgeId();

private:
    unsigned int v1, v2; // The two vertices of the edge, in sorted order

};

#endif
