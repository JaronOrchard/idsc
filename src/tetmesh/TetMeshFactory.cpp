
#include "TetMeshFactory.h"

#include <vector>

#include "util/vec.h"

// assumes interface will not evolve beyond [-10, 10]^3 and encloses the point (0, 0, 0)
TetMesh * TetMeshFactory::from_indexed_face_set(IndexedFaceSet & ifs) {
    tetgenio * inner_input = IndexedFaceSet::to_tetgenio(ifs);

    tetgenio * outer_input = IndexedFaceSet::to_tetgenio(ifs);
    //add outer cube vertices
    int orig_num_v = outer_input->numberofpoints;
    outer_input->numberofpoints += 8;
    REAL * new_vertices = new REAL[outer_input->numberofpoints * 3];
    for (int i = 0; i < orig_num_v * 3; i++) {
        new_vertices[i] = outer_input->pointlist[i];
    }

    // TODO: for robustness, generate size based on bounding sphere
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

    delete[] outer_input->pointlist;
    outer_input->pointlist = new_vertices;

    //add outer cube faces
    int orig_num_f = outer_input->numberoffacets;
    outer_input->numberoffacets += 6;
    tetgenio::facet * new_facets = new tetgenio::facet[outer_input->numberoffacets];
    delete[] outer_input->facetmarkerlist;
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

    delete[] outer_input->facetlist;
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
    switches.parse_commandline("pYqQ");
    tetrahedralize(&switches, inner_input, &inner_output);
    tetrahedralize(&switches, outer_input, &outer_output);

    int outer_num_v = outer_output.numberofpoints - orig_num_v;
    int inner_num_v = inner_output.numberofpoints - orig_num_v;

    int num_v = orig_num_v + outer_num_v + inner_num_v;
    std::vector<REAL> vertices;
    std::vector<REAL> targets;
    vertices.resize(num_v * 3);
    targets.resize(num_v * 3);
    
    for (int i = 0; i < orig_num_v + inner_num_v; i++) {
        vec_copy(&vertices[i * 3], &inner_output.pointlist[i * 3]);
    }
    for (int i = orig_num_v; i < orig_num_v + outer_num_v; i++) {
        vec_copy(&vertices[(i + inner_num_v) * 3], &outer_output.pointlist[i * 3]);
    }

    for (int i = 0; i < num_v * 3; i++) {
        targets[i] = vertices[i];
    }

    int inner_num_t = inner_output.numberoftetrahedra;
    int num_t = outer_output.numberoftetrahedra + inner_num_t;
    std::vector<unsigned int> tetrahedra;
    std::vector<status_t> statuses;
    tetrahedra.resize(num_t * 4);
    statuses.resize(num_t);
    for (int i = 0; i < inner_num_t; i++) {
        for (int j = 0; j < 4; j++) {
            tetrahedra[i * 4 + j] = inner_output.tetrahedronlist[i * 4 + j];
        }
        statuses[i] = INSIDE;
    }

    for (int i = 0; i < outer_output.numberoftetrahedra; i++) {
        for (int j = 0; j < 4; j++) {
            tetrahedra[(i + inner_num_t) * 4 + j] = outer_output.tetrahedronlist[i * 4 + j];
            if (outer_output.tetrahedronlist[i * 4 + j] >= orig_num_v) {
                tetrahedra[(i + inner_num_t) * 4 + j] += inner_num_v;
            }
        }
        statuses[i + inner_num_t] = OUTSIDE;
    }

    delete inner_input;
    delete outer_input;

    std::vector<GeometrySet<unsigned int>> vertex_to_tet(num_v);
    for (int i = 0; i < num_t; i++) {
        for (int j = 0; j < 4; j++) {
            vertex_to_tet[tetrahedra[i * 4 + j]].insert(i);
        }
    }

    return new TetMesh(vertices, targets, tetrahedra, statuses, vertex_to_tet);
}

TetMesh * TetMeshFactory::create_debug_tetmesh() {
    std::vector<REAL> vertices;
    std::vector<REAL> targets;
    std::vector<unsigned int> tetrahedra;
    std::vector<status_t> statuses;

    vertices.push_back(-3); vertices.push_back(0); vertices.push_back(0);
    vertices.push_back(0); vertices.push_back(2); vertices.push_back(0);
    vertices.push_back(0); vertices.push_back(-1); vertices.push_back(-1);
    vertices.push_back(0); vertices.push_back(-1); vertices.push_back(1);
    vertices.push_back(3); vertices.push_back(0); vertices.push_back(0);

    for (unsigned int i = 0; i < vertices.size(); i++) { targets.push_back(vertices[i]); }
    targets[0] = 1.5;

    tetrahedra.push_back(0); tetrahedra.push_back(1); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(INSIDE);

    tetrahedra.push_back(4); tetrahedra.push_back(1); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(OUTSIDE);

    std::vector<GeometrySet<unsigned int>> vertex_to_tet(vertices.size() / 3);
    for (unsigned int i = 0; i < tetrahedra.size(); i++) {
        vertex_to_tet[tetrahedra[i]].insert(i / 4);
    }
    
    return new TetMesh(vertices, targets, tetrahedra, statuses, vertex_to_tet);
}


TetMesh * TetMeshFactory::create_big_debug_tetmesh() {
    std::vector<REAL> vertices;
    std::vector<REAL> targets;
    std::vector<unsigned int> tetrahedra;
    std::vector<status_t> statuses;

    vertices.push_back(-3); vertices.push_back(0); vertices.push_back(0);
    vertices.push_back(0); vertices.push_back(2); vertices.push_back(0);
    vertices.push_back(0); vertices.push_back(-1); vertices.push_back(-1);
    vertices.push_back(0); vertices.push_back(-1); vertices.push_back(1);
    vertices.push_back(3); vertices.push_back(0); vertices.push_back(0);
    vertices.push_back(2); vertices.push_back(-2); vertices.push_back(0);
    vertices.push_back(-2); vertices.push_back(-2); vertices.push_back(0);
    vertices.push_back(-2); vertices.push_back(1); vertices.push_back(-2);
    vertices.push_back(-2); vertices.push_back(1); vertices.push_back(2);
    vertices.push_back(2); vertices.push_back(1); vertices.push_back(-2);
    vertices.push_back(2); vertices.push_back(1); vertices.push_back(2);
    vertices.push_back(-4); vertices.push_back(2); vertices.push_back(0);
    vertices.push_back(-4); vertices.push_back(-1); vertices.push_back(-1);
    vertices.push_back(-4); vertices.push_back(-1); vertices.push_back(1);

    for (unsigned int i = 0; i < vertices.size(); i++) { targets.push_back(vertices[i]); }
    targets[0] = 1.5;

    tetrahedra.push_back(0); tetrahedra.push_back(1); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(INSIDE);
    tetrahedra.push_back(4); tetrahedra.push_back(1); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(INSIDE);

    tetrahedra.push_back(4); tetrahedra.push_back(5); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(6); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(5); tetrahedra.push_back(6); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(7); tetrahedra.push_back(1); tetrahedra.push_back(2); tetrahedra.push_back(0);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(9); tetrahedra.push_back(1); tetrahedra.push_back(2); tetrahedra.push_back(4);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(9); tetrahedra.push_back(1); tetrahedra.push_back(3); tetrahedra.push_back(7);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(8); tetrahedra.push_back(1); tetrahedra.push_back(3); tetrahedra.push_back(0);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(10); tetrahedra.push_back(1); tetrahedra.push_back(3); tetrahedra.push_back(4);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(10); tetrahedra.push_back(1); tetrahedra.push_back(3); tetrahedra.push_back(8);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(1); tetrahedra.push_back(7); tetrahedra.push_back(11);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(1); tetrahedra.push_back(8); tetrahedra.push_back(11);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(2); tetrahedra.push_back(7); tetrahedra.push_back(12);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(3); tetrahedra.push_back(8); tetrahedra.push_back(13);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(2); tetrahedra.push_back(6); tetrahedra.push_back(12);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(3); tetrahedra.push_back(6); tetrahedra.push_back(13);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(11); tetrahedra.push_back(12); tetrahedra.push_back(7);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(11); tetrahedra.push_back(13); tetrahedra.push_back(8);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(12); tetrahedra.push_back(13); tetrahedra.push_back(6);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(11); tetrahedra.push_back(12); tetrahedra.push_back(13);
    statuses.push_back(OUTSIDE);

    std::vector<GeometrySet<unsigned int>> vertex_to_tet(vertices.size() / 3);
    for (unsigned int i = 0; i < tetrahedra.size(); i++) {
        vertex_to_tet[tetrahedra[i]].insert(i / 4);
    }
    
    return new TetMesh(vertices, targets, tetrahedra, statuses, vertex_to_tet);
}


TetMesh * TetMeshFactory::create_collapsed_tetmesh() {
    std::vector<REAL> vertices;
    std::vector<REAL> targets;
    std::vector<unsigned int> tetrahedra;
    std::vector<status_t> statuses;

    vertices.push_back(5); vertices.push_back(5); vertices.push_back(5);
    vertices.push_back(-5); vertices.push_back(-5); vertices.push_back(-5);
    vertices.push_back(5); vertices.push_back(5); vertices.push_back(-5);
    // vertices.push_back(5); vertices.push_back(-5); vertices.push_back(-5);
    vertices.push_back(5); vertices.push_back(-5); vertices.push_back(5);
    // vertices.push_back(-5); vertices.push_back(-5); vertices.push_back(5);
    vertices.push_back(-5); vertices.push_back(5); vertices.push_back(5);
    // vertices.push_back(-5); vertices.push_back(5); vertices.push_back(-5);

    for (unsigned int i = 0; i < vertices.size(); i++) { targets.push_back(vertices[i]); }

    tetrahedra.push_back(0); tetrahedra.push_back(1); tetrahedra.push_back(2); tetrahedra.push_back(3);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(1); tetrahedra.push_back(3); tetrahedra.push_back(4);
    statuses.push_back(OUTSIDE);
    tetrahedra.push_back(0); tetrahedra.push_back(1); tetrahedra.push_back(4); tetrahedra.push_back(2);
    statuses.push_back(OUTSIDE);

    std::vector<GeometrySet<unsigned int>> vertex_to_tet(vertices.size() / 3);
    for (unsigned int i = 0; i < tetrahedra.size(); i++) {
        vertex_to_tet[tetrahedra[i]].insert(i / 4);
    }
    
    TetMesh * tet_mesh = new TetMesh(vertices, targets, tetrahedra, statuses, vertex_to_tet);
    unsigned int a = tet_mesh->split_edge(Edge(0, 1));
    unsigned int b = tet_mesh->split_edge(Edge(0, a));
    tet_mesh->collapse_edge(Edge(a, b));
    return tet_mesh;
}

