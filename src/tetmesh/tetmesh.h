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
    INTERFACE = 2,
    DOMAIN_BOUNDARY = 3
} status_t;

typedef enum {
    ALIVE = 0,
    DEAD = 1
} gravestone_t;

typedef enum {
    STATIC = 0,
    MOVING = 1,
    STATIC_BOUNDARY = 2
} vertex_status_t;

class TetMeshFactory;
class TetrahedralViewer;

class TetMesh {
    friend class TetMeshFactory;
    friend class TetrahedralViewer;

public:

    void evolve();

    void bind_attributes(Renderable & renderable);

    ~TetMesh();

    std::vector<REAL> vertices;         // 3 REALs per vertex for x, y, z
    std::vector<REAL> vertex_targets;   // 3 REALs per vertex for x, y, z
    std::vector<vertex_status_t> vertex_statuses;

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

    struct DistanceMovableInfo {
        DistanceMovableInfo() : distance(-1), tet_index(-1) { }
        REAL distance;
        int tet_index;
    };

    bool advect();
    void retesselate();
    bool is_coplanar(unsigned int tet_id);
    void collapse_tet(unsigned int i);
    bool is_cap(Face f, unsigned int apex);
    void calculate_plane(REAL * plane, Face f);
    DistanceMovableInfo get_distance_movable(unsigned int vertex_index, REAL * velocity);
    REAL intersect_plane(REAL * plane, REAL * vertex, REAL * velocity);

    unsigned int get_opposite_vertex(unsigned int tet_id, Face face);
    Edge get_opposite_edge(unsigned int tet_id, Edge e);
    Face get_opposite_face(unsigned int tet_id, unsigned int vert_id);
    unsigned int split_edge(Edge edge);
    int collapse_edge(Edge edge);
    bool is_movable(unsigned int v);

    REAL get_edge_length(Edge edge);
    REAL distance_between_point_and_edge(Edge edge, int vertex_index);
    Edge shortest_edge_in_set(GeometrySet<Edge> set_of_edges);
    Edge longest_edge_in_set(GeometrySet<Edge> set_of_edges);
    Edge edge_in_set_helper(GeometrySet<Edge> set_of_edges, bool shortest);
    Face largest_face_in_set(GeometrySet<Face> set_of_faces);
    GeometrySet<Edge> get_edges_from_face(Face face);
    GeometrySet<Edge> get_edges_from_tet(int tet_id);
    GeometrySet<Face> get_faces_from_tet(int tet_id);
    GeometrySet<unsigned int> get_tets_from_face(Face f);
    bool is_on_domain_boundary(unsigned int v);

    void delete_tet(unsigned int t);
    unsigned int insert_tet(unsigned int v1, unsigned int v2, unsigned int v3, unsigned int v4, status_t tet_status);
    unsigned int insert_vertex(Edge edge);
    unsigned int insert_vertex(Edge edge, unsigned int moving_vertex);
};

#endif
