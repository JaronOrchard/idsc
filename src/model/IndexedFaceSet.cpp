
#include "IndexedFaceSet.h"

#include <fstream>
#include <vector>

tetgenio * IndexedFaceSet::to_tetgenio(IndexedFaceSet & ifs) {
    return NULL;
}

IndexedFaceSet * IndexedFaceSet::from_tetgenio(tetgenio & tet) {
    int num_vertices = tet.numberofpoints;
    float * vertex_buffer = (float *) malloc(num_vertices * 3 * sizeof(float));
    memcpy(vertex_buffer, tet.pointlist, num_vertices * 3 * sizeof(float));

    int num_indices = tet.numberoftrifaces * 3;
    int * index_buffer = (int *) malloc(num_indices * sizeof(int));
    memcpy(index_buffer, tet.trifacelist, num_indices * sizeof(int));

    return new IndexedFaceSet(num_vertices, vertex_buffer, num_indices, index_buffer);
}

IndexedFaceSet * IndexedFaceSet::load_from_obj(std::string file_name) {
    std::ifstream input(file_name);
    std::vector<float> vertices;
    std::vector<int> indices;
    for (std::string line; getline(input, line);) {
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

