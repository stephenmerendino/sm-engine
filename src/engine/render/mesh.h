#pragma once

#include "engine/core/hash.h"
#include "engine/core/types.h"
#include "engine/math/mat44.h"
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
    kUnitAxes,
    kUnitTetrahedron,
    kUnitHexahedron,
    kUnitCube = kUnitHexahedron,
    kUnitOctahedron,
    kUnitUvSphere,
    kUnitPlane,
    kUnitCone,
    kUnitCylinder,
    kUnitTorus
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
// mesh_t* mesh_load_from_usd();
// mesh_t* mesh_load_from_fbx();
// mesh_t* mesh_load_from_gltf();

void mesh_build_quad_3d(mesh_t& mesh, const vec3& center_pos, const vec3& right, const vec3& up, f32 half_width, f32 half_height, u32 resolution = 1);
void mesh_build_quad_3d(mesh_t& mesh, const vec3& top_left, const vec3& top_right, const vec3& bottom_right, const vec3& bottom_left);
void mesh_build_axes_lines_3d(mesh_t& mesh, const vec3& origin, const vec3& i, const vec3& j, const vec3& k);
void mesh_build_uv_sphere(mesh_t& mesh, vec3& origin, f32 radius, u32 resolution = 32);
void mesh_build_cube(mesh_t& mesh, const vec3& center, f32 radius, u32 resolution = 1);
void mesh_build_frustum(mesh_t& mesh, const mat44& view_projection);
void mesh_build_cylinder(mesh_t& mesh, const vec3& base_center, const vec3& dir, f32 height, f32 base_radius, f32 resolution);
void mesh_build_cone(mesh_t& mesh, const vec3& base_center, const vec3& dir, f32 height, f32 base_radius, f32 resolution);
//void mesh_build_arrow(mesh_t& mesh, const vec3& origin, const vec3& dir, f32 length, f32 width);
//void mesh_build_arrow_tip_to_origin(mesh_t& mesh, const vec3& tip_position, const vec3& dir_to_arrow_origin, f32 length, f32 width);

mesh_t* mesh_load_unit_axes();
mesh_t* mesh_load_unit_tetrahedron();
mesh_t* mesh_load_unit_cube();
mesh_t* mesh_load_unit_octahedron();
mesh_t* mesh_load_unit_uv_sphere();
mesh_t* mesh_load_unit_plane();
mesh_t* mesh_load_unit_cone();
mesh_t* mesh_load_unit_cylinder();
mesh_t* mesh_load_unit_torus();
