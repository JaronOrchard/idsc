
#include "IndexedFaceSet.h"

#include <fstream>
#include <vector>

tetgenio * IndexedFaceSet::to_tetgenio(IndexedFaceSet & ifs) {
    tetgenio * out = new tetgenio();

    out->firstnumber = 0;
    out->numberofpoints = ifs.num_vertices;
    out->pointlist = new REAL[ifs.num_vertices * 3];
    // cant use memcpy because out->pointlist could be double precision
    for (int i = 0; i < ifs.num_vertices * 3; i++) {
        out->pointlist[i] = ifs.vertices[i];
    }

    out->numberoffacets = ifs.num_indices / 3;
    out->facetlist = new tetgenio::facet[out->numberoffacets];
    out->facetmarkerlist = new int[out->numberoffacets];
    for (int i = 0; i < out->numberoffacets; i++) {
        out->facetlist[i].numberofholes = 0;
        out->facetlist[i].holelist = NULL;
        out->facetlist[i].numberofpolygons = 1;
        out->facetlist[i].polygonlist = new tetgenio::polygon[1];
        out->facetlist[i].polygonlist[0].numberofvertices = 3;
        out->facetlist[i].polygonlist[0].vertexlist = new int[3];
        out->facetlist[i].polygonlist[0].vertexlist[0] = ifs.indices[i * 3];
        out->facetlist[i].polygonlist[0].vertexlist[1] = ifs.indices[i * 3 + 1];
        out->facetlist[i].polygonlist[0].vertexlist[2] = ifs.indices[i * 3 + 2];
        out->facetmarkerlist[i] = 0;
    }

    return out;
}

IndexedFaceSet * IndexedFaceSet::surface_mesh_from_tetgenio(tetgenio & tet) {
    int num_vertices = tet.numberofpoints;
    float * vertex_buffer = (float *) malloc(num_vertices * 3 * sizeof(float));
    // cant use memcpy because tet.pointlist could be double precision
    for (int i = 0; i < num_vertices * 3; i++) {
        vertex_buffer[i] = tet.pointlist[i];
    }

    int num_indices = tet.numberoftrifaces * 3;
    int * index_buffer = (int *) malloc(num_indices * sizeof(int));
    memcpy(index_buffer, tet.trifacelist, num_indices * sizeof(int));

    return new IndexedFaceSet(num_vertices, vertex_buffer, num_indices, index_buffer);
}

IndexedFaceSet * IndexedFaceSet::tet_mesh_from_tetgenio(tetgenio & tet) {
    int num_vertices = tet.numberofpoints;
    float * vertex_buffer = (float *) malloc(num_vertices * 3 * sizeof(float));
    // cant use memcpy because tet.pointlist could be double precision
    for (int i = 0; i < num_vertices * 3; i++) {
        vertex_buffer[i] = tet.pointlist[i];
    }

    int num_indices = tet.numberoftetrahedra * 4 * 3;
    int * index_buffer = (int *) malloc(num_indices * sizeof(int));
    for (int i = 0; i < tet.numberoftetrahedra; i++) {
        index_buffer[i * 12] = tet.tetrahedronlist[i * 4];
        index_buffer[i * 12 + 1] = tet.tetrahedronlist[i * 4 + 1];
        index_buffer[i * 12 + 2] = tet.tetrahedronlist[i * 4 + 2];

        index_buffer[i * 12 + 3] = tet.tetrahedronlist[i * 4];
        index_buffer[i * 12 + 4] = tet.tetrahedronlist[i * 4 + 1];
        index_buffer[i * 12 + 5] = tet.tetrahedronlist[i * 4 + 3];

        index_buffer[i * 12 + 6] = tet.tetrahedronlist[i * 4];
        index_buffer[i * 12 + 7] = tet.tetrahedronlist[i * 4 + 2];
        index_buffer[i * 12 + 8] = tet.tetrahedronlist[i * 4 + 3];

        index_buffer[i * 12 + 9] = tet.tetrahedronlist[i * 4 + 1];
        index_buffer[i * 12 + 10] = tet.tetrahedronlist[i * 4 + 2];
        index_buffer[i * 12 + 11] = tet.tetrahedronlist[i * 4 + 3];
    }

    return new IndexedFaceSet(num_vertices, vertex_buffer, num_indices, index_buffer);
}

IndexedFaceSet * IndexedFaceSet::load_from_obj(std::string file_name) {
    std::ifstream input(file_name);
    std::vector<float> vertices;
    std::vector<int> indices;
    for (std::string line; getline(input, line);) {
        if (line.size() <= 0) {
            continue;
        }
        unsigned int start_ind;
        bool in_space = false;
        for (start_ind = 0; start_ind < line.size(); start_ind++) {
            if (line.at(start_ind) == ' ') {
                in_space = true;
            }
            if (line.at(start_ind) != ' ' && in_space) {
                break;
            }
        }
        switch (line.at(0)) {
            case 'v':
                for (int i = 0; i < 3; i++) {
                    unsigned int end_ind = start_ind;
                    while (end_ind != line.size() && line.at(end_ind) != ' ') {
                        end_ind++;
                    }
                    vertices.push_back(atof(line.substr(start_ind, end_ind - start_ind).c_str()));
                    start_ind = end_ind + 1;
                }
                break;
            case 'f':
                for (int i = 0; i < 3; i++) {
                    unsigned int end_ind = start_ind;
                    while (end_ind != line.size() && line.at(end_ind) != ' ') {
                        end_ind++;
                    }
                    indices.push_back(atoi(line.substr(start_ind, end_ind - start_ind).c_str()) - 1);
                    start_ind = end_ind + 1;
                }
                // support quad faces
                if (start_ind < line.size()) {
                    unsigned int end_ind = start_ind;
                    while (end_ind != line.size() && line.at(end_ind) != ' ') {
                        end_ind++;
                    }
                    int quad_start = indices.size() - 3;
                    indices.push_back(indices[quad_start]);
                    indices.push_back(indices[quad_start + 2]);
                    indices.push_back(atoi(line.substr(start_ind, end_ind - start_ind).c_str()) - 1);
                }
                break;
            default:
                break;
        }
    }

    float * vertex_buffer = (float *) malloc(vertices.size() * sizeof(float));
    std::copy(vertices.begin(), vertices.end(), vertex_buffer);
    int * index_buffer = (int *) malloc(indices.size() * sizeof(int));
    std::copy(indices.begin(), indices.end(), index_buffer);
    return new IndexedFaceSet(vertices.size() / 3, vertex_buffer, indices.size(), index_buffer);
}

IndexedFaceSet::IndexedFaceSet(int num_vertices, float * vertices,
                               int num_indices, int * indices) {
    this->num_vertices = num_vertices;
    this->vertices = vertices;

    this->num_indices = num_indices;
    this->indices = indices;
}

IndexedFaceSet::~IndexedFaceSet() {
    free(vertices);
    free(indices);
}

void IndexedFaceSet::bind_attributes(Renderable & renderable) {
    renderable.bind_attribute(vertices, num_vertices * 3 * sizeof(float), 3, "vertex_position");
    renderable.bind_indices(indices, num_indices * sizeof(int));
}

void IndexedFaceSet::update_attributes(Renderable & renderable) {
    renderable.bind_attribute(vertices, num_vertices * 3 * sizeof(float), 3, "vertex_position");
}

