#include "tetmesh.h"

#include <string>
#include <fstream>
#include <iostream>

#include "util/vec.h"

// threshold for determining geometric equality
#define EPSILON 0.00001

#define absolute(a) ((a) < 0 ? -(a) : (a))

TetMesh::TetMesh(std::vector<REAL> vertices, std::vector<REAL> vertex_targets,
                 std::vector<unsigned int> tets, std::vector<status_t> tet_statuses,
                 std::vector<GeometrySet<unsigned int>> vertex_tet_map) {
    this->vertices = vertices;
    this->vertex_targets = vertex_targets;
    this->tets = tets;
    this->tet_statuses = tet_statuses;
    this->vertex_tet_map = vertex_tet_map;

    vertex_statuses.resize(vertices.size() / 3, STATIC);
    vertex_gravestones.resize(vertices.size() / 3, ALIVE);
    tet_gravestones.resize(tets.size() / 3, ALIVE);
}

TetMesh::~TetMesh() {
    // nothing to clean up
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
        if (vertex_gravestones[i] == DEAD || vertex_statuses[i] == STATIC) {
            num_vertices_at_target++;
            continue;
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
                std::cout << "warning: unable to move vertex " << i << ", could be in an infinite loop" << std::endl;
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
        assert(tet_gravestones[*it] != DEAD);
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
    static REAL u[] = {
        0, 0, 0
    };
    static REAL v[] = {
        0, 0, 0
    };
    static REAL w[] = {
        0, 0, 0
    };
    static REAL x[] = {
        0, 0, 0
    };
    static REAL plane[] = {
        0, 0, 0, 0
    };
    unsigned int num_tets = tets.size() / 4;
    for (unsigned int i = 0; i < num_tets; i++) {
        if (tet_gravestones[i] == DEAD) {
            continue;
        }

        GeometrySet<Face> faces = get_faces_from_tet(i);
        Face test_face = *faces.begin();
        calculate_plane(plane, test_face);
        REAL * opp_v = &vertices[get_opposite_vertex(i, test_face) * 3];
        bool is_coplanar = absolute(vec_dot(plane, opp_v) + plane[3]) < EPSILON;
        if (is_coplanar) {
            GeometrySet<Edge> edges = get_edges_from_tet(i);

            // vertex on vertex
            Edge shortest_edge = shortest_edge_in_set(edges);
            if (get_edge_length(shortest_edge) < EPSILON) {
                printf("vertex on vertex\n");
                collapse_edge(shortest_edge);
                continue;
            }

            // vertex on edge
            unsigned int closest_v;
            Edge closest_edge(0, 0);
            REAL min_dist = -1;
            for (unsigned int j = 0; j < 4; j++) {
                unsigned int v = tets[i * 4];
                Face f = get_opposite_face(i, v);
                GeometrySet<Edge> opp_edges = get_edges_from_face(f);
                for (auto it = opp_edges.begin(); it != opp_edges.end(); it++) {
                    REAL d = distance_between_point_and_edge(*it, v);
                    if (d < min_dist || min_dist == -1) {
                        closest_v = v;
                        closest_edge = *it;
                        min_dist = d;
                    }
                }
            }
            if (min_dist < EPSILON) {
                printf("vertex on edge %f\n", min_dist);
                unsigned int c = split_edge(closest_edge);
                collapse_edge(Edge(closest_v, c));
                continue;
            }

            // concave and convex flattened tets
            Face f = largest_face_in_set(faces);
            unsigned int apex = get_opposite_vertex(i, f);
            REAL * apex_data = &vertices[apex * 3];
            REAL * v1 = &vertices[f.getV1() * 3];
            REAL * v2 = &vertices[f.getV2() * 3];
            REAL * v3 = &vertices[f.getV3() * 3];
            vec_subtract(u, apex_data, v1);
            vec_subtract(v, v2, apex_data);
            vec_subtract(w, v3, v2);
            vec_cross(x, u, v);
            vec_cross(u, v, w);
            // apex is inside f
            if (vec_dot(x, u) < 0) {
                printf("concave\n");
                Edge e = longest_edge_in_set(edges);
                unsigned int c = split_edge(e);
                collapse_edge(Edge(apex, c));
            // apex is outside f
            } else {
                printf("convex\n");
                Edge e1 = longest_edge_in_set(edges);
                edges.remove(e1);
                Edge e2 = longest_edge_in_set(edges);
                unsigned int c1 = split_edge(e1);
                unsigned int c2 = split_edge(e2);
                collapse_edge(Edge(c1, c2));
            }
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

    unsigned int buffer_i = 0;
    for (unsigned int i = 0; i < num_tets; i++) {
        if (tet_gravestones[i] != DEAD) {
            for (unsigned int j = 0; j < 4; j++) {
                Face f = get_opposite_face(i, tets[i * 4 + j]);
                indices[buffer_i * 12 + j * 3] = f.getV1();
                indices[buffer_i * 12 + j * 3 + 1] = f.getV2();
                indices[buffer_i * 12 + j * 3 + 2] = f.getV3();
            }
            buffer_i++;
        }
    }

    renderable.bind_indices(indices, buffer_i * 12 * sizeof(int));
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

unsigned int TetMesh::split_edge(Edge edge) {
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

    return c;
}

// TODO: collapsing could possible invert a tet
//      Use getDistanceMovable to make sure this does not happen
int TetMesh::collapse_edge(Edge edge) {
    unsigned int v1 = edge.getV1();
    unsigned int v2 = edge.getV2();
    if (vertex_statuses[v1] == MOVING && vertex_statuses[v2] == MOVING) {
        std::cout << "warning: unable to collapse edge " << v1 << ", " << v2
            << ".  May be in an infinite loop." << std::endl;
        return -1;
    }
    GeometrySet<unsigned int> deleted = vertex_tet_map[v1].intersectWith(vertex_tet_map[v2]);
    GeometrySet<unsigned int> affected = vertex_tet_map[v1].outersectWith(vertex_tet_map[v2]);

    unsigned int c;

    if (vertex_statuses[v1] == MOVING) {
        c = insert_vertex(edge, v1);
    } else if (vertex_statuses[v2] == MOVING) {
        c = insert_vertex(edge, v2);
    } else {
        c = insert_vertex(edge);
    }

    for (auto it = affected.begin(); it != affected.end(); it++) {
        for (unsigned int i = 0; i < 4; i++) {
            if (tets[*it * 4 + i] == v1 || tets[*it * 4 + i] == v2) {
                tets[*it * 4 + i] = c;
                vertex_tet_map[c].insert(*it);
                break;
            }
        }
    }
    for (auto it = deleted.begin(); it != deleted.end(); it++) {
        delete_tet(*it);
    }
    vertex_gravestones[v1] = DEAD;
    vertex_gravestones[v2] = DEAD;

    return c;
}

unsigned int TetMesh::insert_vertex(Edge edge) {
    unsigned int v1 = edge.getV1();
    unsigned int v2 = edge.getV2();

    unsigned int c = vertices.size() / 3;
    vertices.resize((c + 1) * 3);
    REAL * c_data = &vertices[c * 3];
    vec_add(c_data, &vertices[v1 * 3], &vertices[v2 * 3]);
    vec_divide(c_data, c_data, 2);

    vertex_targets.resize((c + 1) * 3);

    vertex_gravestones.push_back(ALIVE);
    vertex_statuses.push_back(STATIC);
    vertex_tet_map.push_back(GeometrySet<unsigned int>());
    return c;
}

unsigned int TetMesh::insert_vertex(Edge edge, unsigned int moving_vertex) {
    unsigned int c = insert_vertex(edge);

    vec_copy(&vertex_targets[c * 3], &vertex_targets[moving_vertex * 3]);

    vertex_statuses[c] = MOVING;
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

REAL TetMesh::get_edge_length(Edge edge) {
    static REAL base[] = {
        0, 0, 0
    };
    REAL * v1 = &vertices[edge.getV1() * 3];
    REAL * v2 = &vertices[edge.getV2() * 3];
    vec_subtract(base, v1, v2);
    return vec_length(base);
}

// Derived from: http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
REAL TetMesh::distance_between_point_and_edge(Edge edge, int vertex_index) {
    static REAL cross[] = {
        0, 0, 0
    };
    static REAL temp1[] = {
        0, 0, 0
    };
    static REAL temp2[] = {
        0, 0, 0
    };

    REAL * v0 = &vertices[vertex_index * 3];
    REAL * v1 = &vertices[edge.getV1() * 3];
    REAL * v2 = &vertices[edge.getV2() * 3];
    vec_subtract(temp1, v2, v1);
    vec_subtract(temp2, v1, v0);
    vec_cross(cross, temp1, temp2);
    return (vec_length(cross) / vec_length(temp1));
}

Edge TetMesh::shortest_edge_in_set(GeometrySet<Edge> set_of_edges) {
    return edge_in_set_helper(set_of_edges, true);
}

Edge TetMesh::longest_edge_in_set(GeometrySet<Edge> set_of_edges) {
    return edge_in_set_helper(set_of_edges, false);
}

// If shortest == true, returns the shortest edge, else returns the longest edge
Edge TetMesh::edge_in_set_helper(GeometrySet<Edge> set_of_edges, bool shortest) {
    assert(set_of_edges.size() > 0);
    std::vector<Edge> edges = set_of_edges.getItems();
    int bestEdgeIndex = 0;
    REAL bestDistSq = get_edge_length(edges[0]);
    for (size_t i = 1; i < edges.size(); i++) {
        REAL distSq = get_edge_length(edges[i]);
        if ((!shortest && distSq > bestDistSq) || (shortest && distSq < bestDistSq)) {
            bestDistSq = distSq;
            bestEdgeIndex = i;
        }
    }
    Edge longestEdge = edges[bestEdgeIndex];
    return longestEdge;
}

unsigned int TetMesh::get_opposite_vertex(unsigned int tet_id, Face face) {
    for (int i = 0; i < 4; i++) {
        unsigned int v = tets[tet_id * 4 + i];
        if (face.getV1() != v && face.getV2() != v && face.getV3() != v) {
            return v;
        }
    }
    assert(false);
    return 0;
}

Face TetMesh::largest_face_in_set(GeometrySet<Face> set_of_faces) {
    assert(set_of_faces.size() > 0);
    std::vector<Face> faces = set_of_faces.getItems();
    int largestFaceIndex = 0;
    static REAL base[] = {
        0, 0, 0
    };
    REAL * v1 = &vertices[faces[0].getV1() * 3];
    REAL * v2 = &vertices[faces[0].getV2() * 3];
    vec_subtract(base, v1, v2);
    REAL b = vec_length(base);
    REAL h = distance_between_point_and_edge(Edge(faces[0].getV1(), faces[0].getV2()), faces[0].getV3());
    REAL largestFaceAreaDoubled = b * h; // Doubled because *0.5 is irrelevant
    
    for (size_t i = 1; i < faces.size(); i++) {
        v1 = &vertices[faces[i].getV1() * 3];
        v2 = &vertices[faces[i].getV2() * 3];
        vec_subtract(base, v1, v2);
        b = vec_length(base);
        h = distance_between_point_and_edge(Edge(faces[i].getV1(), faces[i].getV2()), faces[i].getV3());
        REAL faceAreaDoubled = b * h;
        if (faceAreaDoubled > largestFaceAreaDoubled) {
            largestFaceAreaDoubled = faceAreaDoubled;
            largestFaceIndex = i;
        }
    }
    Face largestFace = faces[largestFaceIndex];
    return largestFace;    
}

GeometrySet<Edge> TetMesh::get_edges_from_face(Face face) {
    GeometrySet<Edge> edges;
    edges.insert(Edge(face.getV1(), face.getV2()));
    edges.insert(Edge(face.getV1(), face.getV3()));
    edges.insert(Edge(face.getV2(), face.getV3()));
    return edges;
}

GeometrySet<Edge> TetMesh::get_edges_from_tet(int tet_id) {
    GeometrySet<Edge> edges;
    edges.insert(Edge(tets[tet_id * 4], tets[tet_id * 4 + 1]));
    edges.insert(Edge(tets[tet_id * 4], tets[tet_id * 4 + 2]));
    edges.insert(Edge(tets[tet_id * 4], tets[tet_id * 4 + 3]));
    edges.insert(Edge(tets[tet_id * 4 + 1], tets[tet_id * 4 + 2]));
    edges.insert(Edge(tets[tet_id * 4 + 1], tets[tet_id * 4 + 3]));
    edges.insert(Edge(tets[tet_id * 4 + 2], tets[tet_id * 4 + 3]));
    return edges;
}

GeometrySet<Face> TetMesh::get_faces_from_tet(int tet_id) {
    GeometrySet<Face> faces;
    faces.insert(Face(tets[tet_id * 4], tets[tet_id * 4 + 1], tets[tet_id * 4 + 2]));
    faces.insert(Face(tets[tet_id * 4], tets[tet_id * 4 + 1], tets[tet_id * 4 + 3]));
    faces.insert(Face(tets[tet_id * 4], tets[tet_id * 4 + 2], tets[tet_id * 4 + 3]));
    faces.insert(Face(tets[tet_id * 4 + 1], tets[tet_id * 4 + 2], tets[tet_id * 4 + 3]));
    return faces;
}
