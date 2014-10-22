#ifndef TET_MESH_H
#define TET_MESH_H

#include "IndexedFaceSet.h"
#include "tetgen.h"
#include <string>

class TetMesh {

public:
    
    static TetMesh * from_indexed_face_set(IndexedFaceSet & ifs);

    // Saves the current TetMesh to .node, .ele, and .face files for use
    // with TetView.  The file extensions are added automatically.
    void save(std::string object_name);
    void evolve();

    ~TetMesh();
    
    int num_vertices;
    REAL * vertices;            // 3 REALs per vertex for x, y, z
    REAL * vertex_targets;   // 3 REALs per vertex for x, y, z
    short * vertex_statuses;    // (0, 1, 2) for inside/outside/boundary

private:
    
    int num_tets;
    int * tets;                 // 4 vertex indices per tet
    
    TetMesh(int num_vertices, REAL * vertices, short * vertex_statuses,
            REAL * vertex_velocities, int num_tets, int * tets);
    short get_face_boundary_marker(int vertex1, int vertex2, int vertex3);
    bool advect();
    void retesselate();
    REAL get_distance_movable(int vertex_index, REAL * velocity);
    void calculate_plane(REAL * plane, REAL * v1, REAL * v2, REAL * v3);
    REAL intersect_plane(REAL * plane, REAL * vertex, REAL * velocity);
};

#endif
