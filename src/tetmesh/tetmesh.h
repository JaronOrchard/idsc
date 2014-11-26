#ifndef TET_MESH_H
#define TET_MESH_H

#include "IndexedFaceSet.h"
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
} gravestone;

class TetMesh {

public:
    
    static TetMesh * from_indexed_face_set(IndexedFaceSet & ifs);
    static TetMesh * create_debug_tetmesh();

    // Saves the current TetMesh to .node, .ele, and .face files for use
    // with TetView.  The file extensions are added automatically.
    void save(std::string object_name);
    void evolve();

    void bind_attributes(Renderable & renderable);

    ~TetMesh();
    
    int num_vertices;
    std::vector<REAL> vertices;         // 3 REALs per vertex for x, y, z
    std::vector<REAL> vertex_targets;   // 3 REALs per vertex for x, y, z

    status_t get_vertex_status(int vertex_index);
    
private:
    
    int num_tets;
    std::vector<int> tets;              // 4 vertex indices per tet
    std::vector<status_t> tet_statuses;

    std::vector<gravestone> vertex_gravestones; // ALIVE or DEAD per vertex
    std::vector<gravestone> tet_gravestones;    // ALIVE or DEAD per tet

    std::vector< GeometricSet<int> > vertex_to_tet; // Each vertex has a set of neighboring tets

    std::vector< std::pair<int, int> > pending_subdivisions; // pair<tet index, opposite vertex>
    
    struct DistanceMovableInfo {
        DistanceMovableInfo() : distance(-1), tet_index(-1) { }
        REAL distance;
        int tet_index;
    };

    TetMesh(int num_vertices, std::vector<REAL> vertices, std::vector<REAL> vertex_velocities,
            int num_tets, std::vector<int> tets, std::vector<status_t> tet_statuses,
            std::vector<gravestone> vertex_gravestones, std::vector<gravestone> tet_gravestones);
    bool advect();
    void retesselate();
    DistanceMovableInfo get_distance_movable(int vertex_index, REAL * velocity);
    void calculate_plane(REAL * plane, REAL * v1, REAL * v2, REAL * v3);
    REAL intersect_plane(REAL * plane, REAL * vertex, REAL * velocity);
    
};

#endif
