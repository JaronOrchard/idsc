
#ifndef TET_MESH_FACTORY_H
#define TET_MESH_FACTORY_H

#include "tetmesh.h"

#include "model/IndexedFaceSet.h"

class TetMeshFactory {
    public:
        static TetMesh * from_indexed_face_set(IndexedFaceSet & ifs);
        static TetMesh * create_debug_tetmesh();
        static TetMesh * create_big_debug_tetmesh();
        static TetMesh * create_collapsed_tetmesh();
};

#endif
