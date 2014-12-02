#include "tetmesh.h"

#include <string>
#include <fstream>
#include <iostream>

// threshold for determining geometric equality
#define EPSILON 0.00001

#define vec_copy(dest, a) (dest)[0] = (a)[0]; (dest)[1] = (a)[1]; (dest)[2] = (a)[2]
#define vec_add(dest, a, b) (dest)[0] = (a)[0] + (b)[0]; (dest)[1] = (a)[1] + (b)[1]; (dest)[2] = (a)[2] + (b)[2]
#define vec_subtract(dest, a, b) (dest)[0] = (a)[0] - (b)[0]; (dest)[1] = (a)[1] - (b)[1]; (dest)[2] = (a)[2] - (b)[2]
#define vec_dot(a, b) ((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])
#define vec_cross(dest, a, b) (dest)[0] = (a)[1] * (b)[2] - (a)[2] * (b)[1]; (dest)[1] = (a)[2] * (b)[0] - (a)[0] * (b)[2]; (dest)[2] = (a)[0] * (b)[1] - (a)[1] * (b)[0]
#define vec_sqr_length(a) vec_dot(a, a)
#define vec_length(a) (sqrt(vec_sqr_length(a)))
#define vec_scale(dest, a, s) (dest)[0] = (a)[0] * s; (dest)[1] = (a)[1] * s; (dest)[2] = (a)[2] * s
#define vec_divide(dest, a, d) (dest)[0] = (a)[0] / d; (dest)[1] = (a)[1] / d; (dest)[2] = (a)[2] / d

#define absolute(a) (a < 0 ? -a : a)

TetMesh::TetMesh(std::vector<REAL> vertices, std::vector<REAL> vertex_targets,
                 std::vector<unsigned int> tets, std::vector<status_t> tet_statuses,
                 std::vector<GeometrySet<unsigned int>> vertex_tet_map) {
    this->vertices = vertices;
    this->vertex_targets = vertex_targets;
    this->tets = tets;
    this->tet_statuses = tet_statuses;
    this->vertex_tet_map = vertex_tet_map;

    vertex_gravestones.resize(vertices.size() / 3, ALIVE);
    tet_gravestones.resize(tets.size() / 3, ALIVE);
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

    std::vector<GeometrySet<unsigned int>> vertex_to_tet(num_t);
    for (int i = 0; i < num_t; i++) {
        for (int j = 0; j < 4; j++) {
            vertex_to_tet[tetrahedra[i * 4 + j]].insert(i);
        }
    }

    return new TetMesh(vertices, targets, tetrahedra, statuses, vertex_to_tet);
}

TetMesh * TetMesh::create_debug_tetmesh() {
    int num_v = 5;
    int num_t = 2;
    std::vector<REAL> vertices;
    std::vector<REAL> targets;
    std::vector<unsigned int> tetrahedra;
    std::vector<status_t> statuses;
    vertices.resize(num_v * 3);
    targets.resize(num_v * 3);
    tetrahedra.resize(num_t * 4);
    statuses.resize(num_t);
    
    vertices[0] = 0;  vertices[1] = 0;   vertices[2] = 0;  // (0, 0, 0)
    vertices[3] = 6;  vertices[4] = 6;   vertices[5] = 5;  // (6, 6, 5)
    vertices[6] = -6; vertices[7] = 6;   vertices[8] = 5;  // (-6, 6, 5)
    vertices[9] = 0;  vertices[10] = -6; vertices[11] = 5; // (0, -6, 5)
    vertices[12] = 0; vertices[13] = 0;  vertices[14] = 10; // (0, 0, 10)

    for (int i = 0; i < num_v*3; i++) { targets[i] = vertices[i]; }
    targets[14] = 2.5;

    statuses[0] = INSIDE;
    statuses[1] = OUTSIDE;

    tetrahedra[0] = 0; tetrahedra[1] = 1; tetrahedra[2] = 2; tetrahedra[3] = 3;
    tetrahedra[4] = 4; tetrahedra[5] = 1; tetrahedra[6] = 2; tetrahedra[7] = 3;

    std::vector<GeometrySet<unsigned int>> vertex_to_tet(num_t);
    for (int i = 0; i < num_t; i++) {
        for (int j = 0; j < 4; j++) {
            vertex_to_tet[tetrahedra[i * 4 + j]].insert(i);
        }
    }
    
    return new TetMesh(vertices, targets, tetrahedra, statuses, vertex_to_tet);
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
    unsigned int num_vertices_at_target = 0;
    unsigned int num_vertices = vertices.size() / 3;
    for (unsigned int i = 0; i < num_vertices; i++) {
        if (vertex_gravestones[i] == DEAD) {
            break;
        }
        vec_subtract(velocity, &vertex_targets[i * 3], &vertices[i * 3]);
        // TODO: for performance, can keep flag of whether at target and calculate this in else
        REAL target_distance = vec_length(velocity);
        // this vertex is already moved
        if (target_distance < EPSILON) {
            num_vertices_at_target++;
        } else {
            // normalize velocity
            vec_divide(velocity, velocity, target_distance);
            REAL distance = get_distance_movable(i, velocity);
            if (distance < EPSILON) { // Vertex can't move but wants to
                std::cout << "warning: unable to move vertex " << i << ", exiting to avoid an infinite loop" << std::endl;
                return true;
            } else if (distance >= target_distance) { // Vertex can move to target
                vec_copy(&vertices[i * 3], &vertex_targets[i * 3]);
            } else {
                vec_scale(velocity, velocity, distance);
                vec_add(&vertices[i * 3], &vertices[i * 3], velocity);
            }
        }
    }
    return num_vertices_at_target == num_vertices;
}

REAL TetMesh::get_distance_movable(unsigned int vertex_index, REAL * velocity) {
    static REAL plane[] = {
        0, 0, 0, 0
    };
    REAL min_distance = -1;
    GeometrySet<unsigned int> t = vertex_tet_map[vertex_index];
    for (auto it = t.begin(); it != t.end(); it++) {
        Face f = get_opposite_face(*it, vertex_index);
        calculate_plane(plane, f);
        REAL distance = intersect_plane(plane, &vertices[vertex_index * 3], velocity);
        if (distance > 0 && (distance < min_distance || min_distance == -1)) {
            min_distance = distance;
        }

    }
    return min_distance;
}

void TetMesh::calculate_plane(REAL * plane, Face f) {
    static REAL temp1[] = {
        0, 0, 0
    };
    static REAL temp2[] = {
        0, 0, 0
    };

    REAL * v1 = &vertices[f.getV1() * 3];
    REAL * v2 = &vertices[f.getV2() * 3];
    REAL * v3 = &vertices[f.getV3() * 3];
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
    unsigned int num_tets = tets.size() / 4;
    for (unsigned int i = 0; i < num_tets; i++) {
        if (tet_gravestones[i] == DEAD) {
            break;
        }
    }
}

void TetMesh::bind_attributes(Renderable & renderable) {
    // ensure vertices are not double precision
    float * verts = new float[vertices.size()];
    for (unsigned int i = 0; i < vertices.size(); i++) {
        verts[i] = vertices[i];
    }
    renderable.bind_attribute(verts, VEC3_FLOAT, vertices.size() / 3, "vertex_position");
    delete[] verts;

    int * vertex_statuses = new int[vertices.size() / 3];
    for (unsigned int i = 0; i < vertices.size() / 3; i++) {
        vertex_statuses[i] = (int)get_vertex_status(i);
    }
    renderable.bind_attribute(&vertex_statuses[0], SCALAR_INT, vertices.size() / 3, "vertex_status");
    delete[] vertex_statuses;

    unsigned int num_tets = tets.size() / 4;
    unsigned int num_faces = num_tets * 4;
    unsigned int num_indices = num_faces * 3;
    int * indices = new int[num_indices];

    for (unsigned int i = 0; i < num_tets; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            Face f = get_opposite_face(i, tets[i * 4 + j]);
            indices[i * 12 + j] = f.getV1();
            indices[i * 12 + j + 1] = f.getV2();
            indices[i * 12 + j + 2] = f.getV3();
        }
    }

    renderable.bind_indices(indices, num_indices * sizeof(int));
    delete[] indices;
}

status_t TetMesh::get_vertex_status(unsigned int vertex_index) {
    bool all_inside = true;
    bool all_outside = true;
    for (auto it = vertex_tet_map[vertex_index].begin(); it != vertex_tet_map[vertex_index].end(); it++) {
        if (tet_statuses[*it] == INSIDE) {
            all_outside = false;
        } else {
            all_inside = false;
        }
    }
    if (all_inside) {
        return INSIDE;
    } else if (all_outside) {
        return OUTSIDE;
    }
    return INTERFACE;
}


Edge TetMesh::get_opposite_edge(unsigned int tet_id, Edge e) {
    unsigned int v1 = 0, v2 = 0;
    bool found_v1 = false;
    for (int i = 0; i < 4; i++) {
        if (!e.contains(tets[tet_id * 4 + i])) {
            if (found_v1) {
                v2 = tets[tet_id * 4 + i];
                break;
            } else {
                v1 = tets[tet_id * 4 + i];
                found_v1 = true;
            }
        }
    }
    return Edge(v1, v2);
}

void TetMesh::split_edge(Edge edge) {
    unsigned int v1 = edge.getV1();
    unsigned int v2 = edge.getV2();
    GeometrySet<unsigned int> split = vertex_tet_map[v1].intersectWith(vertex_tet_map[v2]);

    unsigned int c = insert_vertex(edge);

    for (auto it = split.begin(); it != split.end(); it++) {
        Edge opposite = get_opposite_edge(*it, edge);
        insert_tet(c, v1, opposite.getV1(), opposite.getV2(), tet_statuses[*it]);
        insert_tet(c, v2, opposite.getV1(), opposite.getV2(), tet_statuses[*it]);
        delete_tet(*it);
    }
}

void TetMesh::collapse_edge(Edge edge) {
    unsigned int v1 = edge.getV1();
    unsigned int v2 = edge.getV2();
    GeometrySet<unsigned int> deleted = vertex_tet_map[v1].intersectWith(vertex_tet_map[v2]);
    GeometrySet<unsigned int> affected = vertex_tet_map[v1].outersectWith(vertex_tet_map[v2]);

    unsigned int c = insert_vertex(edge);

    for (auto it = affected.begin(); it != affected.end(); it++) {
        for (unsigned int i = 0; i < 4; i++) {
            if (tets[*it * 4 + i] == v1 || tets[*it * 4 + i] == v2) {
                tets[*it * 4 + i] = c;
                break;
            }
        }
    }
    for (auto it = deleted.begin(); it != deleted.end(); it++) {
        delete_tet(*it);
    }
    vertex_gravestones[v1] = DEAD;
    vertex_gravestones[v2] = DEAD;
}

unsigned int TetMesh::insert_vertex(Edge edge) {
    unsigned int v1 = edge.getV1();
    unsigned int v2 = edge.getV2();

    unsigned int c = vertices.size() / 3;
    vertices.resize((c + 1) * 3);
    REAL * c_data = &vertices[c * 3];
    vec_add(c_data, &vertices[v1], &vertices[v2]);
    vec_divide(c_data, c_data, 2);

    vertex_targets.resize((c + 1) * 3);
    c_data = &vertex_targets[c * 3];
    vec_add(c_data, &vertex_targets[v1], &vertex_targets[v2]);
    vec_divide(c_data, c_data, 2);

    vertex_gravestones.push_back(ALIVE);
    return c;
}

void TetMesh::delete_tet(unsigned int t) {
    tet_gravestones[t] = DEAD;
    for (unsigned int i = 0; i < 4; i++) {
        vertex_tet_map[tets[t * 4 + i]].remove(t);
    }
}

unsigned int TetMesh::insert_tet(unsigned int v1, unsigned int v2, unsigned int v3, unsigned int v4, status_t status) {
    unsigned int t = tets.size() / 4;
    tets.push_back(v1);
    tets.push_back(v2);
    tets.push_back(v3);
    tets.push_back(v4);
    vertex_tet_map[v1].insert(t);
    vertex_tet_map[v2].insert(t);
    vertex_tet_map[v3].insert(t);
    vertex_tet_map[v4].insert(t);
    tet_gravestones.push_back(ALIVE);
    tet_statuses.push_back(status);
    return t;
}

Face TetMesh::get_opposite_face(unsigned int tet_id, unsigned int vert_id) {
    unsigned int v1 = tets[tet_id * 4];
    unsigned int v2 = tets[tet_id * 4 + 1];
    unsigned int v3 = tets[tet_id * 4 + 2];
    unsigned int v4 = tets[tet_id * 4 + 3];
    if (v1 == vert_id) {
        return Face(v2, v3, v4);
    } else if (v2 == vert_id) {
        return Face(v1, v3, v4);
    } else if (v3 == vert_id) {
        return Face(v1, v2, v4);
    } else {
        return Face(v1, v2, v3);
    }
}

// Derived from: http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
REAL TetMesh::distance_between_point_and_edge(Edge edge, int vertex_index) {
    static REAL cross[] = {
        0, 0, 0
    };
    static REAL temp1[] = {
        vertices[edge.getV2() * 3] - vertices[edge.getV1() * 3],
        vertices[edge.getV2() * 3 + 1] - vertices[edge.getV1() * 3 + 1],
        vertices[edge.getV2() * 3 + 2] - vertices[edge.getV1() * 3 + 2]
    };
    static REAL temp2[] = {
        vertices[edge.getV1() * 3] - vertices[vertex_index * 3],
        vertices[edge.getV1() * 3 + 1] - vertices[vertex_index * 3 + 1],
        vertices[edge.getV1() * 3 + 2] - vertices[vertex_index * 3 + 2],
    };

    vec_cross(cross, temp1, temp2);
    return (vec_length(cross) / vec_length(temp1));
}

Edge TetMesh::longest_edge_in_set(GeometrySet<Edge> set_of_edges) {
    assert(set_of_edges.size() > 0);
    std::vector<Edge> edges = set_of_edges.getItems();
    int longestEdgeIndex = 0;
    REAL dx = vertices[edges[0].getV1() * 3] - vertices[edges[0].getV2() * 3];
    REAL dy = vertices[edges[0].getV1() * 3 + 1] - vertices[edges[0].getV2() * 3 + 1];
    REAL dz = vertices[edges[0].getV1() * 3 + 2] - vertices[edges[0].getV2() * 3 + 2];
    REAL longestDistSq = dx*dx + dy*dy + dz*dz;
        
    for (size_t i = 1; i < edges.size(); i++) {
        dx = vertices[edges[i].getV1() * 3] - vertices[edges[i].getV2() * 3];
        dy = vertices[edges[i].getV1() * 3 + 1] - vertices[edges[i].getV2() * 3 + 1];
        dz = vertices[edges[i].getV1() * 3 + 2] - vertices[edges[i].getV2() * 3 + 2];
        REAL distSq = dx*dx + dy*dy + dz*dz;
        if (distSq > longestDistSq) {
            longestDistSq = distSq;
            longestEdgeIndex = i;
        }
    }
    Edge longestEdge = edges[longestEdgeIndex];
    return longestEdge;
}
