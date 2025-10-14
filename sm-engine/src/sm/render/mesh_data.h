#pragma once

#include "sm/math/vec2.h"
#include "sm/math/vec3.h"
#include "sm/core/array.h"
#include "sm/core/color.h"

namespace sm
{
    enum class mesh_topology_t : u8
    {
        TRIANGLE_LIST,
        LINE_LIST
    };

    enum class primitive_t : u32
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
        vec3_t normal;
	};

	struct cpu_mesh_t
	{
        mesh_topology_t topology;
		array_t<vertex_t> vertices;
		array_t<u32> indices;
	};

    void mesh_data_init_primitives();
    const cpu_mesh_t* mesh_data_get_primitive(primitive_t shape);

    cpu_mesh_t* mesh_data_init(arena_t* arena, mesh_topology_t topology = mesh_topology_t::TRIANGLE_LIST);
	cpu_mesh_t* mesh_data_init_from_obj(arena_t* arena, const char* obj_filepath);
    u32 mesh_data_add_vertex(cpu_mesh_t* mesh, const vertex_t& v);
    void mesh_data_add_index(cpu_mesh_t* mesh, u32 index);
    void mesh_data_add_vertex_and_index(cpu_mesh_t* mesh, const vertex_t& v);
    void mesh_data_add_triangle_indices(cpu_mesh_t* mesh, u32 index0, u32 index1, u32 index2);

    void mesh_data_add_cube(cpu_mesh_t* mesh, const vec3_t& center, f32 half_size, u32 resolution = 1, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_data_add_uv_sphere(cpu_mesh_t* mesh, const vec3_t& origin, f32 radius, u32 resolution = 64, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_data_add_quad_3d(cpu_mesh_t* mesh, const vec3_t& top_left, const vec3_t& top_right, const vec3_t& bottom_right, const vec3_t& bottom_left, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_data_add_quad_3d(cpu_mesh_t* mesh, const vec3_t& centerPos, const vec3_t& right, const vec3_t& up, f32 half_width, f32 half_height, u32 resolution = 1, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_data_add_cone(cpu_mesh_t* mesh, const vec3_t& base_center, const vec3_t& dir, f32 height, f32 radius, u32 resolution = 32, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_data_add_cylinder(cpu_mesh_t* mesh, const vec3_t& base_center, const vec3_t& dir, f32 height, f32 base_radius, u32 resolution = 32, const color_f32_t& vertex_color = color_f32_t::WHITE);
    void mesh_data_add_torus(cpu_mesh_t* mesh, const vec3_t& center, const vec3_t& normal, f32 radius, f32 thickness, u32 resolution = 32, const color_f32_t& vertex_color = color_f32_t::WHITE);

    size_t mesh_data_calc_vertex_buffer_size(const cpu_mesh_t& mesh);
    size_t mesh_data_calc_index_buffer_size(const cpu_mesh_t& mesh);
}
