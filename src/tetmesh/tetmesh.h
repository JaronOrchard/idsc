#ifndef TET_MESH_H
#define TET_MESH_H

#include "IndexedFaceSet.h"
#include "tetgen.h"
#include <string>
#include <vector>

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
	std::vector<short> vertex_statuses;	// 0 for inside, 1 for outside, 2 for boundary
	
private:
    
    int num_tets;
	std::vector<int> tets;              // 4 vertex indices per tet

	std::vector< std::pair<int, int> > pending_subdivisions; // pair<tet index, opposite vertex>
    
	struct DistanceMovableInfo {
		DistanceMovableInfo() : distance(-1), tet_index(-1) { }
		REAL distance;
		int tet_index;
	};

    TetMesh(int num_vertices, std::vector<REAL> vertices, std::vector<short> vertex_statuses,
            std::vector<REAL> vertex_velocities, int num_tets, std::vector<int> tets);
    short get_face_boundary_marker(int vertex1, int vertex2, int vertex3);
    bool advect();
    void retesselate();
    DistanceMovableInfo get_distance_movable(int vertex_index, REAL * velocity);
    void calculate_plane(REAL * plane, REAL * v1, REAL * v2, REAL * v3);
    REAL intersect_plane(REAL * plane, REAL * vertex, REAL * velocity);
	std::vector<int> get_other_vertices(int tet, int vertex);
	int get_opposite_tet(int tet, int vertex);
	int get_opposite_vertex(int tet, int vertex);
    int create_new_vertex_in_tet_center(int tet);
	void subdivide_opposite_tet(int tet, int vertex);
    
};

#endif
