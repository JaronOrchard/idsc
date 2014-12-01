

#ifndef _INDEXED_FACE_SET_H
#define _INDEXED_FACE_SET_H

#include "render/Renderable.h"
#include "tetgen.h"

class IndexedFaceSet {
    public:
        static IndexedFaceSet * load_from_obj(std::string file_name);
        static tetgenio * to_tetgenio(IndexedFaceSet & ifs);
        static IndexedFaceSet * surface_mesh_from_tetgenio(tetgenio & tet);
        static IndexedFaceSet * tet_mesh_from_tetgenio(tetgenio & tet);
        IndexedFaceSet(int num_vertices, float * vertices,
                       int num_indices, int * indices);
        ~IndexedFaceSet();
        void bind_attributes(Renderable & renderable);
        void update_attributes(Renderable & renderable);
    private:
        int num_vertices;
        float * vertices;

        int num_indices;
        int * indices;
};

#endif
