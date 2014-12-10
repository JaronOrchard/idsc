
#ifndef GEOMETRY_H
#define GEOMETRY_H

class Edge {

public:
    Edge(unsigned int vertex1, unsigned int vertex2);

    unsigned int getV1() { return v1; }
    unsigned int getV2() { return v2; }

    bool operator==(const Edge & e) const {
        return e.v1 == v1 && e.v2 == v2;
    }

    bool contains(unsigned int v) {
        return v == v1 || v == v2;
    }

private:
    unsigned int v1, v2; // The two vertices of the edge, in sorted order

};

class Face {

public:
    Face(unsigned int vertex1, unsigned int vertex2, unsigned int vertex3);

    unsigned int getV1() { return v1; }
    unsigned int getV2() { return v2; }
    unsigned int getV3() { return v3; }

    bool operator==(const Face & f) const {
        return f.v1 == v1 && f.v2 == v2 && f.v3 == v3;
    }

    bool contains(unsigned int v) {
        return v == v1 || v == v2 || v == v3;
    }

    bool contains(Edge e) {
        return contains(e.getV1()) && contains(e.getV2());
    }

private:
    unsigned int v1, v2, v3; // The three vertices of the face, in sorted order

};

#endif