#pragma once

#include "engine/core/hash.h"
#include "engine/core/types.h"
#include "engine/render/vertex.h"

#include <vector>

enum class PrimitiveTopology : u8
{
    kTriangleList,
    kLineList,
    kPointList
};

enum PrimitiveMeshType : u32
{
    kAxes,
    kTetrahedron,
    kHexahedron,
    kCube = kHexahedron,
    kOctahedron,
    kUvSphere,
    kPlane
};

struct mesh_t
{
    PrimitiveTopology topology;
    std::vector<vertex_pct_t> vertices;
    std::vector<u32> indices;
};

size_t mesh_calc_vertex_buffer_size(mesh_t* mesh);
size_t mesh_calc_index_buffer_size(mesh_t* mesh);

mesh_t* mesh_load_from_obj(const char* obj_filepath);

mesh_t* mesh_load_axes();

mesh_t* mesh_load_tetrahedron();
mesh_t* mesh_load_cube(); // hexahedron
mesh_t* mesh_load_octahedron();
//mesh_t* mesh_load_dodecahedron();
//mesh_t* mesh_load_icosahedron();
//mesh_t* mesh_load_ico_sphere();
mesh_t* mesh_load_uv_sphere();
mesh_t* mesh_load_plane();
