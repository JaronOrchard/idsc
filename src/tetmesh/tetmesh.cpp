#include "tetmesh.h"
#include <string>
#include <fstream>
#include <iostream>

// threshold for determining geometric equality
#define EPSILON 0.0001

#define vec_subtract(dest, a, b) (dest)[0] = (a)[0] - (b)[0]; (dest)[1] = (a)[1] - (b)[1]; (dest)[2] = (a)[2] - (b)[2]
#define vec_dot(a, b) ((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])
#define vec_cross(dest, a, b) (dest)[0] = (a)[1] * (b)[2] - (a)[2] * (b)[1]; (dest)[1] = (a)[2] * (b)[0] - (a)[0] * (b)[2]; (dest)[2] = (a)[0] * (b)[1] - (a)[1] * (b)[0]
#define vec_sqr_length(a) vec_dot(a, a)
#define vec_length(a) (sqrt(vec_sqr_length(a)))
#define vec_scale(dest, a, s) (dest)[0] = (a)[0] * s; (dest)[1] = (a)[1] * s; (dest)[2] = (a)[2] * s
#define vec_divide(dest, a, d) (dest)[0] = (a)[0] / d; (dest)[1] = (a)[1] / d; (dest)[2] = (a)[2] / d

#define absolute(a) (a < 0 ? -a : a)

TetMesh::TetMesh(int num_vertices, std::vector<REAL> vertices, std::vector<short> vertex_statuses,
            std::vector<REAL> vertex_targets, int num_tets, std::vector<int> tets) {
    this->num_vertices = num_vertices;
    this->vertices = vertices;
    this->vertex_statuses = vertex_statuses;
    this->vertex_targets = vertex_targets;
    this->num_tets = num_tets;
    this->tets = tets;
}

TetMesh::~TetMesh() {
    // nothing to clean up
}

// assumes interface will not evolve beyond [-10, 10]^3 and encloses the point (0, 0, 0)
TetMesh * TetMesh::from_indexed_face_set(IndexedFaceSet & ifs) {
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

    delete outer_input->pointlist;
    outer_input->pointlist = new_vertices;

    //add outer cube faces
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
    switches.parse_commandline("pYqQ");
    tetrahedralize(&switches, inner_input, &inner_output);
    tetrahedralize(&switches, outer_input, &outer_output);

    int outer_num_v = outer_output.numberofpoints - orig_num_v;
    int inner_num_v = inner_output.numberofpoints - orig_num_v;

    int num_v = orig_num_v + outer_num_v + inner_num_v;
    std::vector<REAL> vertices;
    std::vector<REAL> targets;
    std::vector<short> statuses;
    vertices.resize(num_v * 3);
    targets.resize(num_v * 3);
    statuses.resize(num_v);
    
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
        statuses[i + inner_num_v] = 1;
    }

    for (int i = 0; i < num_v * 3; i++) {
        targets[i] = vertices[i];
    }

    int inner_num_t = inner_output.numberoftetrahedra;
    int num_t = outer_output.numberoftetrahedra + inner_num_t;
    std::vector<int> tetrahedra;
    tetrahedra.resize(num_t * 4);
    for (int i = 0; i < inner_num_t; i++) {
        for (int j = 0; j < 4; j++) {
            tetrahedra[i * 4 + j] = inner_output.tetrahedronlist[i * 4 + j];
        }
    }

    for (int i = 0; i < outer_output.numberoftetrahedra; i++) {
        for (int j = 0; j < 4; j++) {
            tetrahedra[(i + inner_num_t) * 4 + j] = outer_output.tetrahedronlist[i * 4 + j];
            if (outer_output.tetrahedronlist[i * 4 + j] >= orig_num_v) {
                tetrahedra[(i + inner_num_t) * 4 + j] += inner_num_v;
            }
        }
    }

    delete inner_input;
    delete outer_input;

    return new TetMesh(num_v, vertices, statuses, targets, num_t, tetrahedra);
}

TetMesh * TetMesh::create_debug_tetmesh() {
    int num_v = 5;
    int num_t = 2;
    std::vector<REAL> vertices;
    std::vector<REAL> targets;
    std::vector<short> statuses;
    std::vector<int> tetrahedra;
    vertices.resize(num_v * 3);
    targets.resize(num_v * 3);
    statuses.resize(num_v);
    tetrahedra.resize(num_t * 4);
    
    vertices[0] = 0;  vertices[1] = 0;   vertices[2] = 0;  // (0, 0, 0)
    vertices[3] = 6;  vertices[4] = 6;   vertices[5] = 5;  // (6, 6, 5)
    vertices[6] = -6; vertices[7] = 6;   vertices[8] = 5;  // (-6, 6, 5)
    vertices[9] = 0;  vertices[10] = -6; vertices[11] = 5; // (0, -6, 5)
    vertices[12] = 0; vertices[13] = 0;  vertices[14] = 10; // (0, 0, 10)

    for (int i = 0; i < num_v*3; i++) { targets[i] = vertices[i]; }
    targets[14] = 2.5;

    for (int i = 0; i < num_v; i++) { statuses[i] = 2; }
    
    tetrahedra[0] = 0; tetrahedra[1] = 1; tetrahedra[2] = 2; tetrahedra[3] = 3;
    tetrahedra[4] = 4; tetrahedra[5] = 1; tetrahedra[6] = 2; tetrahedra[7] = 3;

    return new TetMesh(num_v, vertices, statuses, targets, num_t, tetrahedra);
}

void TetMesh::save(std::string object_name) {
    // .node file:
    std::ofstream fout_node(object_name + ".node");
    if (fout_node.fail()) {
        std::cout << "Could not open " << object_name << ".node for writing!" << std::endl;
        return;
    }
    fout_node << num_vertices << "  3  0  0" << std::endl;
    for (int i = 0; i < num_vertices; i++) {
        fout_node << i << "  " << vertices[i*3] << "  " << vertices[i*3+1] << "  " << vertices[i*3+2] << std::endl;
    }
    fout_node.close();
    std::cout << object_name << ".node saved successfully." << std::endl;
        
    // .ele file:
    std::ofstream fout_ele(object_name + ".ele");
    if (fout_ele.fail()) {
        std::cout << "Could not open " << object_name << ".ele for writing!" << std::endl;
        return;
    }
    fout_ele << num_tets << "  4  0" << std::endl;
    for (int i = 0; i < num_tets; i++) {
        fout_ele << i << "  " << tets[i*4] << "  " << tets[i*4+1] << "  " << tets[i*4+2] << "  " << tets[i*4+3] << std::endl;
    }
    fout_ele.close();
    std::cout << object_name << ".ele saved successfully." << std::endl;
    
    // .face file:
    std::ofstream fout_face(object_name + ".face");
    if (fout_face.fail()) {
        std::cout << "Could not open " << object_name << ".face for writing!" << std::endl;
        return;
    }
    fout_face << (num_tets*4) << "  1" << std::endl;
    for (int i = 0; i < num_tets; i++) {
        fout_face << (i*4) << "  " << tets[i*4] << "  " << tets[i*4+1] << "  " << tets[i*4+2] << "  " << get_face_boundary_marker(tets[i*4], tets[i*4+1], tets[i*4+2]) << std::endl;
        fout_face << (i*4+1) << "  " << tets[i*4] << "  " << tets[i*4+1] << "  " << tets[i*4+3] << "  " << get_face_boundary_marker(tets[i*4], tets[i*4+1], tets[i*4+3]) << std::endl;
        fout_face << (i*4+2) << "  " << tets[i*4] << "  " << tets[i*4+2] << "  " << tets[i*4+3] << "  " << get_face_boundary_marker(tets[i*4], tets[i*4+2], tets[i*4+3]) << std::endl;
        fout_face << (i*4+3) << "  " << tets[i*4+1] << "  " << tets[i*4+2] << "  " << tets[i*4+3] << "  " << get_face_boundary_marker(tets[i*4+1], tets[i*4+2], tets[i*4+3]) << std::endl;
    }
    fout_face.close();
    std::cout << object_name << ".face saved successfully." << std::endl;
    
}

short TetMesh::get_face_boundary_marker(int vertex1, int vertex2, int vertex3) {
    short marker1 = vertex_statuses[vertex1];
    short marker2 = vertex_statuses[vertex2];
    short marker3 = vertex_statuses[vertex3];
    if (marker1 == 0 || marker2 == 0 || marker3 == 0) {
        return 0; // Tet is inside
    } else if (marker1 == 1 || marker2 == 1 || marker3 == 1) {
        return 1; // Tet is outside
    } else {
        return 2; // Tet is on the boundary
    }
}

void TetMesh::evolve() {
    bool done = false;
    while (!done) {
        done = advect();
        retesselate();
    }
}

// move vertices as far toward target as possible
bool TetMesh::advect() {
    static REAL velocity[] = {
        0, 0, 0
    };
    int num_vertices_at_target = 0;
    for (int i = 0; i < num_vertices; i++) {
        vec_subtract(velocity, &vertex_targets[i * 3], &vertices[i * 3]);
        // TODO: for performance, can keep flag of whether at target and calculate this in else
        REAL target_distance = vec_length(velocity);
        // this vertex is already moved
        if (target_distance < EPSILON) {
            num_vertices_at_target++;
        } else {
            // normalize velocity
            vec_divide(velocity, velocity, target_distance);
            DistanceMovableInfo dminfo = get_distance_movable(i, velocity);
            if (dminfo.distance < EPSILON) { // Vertex can't move but wants to
                std::cout << "warning: unable to move vertex " << i << ", exiting to avoid an infinite loop" << std::endl;
                return true;
            } else if (dminfo.distance >= target_distance) { // Vertex can move to target
                memcpy(&vertices[i * 3], &vertex_targets[i * 3], 3 * sizeof(REAL));
            } else { // Vertex can only move to a face boundary, so subdivide now
                subdivide_opposite_tet(dminfo.tet_index, i);
            }
        //// if the vertex has not reached its target, queue the tet for subdivision
        //vec_subtract(velocity, &vertex_targets[i * 3], &vertices[i * 3]);
        //target_distance = vec_length(velocity);
        //if (target_distance >= EPSILON) {
        //    pending_subdivisions.push_back(std::pair<int, int>(dminfo.tet_index, i));
        }
    }
    return num_vertices_at_target == num_vertices;
}

TetMesh::DistanceMovableInfo TetMesh::get_distance_movable(int vertex_index, REAL * velocity) {
    DistanceMovableInfo dminfo;
    static REAL plane[] = {
        0, 0, 0, 0
    };
    // TODO: for performance, can make inverted index so vertices know about tets they're in
    for (int i = 0; i < num_tets; i++) {
        for (int j = 0; j < 4; j++) {
            if (tets[i * 4 + j] == vertex_index) {
                // TODO: clean this up
                switch (j) {
                    case 0:
                        calculate_plane(plane, &vertices[tets[i * 4 + 1] * 3], &vertices[tets[i * 4 + 2] * 3], &vertices[tets[i * 4 + 3] * 3]);
                        break;
                    case 1:
                        calculate_plane(plane, &vertices[tets[i * 4 + 0] * 3], &vertices[tets[i * 4 + 2] * 3], &vertices[tets[i * 4 + 3] * 3]);
                        break;
                    case 2:
                        calculate_plane(plane, &vertices[tets[i * 4 + 0] * 3], &vertices[tets[i * 4 + 1] * 3], &vertices[tets[i * 4 + 3] * 3]);
                        break;
                    case 3:
                        calculate_plane(plane, &vertices[tets[i * 4 + 0] * 3], &vertices[tets[i * 4 + 1] * 3], &vertices[tets[i * 4 + 2] * 3]);
                        break;
                }
                REAL distance = intersect_plane(plane, &vertices[vertex_index * 3], velocity);
                if (distance > 0 && (distance < dminfo.distance || dminfo.distance == -1)) {
                    dminfo.distance = distance;
                    dminfo.tet_index = i;
                }
                break;
            }
        }
    }
    return dminfo;
}

void TetMesh::calculate_plane(REAL * plane, REAL * v1, REAL * v2, REAL * v3) {
    static REAL temp1[] = {
        0, 0, 0
    };
    static REAL temp2[] = {
        0, 0, 0
    };
    vec_subtract(temp1, v1, v3);
    vec_subtract(temp2, v2, v3);
    vec_cross(plane, temp1, temp2);
    vec_divide(plane, plane, vec_length(plane));
    plane[3] = -vec_dot(plane, v3);
}

REAL TetMesh::intersect_plane(REAL * plane, REAL * vertex, REAL * velocity) {
    REAL denominator = vec_dot(velocity, plane);
    // ray perpendicular to plane
    if (absolute(denominator) < EPSILON) {
        return -1;
    }
    return (vec_dot(vertex, plane) + plane[3]) / absolute(denominator);
}

void TetMesh::retesselate() {
    /* (Removed as we're doing subdivisions on demand now):
    while (!pending_subdivisions.empty()) {
        std::pair<int, int> next_subdivision = pending_subdivisions.back();
        pending_subdivisions.pop_back();
        subdivide_opposite_tet(next_subdivision.first, next_subdivision.second);
    }
    */
}

void TetMesh::bind_attributes(Renderable & renderable) {
    // ensure vertices are not double precision
    float * verts = new float[num_vertices * 3];
    for (int i = 0; i < num_vertices * 3; i++) {
        verts[i] = vertices[i];
    }
    renderable.bind_attribute(verts, VEC3_FLOAT, num_vertices, "vertex_position");
    free(verts);

    renderable.bind_attribute(&vertex_statuses[0], SCALAR_SHORT, num_vertices, "vertex_status");

    int num_faces = num_tets * 4;
    int num_indices = num_faces * 3;
    int * indices = new int[num_indices];

    for (int i = 0; i < num_tets; i++) {
        indices[i * 12] = tets[i * 4];
        indices[i * 12 + 1] = tets[i * 4 + 1];
        indices[i * 12 + 2] = tets[i * 4 + 2];

        indices[i * 12 + 3] = tets[i * 4];
        indices[i * 12 + 4] = tets[i * 4 + 1];
        indices[i * 12 + 5] = tets[i * 4 + 3];

        indices[i * 12 + 6] = tets[i * 4];
        indices[i * 12 + 7] = tets[i * 4 + 2];
        indices[i * 12 + 8] = tets[i * 4 + 3];

        indices[i * 12 + 9] = tets[i * 4 + 1];
        indices[i * 12 + 10] = tets[i * 4 + 2];
        indices[i * 12 + 11] = tets[i * 4 + 3];
    }

    renderable.bind_indices(indices, num_indices * sizeof(int));
    free(indices);
}

/*
 * Given the index of a tet and the index of a vertex (in that tet), returns
 * a vector of ints comprised of the OTHER three vertices in that tet.
 */
std::vector<int> TetMesh::get_other_vertices(int tet, int vertex) {
    std::vector<int> v;
    if (tet < 0 || tet >= num_tets) {
        std::cerr << "** ERROR in get_other_vertices() - Tet index does not exist" << std::endl;
        return v;
    }
    for (int i = tet*4; i < tet*4+4; i++) {
        if (tets[i] == vertex) { continue; }
        v.push_back(tets[i]);
    }
    if (v.size() > 3) {
        std::cerr << "** ERROR in get_other_vertices() - Given tet does not contain given vertex" << std::endl;
    }
    return v;
}

/*
 * Given the index of a tet and the index of a vertex (in that tet), returns
 * the tet that shares the OTHER three out of four vertices in the tet.
 */
int TetMesh::get_opposite_tet(int tet, int vertex) {
    std::vector<int> v = get_other_vertices(tet, vertex);
    for (int i = 0; i < num_tets; i++) {
        if (tets[i*4] != vertex && tets[i*4+1] != vertex && tets[i*4+2] != vertex && tets[i*4+3] != vertex &&
            (tets[i*4] == v[0] || tets[i*4+1] == v[0] || tets[i*4+2] == v[0] || tets[i*4+3] == v[0]) &&
            (tets[i*4] == v[1] || tets[i*4+1] == v[1] || tets[i*4+2] == v[1] || tets[i*4+3] == v[1]) &&
            (tets[i*4] == v[2] || tets[i*4+1] == v[2] || tets[i*4+2] == v[2] || tets[i*4+3] == v[2])) {
                return i;
        }
    }
    return -1; // tet not found
}

/*
 * Given the index of a tet and the index of a vertex (in that tet),
 * return the opposite vertex of the opposite tet.
 */
int TetMesh::get_opposite_vertex(int tet, int vertex) {
    std::vector<int> v = get_other_vertices(tet, vertex);
    int opposite_tet = get_opposite_tet(tet, vertex);
    for (int i = opposite_tet*4; i <= opposite_tet*4+4; i++) {
        if (tets[i] == v[0] || tets[i] == v[1] || tets[i] == v[2]) { continue; }
        return tets[i];
    }
    return -1; // vertex not found
}

/**
 * Given a tet index, averages its four vertices' positions and
 * creates a new vertex in that spot, returning the new vertex.
 */
int TetMesh::create_new_vertex_in_tet_center(int tet) {
    int new_vertex_index = num_vertices++;
    vertices.push_back((vertices[tets[tet*4]]   + vertices[tets[tet*4+1]]   + vertices[tets[tet*4+2]]   + vertices[tets[tet*4+3]])   / 4);
    vertices.push_back((vertices[tets[tet*4]+1] + vertices[tets[tet*4+1]+1] + vertices[tets[tet*4+2]+1] + vertices[tets[tet*4+3]+1]) / 4);
    vertices.push_back((vertices[tets[tet*4]+2] + vertices[tets[tet*4+1]+2] + vertices[tets[tet*4+2]+2] + vertices[tets[tet*4+3]+2]) / 4);
    for (int i = new_vertex_index*3; i < new_vertex_index*3+3; i++) {
        vertex_targets.push_back(vertices[i]);
    }
    return new_vertex_index;
}

/*
 * Given the index of a tet and the index of a vertex (in that tet), subdivides
 * the 2 tets adjacent to and opposite the given vertex into 9 new tets by creating
 * 2 new vertices and removing (replacing) the 2 original tets.
 */
void TetMesh::subdivide_opposite_tet(int tet, int vertex) {
    std::cout << "---> SUBDIVIDING TET " << tet << std::endl;

    std::vector<int> v = get_other_vertices(tet, vertex);
    int opposite_tet = get_opposite_tet(tet, vertex);
    int opposite_vertex = get_opposite_vertex(tet, vertex);

    // Create a new vertex in the middle of the opposite tet and set it up:
    int new_vertex = create_new_vertex_in_tet_center(opposite_tet);
    for (int i = 0; i < 3; i++) {
        vertex_targets[new_vertex*3+i] = vertex_targets[vertex*3+i]; // copy vertex target
        vertex_targets[vertex*3+i] = vertices[vertex*3+i]; // reset old vertex target
    }
    
    // Modify the original opposite tet to use the new vertex as its top:
    for (int i = opposite_tet*4; i < opposite_tet*4+4; i++) {
        if (tets[i] == opposite_vertex) {
            tets[i] = new_vertex;
            break;
        }
    }

    // Create 3 new tets in the empty space we just created:
    num_tets += 3;
    tets.push_back(v[0]); tets.push_back(v[1]); tets.push_back(new_vertex); tets.push_back(opposite_vertex);
    tets.push_back(v[0]); tets.push_back(v[2]); tets.push_back(new_vertex); tets.push_back(opposite_vertex);
    tets.push_back(v[1]); tets.push_back(v[2]); tets.push_back(new_vertex); tets.push_back(opposite_vertex);

    // Fix vertex boundary statuses:
    vertex_statuses[v[0]] = 2;
    vertex_statuses[v[1]] = 2;
    vertex_statuses[v[2]] = 2;
    vertex_statuses.push_back(2);
    vertex_statuses[vertex] = (vertex_statuses[opposite_vertex] == 1 ? 0 : 1);
}
