#pragma once

#include "engine/core/hash.h"
#include "engine/core/types.h"
#include "engine/render/vertex.h"

#include <vector>

typedef hash_id_t mesh_id_t;

enum class PrimitiveTopology : u8
{
    kTriangleList,
    kLineList
};

struct mesh_t
{
    PrimitiveTopology topology;
    std::vector<vertex_pct_t> m_vertices;
    std::vector<u32> m_indices;
};

size_t mesh_calc_vertex_buffer_size(mesh_t* mesh);
size_t mesh_calc_index_buffer_size(mesh_t* mesh);

mesh_t* mesh_load_from_obj(const char* obj_filepath);
mesh_t* mesh_load_cube();
mesh_t* mesh_load_axes();
