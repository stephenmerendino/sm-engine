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
        CUBE,
        UV_SPHERE,
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

    void primitive_shapes_init();
    const mesh_t* primitive_shape_get_mesh(primitive_shape_t shape);

    mesh_t* mesh_init(arena_t* arena, primitive_topology_t topology = primitive_topology_t::TRIANGLE_LIST);
	mesh_t* mesh_init_from_obj(arena_t* arena, const char* obj_filepath);
    u32 mesh_add_vertex(mesh_t* mesh, const vertex_t& v);
    void mesh_add_index(mesh_t* mesh, u32 index);
    void mesh_add_vertex_and_index(mesh_t* mesh, const vertex_t& v);
    void mesh_add_triangle_indices(mesh_t* mesh, u32 index0, u32 index1, u32 index2);

    void mesh_add_cube(mesh_t* mesh, const vec3_t& center, u32 resolution = 1, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_add_uv_sphere(mesh_t* mesh, const vec3_t& origin, f32 radius, u32 resolution = 64, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_add_quad_3d(mesh_t* mesh, const vec3_t& top_left, const vec3_t& top_right, const vec3_t& bottom_right, const vec3_t& bottom_left, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_add_quad_3d(mesh_t* mesh, const vec3_t& centerPos, const vec3_t& right, const vec3_t& up, f32 half_width, f32 half_height, u32 resolution = 1, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_add_cone(mesh_t* mesh, const vec3_t& base_center, const vec3_t& dir, f32 height, f32 base_radius, u32 resolution = 32, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_add_cylinder(mesh_t* mesh, const vec3_t& base_center, const vec3_t& dir, f32 height, f32 base_radius, u32 resolution = 32, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_add_torus(mesh_t* mesh, u32 resolution = 32, const color_f32_t& vertex_color = color_f32_t::WHITE);

    size_t mesh_calc_vertex_buffer_size(const mesh_t* mesh);
    size_t mesh_calc_index_buffer_size(const mesh_t* mesh);
}
