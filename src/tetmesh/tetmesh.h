#ifndef TET_MESH_H
#define TET_MESH_H

#include "model/IndexedFaceSet.h"
#include "util/geometry.h"
#include "util/geometrySet.h"
#include "tetgen.h"
#include <string>
#include <vector>

typedef enum {
    INSIDE = 0,
    OUTSIDE = 1,
    INTERFACE = 2
} status_t;

typedef enum {
    ALIVE = 0,
    DEAD = 1
} gravestone_t;

class TetMesh {

public:
    
    static TetMesh * from_indexed_face_set(IndexedFaceSet & ifs);
    static TetMesh * create_debug_tetmesh();

    void evolve();

    void bind_attributes(Renderable & renderable);

    ~TetMesh();

    std::vector<REAL> vertices;         // 3 REALs per vertex for x, y, z
    std::vector<REAL> vertex_targets;   // 3 REALs per vertex for x, y, z

    status_t get_vertex_status(unsigned int vertex_index);
    
private:
    
    std::vector<unsigned int> tets;              // 4 vertex indices per tet
    std::vector<status_t> tet_statuses;

    std::vector<gravestone_t> vertex_gravestones; // ALIVE or DEAD per vertex
    std::vector<gravestone_t> tet_gravestones;    // ALIVE or DEAD per tet

    std::vector< GeometrySet<unsigned int> > vertex_tet_map; // Each vertex has a set of neighboring tets

    TetMesh(std::vector<REAL> vertices, std::vector<REAL> vertex_targets,
            std::vector<unsigned int> tets, std::vector<status_t> tet_statuses,
            std::vector<GeometrySet<unsigned int>> vertex_tet_map);
    bool advect();
    void retesselate();
    REAL get_distance_movable(unsigned int vertex_index, REAL * velocity);
    void calculate_plane(REAL * plane, Face f);
    REAL intersect_plane(REAL * plane, REAL * vertex, REAL * velocity);

    Edge get_opposite_edge(unsigned int tet_id, Edge e);
    Face get_opposite_face(unsigned int tet_id, unsigned int vert_id);
    void split_edge(Edge edge);
    void collapse_edge(Edge edge);

    REAL distance_between_point_and_edge(Edge edge, int vertex_index);
    Edge longest_edge_in_set(GeometrySet<Edge> set_of_edges);

    void delete_tet(unsigned int t);
    unsigned int insert_tet(unsigned int v1, unsigned int v2, unsigned int v3, unsigned int v4, status_t tet_status);
    unsigned int insert_vertex(Edge edge);
    unsigned int get_opposite_vertex( Face face );
};

#endif
