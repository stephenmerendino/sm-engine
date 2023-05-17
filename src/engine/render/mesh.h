#pragma once

#include "engine/core/types.h"
#include "engine/render/vertex.h"

#include <vector>

struct mesh_t
{
    std::vector<vertex_pct_t> m_vertices;
    std::vector<u32> m_indices;
};

size_t mesh_calc_vertex_buffer_size(mesh_t* mesh);
size_t mesh_calc_index_buffer_size(mesh_t* mesh);

mesh_t* mesh_load_from_obj(const char* obj_filepath);
mesh_t* mesh_load_cube();
mesh_t* mesh_load_axes();
