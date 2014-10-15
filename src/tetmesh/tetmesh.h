#ifndef TET_MESH_H
#define TET_MESH_H

#include "tetgen.h"
#include <string>

class TetMesh {

public:
	
	static TetMesh * get_from_tba();

	// Saves the current TetMesh to .node, .ele, and .face files for use
	// with TetView.  The file extensions are added automatically.
	void save(std::string object_name);

	~TetMesh();

private:
	
	int num_vertices;
	REAL * vertices;			// 3 REALs per vertex for x, y, z
	short * vertex_statuses;	// (0, 1, 2) for inside/outside/boundary
	REAL * vertex_velocities;	// 3 REALs per vertex for x, y, z
	
	int num_tets;
	int * tets;					// 4 vertex indices per tet
	
	TetMesh(int num_vertices, REAL * vertices, short * vertex_statuses,
			REAL * vertex_velocities, int num_tets, int * tets);

};

#endif
