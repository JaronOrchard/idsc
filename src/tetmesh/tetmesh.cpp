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

// assumes interface will not evolve beyod [-10, 10]^3 and encloses the point (0, 0, 0)
TetMesh * TetMesh::from_indexed_face_set(IndexedFaceSet & ifs) {
    tetgenio * inner_input = IndexedFaceSet::to_tetgenio(ifs);

    tetgenio * outer_input = IndexedFaceSet::to_tetgenio(ifs);
    //add outter cube vertices
    int orig_num_v = outer_input->numberofpoints;
    outer_input->numberofpoints += 8;
    REAL * new_vertices = new REAL[outer_input->numberofpoints * 3];
    for (int i = 0; i < orig_num_v * 3; i++) {
        new_vertices[i] = outer_input->pointlist[i];
    }

    static const REAL cube_corners[] = {
        10, 10, 10,
        10, 10, -10,
        10, -10, 10,
        10, -10, -10,
        -10, 10, 10,
        -10, 10, -10,
        -10, -10, 10,
        -10, -10, -10
    };

    for (int i = 0; i < 8 * 3; i++) {
        new_vertices[orig_num_v * 3 + i] = cube_corners[i];
    }

    delete outer_input->pointlist;
    outer_input->pointlist = new_vertices;

    //add outter cube faces
    int orig_num_f = outer_input->numberoffacets;
    outer_input->numberoffacets += 6;
    tetgenio::facet * new_facets = new tetgenio::facet[outer_input->numberoffacets];
    delete outer_input->facetmarkerlist;
    outer_input->facetmarkerlist = new int[outer_input->numberoffacets];
    for (int i = 0; i < orig_num_f; i++) {
        new_facets[i] = outer_input->facetlist[i];
    }

    static const int cube_faces[] = {
        0, 1, 3, 2,
        4, 5, 7, 6,
        0, 1, 5, 4,
        2, 3, 7, 6,
        0, 2, 6, 4,
        1, 3, 7, 5
    };

    for (int i = 0; i < 6; i++) {
        new_facets[orig_num_f + i].numberofholes = 0;
        new_facets[orig_num_f + i].holelist = NULL;
        new_facets[orig_num_f + i].numberofpolygons = 1;
        new_facets[orig_num_f + i].polygonlist = new tetgenio::polygon[1];
        new_facets[orig_num_f + i].polygonlist[0].numberofvertices = 4;
        new_facets[orig_num_f + i].polygonlist[0].vertexlist = new int[4];
        new_facets[orig_num_f + i].polygonlist[0].vertexlist[0] = orig_num_v + cube_faces[i * 4];
        new_facets[orig_num_f + i].polygonlist[0].vertexlist[1] = orig_num_v + cube_faces[i * 4 + 1];
        new_facets[orig_num_f + i].polygonlist[0].vertexlist[2] = orig_num_v + cube_faces[i * 4 + 2];
        new_facets[orig_num_f + i].polygonlist[0].vertexlist[3] = orig_num_v + cube_faces[i * 4 + 3];
    }

    delete outer_input->facetlist;
    outer_input->facetlist = new_facets;

    // add hole at (0, 0, 0)
    outer_input->numberofholes = 1;
    outer_input->holelist = new REAL[3];
    outer_input->holelist[0] = 0;
    outer_input->holelist[1] = 0;
    outer_input->holelist[2] = 0;

    tetgenio inner_output;
    tetgenio outer_output;
    tetgenbehavior switches;
    switches.parse_commandline("pYq");
    tetrahedralize(&switches, inner_input, &inner_output);
    tetrahedralize(&switches, outer_input, &outer_output);

    int outer_num_v = outer_output.numberofpoints - orig_num_v;
    int inner_num_v = inner_output.numberofpoints - orig_num_v;

    int num_v = orig_num_v + outer_num_v + inner_num_v;
    REAL * vertices = new REAL[num_v * 3];
    REAL * velocities = new REAL[num_v * 3];
    short * statuses = new short[num_v];

    for (int i = 0; i < num_v * 3; i++) {
        velocities[i] = 0;
    }

    for (int i = 0; i < orig_num_v; i++) {
        vertices[i * 3] = inner_output.pointlist[i * 3];
        vertices[i * 3 + 1] = inner_output.pointlist[i * 3 + 1];
        vertices[i * 3 + 2] = inner_output.pointlist[i * 3 + 2];
        statuses[i] = 2;
    }
    for (int i = orig_num_v; i < orig_num_v + inner_num_v; i++) {
        vertices[i * 3] = inner_output.pointlist[i * 3];
        vertices[i * 3 + 1] = inner_output.pointlist[i * 3 + 1];
        vertices[i * 3 + 2] = inner_output.pointlist[i * 3 + 2];
        statuses[i] = 0;
    }
    for (int i = orig_num_v; i < orig_num_v + outer_num_v; i++) {
        vertices[(i + inner_num_v) * 3] = outer_output.pointlist[i * 3];
        vertices[(i + inner_num_v) * 3 + 1] = outer_output.pointlist[i * 3 + 1];
        vertices[(i + inner_num_v) * 3 + 2] = outer_output.pointlist[i * 3 + 2];
        statuses[i] = 1;
    }

    int inner_num_t = inner_output.numberoftetrahedra;
    int num_t = outer_output.numberoftetrahedra + inner_num_t;
    int * tetrahedra = new int[num_t * 4];
    for (int i = 0; i < inner_num_t; i++) {
        for (int j = 0; j < 4; j++) {
            tetrahedra[i * 4 + j] = inner_output.tetrahedronlist[i * 4 + j];
        }
    }

    for (int i = 0; i < outer_output.numberoftetrahedra; i++) {
        for (int j = 0; j < 4; j++) {
            tetrahedra[(i + inner_num_t) * 4 + j] = outer_output.tetrahedronlist[i * 4 + j];
            if (outer_output.tetrahedronlist[i * 4 + j] > orig_num_v) {
                tetrahedra[(i + inner_num_t) * 4 + j] += inner_num_v;
            }
        }
    }

    delete inner_input;
    delete outer_input;

    return new TetMesh(num_v, vertices, statuses, velocities, num_t, tetrahedra);
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

