#include "tetmesh.h"
#include <string>
#include <fstream>
#include <iostream>

TetMesh::TetMesh(int num_vertices, REAL * vertices, short * vertex_statuses,
			REAL * vertex_velocities, int num_tets, int * tets) {
	this->num_vertices = num_vertices;
	this->vertices = vertices;
	this->vertex_statuses = vertex_statuses;
	this->vertex_velocities = vertex_velocities;
	this->num_tets = num_tets;
	this->tets = tets;
}

TetMesh::~TetMesh() {
	if (vertices) { delete [] vertices; }
	if (vertex_statuses) { delete [] vertex_statuses; }
	if (vertex_velocities) { delete [] vertex_velocities; }
	if (tets) { delete [] tets; }
}

TetMesh * TetMesh::get_from_tba() {
	// example use of private constructor:
	return new TetMesh(0, NULL, NULL, NULL, 0, NULL);
}

void TetMesh::save(std::string object_name) {
	// .node file:
	std::ofstream fout_node(object_name + ".node");
	if (fout_node.fail()) {
		std::cout << "Could not open " << object_name << ".node for writing!" << std::endl;
		return;
	}
	// === OUTPUT .NODE CONTENTS HERE
	fout_node.close();
	std::cout << object_name << ".node saved successfully." << std::endl;
		
	// .ele file:
	std::ofstream fout_ele(object_name + ".ele");
	if (fout_ele.fail()) {
		std::cout << "Could not open " << object_name << ".ele for writing!" << std::endl;
		return;
	}
	// === OUTPUT .ELE CONTENTS HERE
	fout_ele.close();
	std::cout << object_name << ".ele saved successfully." << std::endl;
	
	// .face file:
	std::ofstream fout_face(object_name + ".face");
	if (fout_face.fail()) {
		std::cout << "Could not open " << object_name << ".face for writing!" << std::endl;
		return;
	}
	// === OUTPUT .FACE CONTENTS HERE
	fout_face.close();
	std::cout << object_name << ".face saved successfully." << std::endl;
	
}

