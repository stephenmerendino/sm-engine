#pragma once

#include "sm/math/vec2.h"
#include "sm/math/vec3.h"
#include "sm/core/array.h"
#include "sm/core/color.h"

namespace sm
{
    enum class primitive_topology_t : u8
    {
        TRIANGLE_LIST,
        LINE_LIST,
        POINT_LIST
    };

    enum class primitive_shape_t : u32
    {
        AXES,
        TETRAHEDRON,
        CUBE,
        OCTAHEDRON,
        UV_SPHERE,
        PLANE,
        QUAD,
        CONE,
        CYLINDER,
        TORUS,
        NUM_PRIMITIVE_SHAPES
    };

	struct vertex_t
	{
		vec3_t pos;
		vec2_t uv;
        vec3_t color;
	};

	struct mesh_t
	{
        primitive_topology_t topology;
		array_t<vertex_t> vertices;
		array_t<u32> indices;
	};

    vertex_t vertex_init(const vec3_t& pos, const vec2_t& uv, const vec3_t& color);
    vertex_t vertex_init(const vec3_t& pos, const vec2_t& uv, const color_f32_t& color);

    u32 mesh_add_vertex(mesh_t* mesh, const vertex_t& v);
    void mesh_add_index(mesh_t* mesh, u32 index);
    void mesh_add_vertex_and_index(mesh_t* mesh, const vertex_t& v);
    void mesh_add_triangle_indices(mesh_t* mesh, u32 index0, u32 index1, u32 index2);

    void primitive_shapes_init();
    const mesh_t* primitive_shape_get_mesh(primitive_shape_t shape);

	mesh_t* mesh_init_from_obj(sm::arena_t* arena, const char* obj_filepath);
    size_t mesh_calc_vertex_buffer_size(const mesh_t* mesh);
    size_t mesh_calc_index_buffer_size(const mesh_t* mesh);
}
