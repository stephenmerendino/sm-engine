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

void mesh_build_quad_3d(mesh_t& mesh, const vec3& pos, const vec3& right, const vec3& up, f32 width, f32 height);
//void mesh_build_axes(mesh_t& mesh);
//void mesh_build_tetrahedron(mesh_t& mesh);
//void mesh_build_cube(mesh_t& mesh);
//void mesh_build_octahedron(mesh_t& mesh);
//void mesh_build_uv_sphere(mesh_t& mesh);
//void mesh_build_plane(mesh_t& mesh);
//void mesh_build_cone(mesh_t& mesh);
//void mesh_build_cylinder(mesh_t& mesh);
//void mesh_build_torus(mesh_t& mesh);

mesh_t* mesh_load_unit_axes();
mesh_t* mesh_load_unit_tetrahedron();
mesh_t* mesh_load_unit_cube();
mesh_t* mesh_load_unit_octahedron();
mesh_t* mesh_load_unit_uv_sphere();
mesh_t* mesh_load_unit_plane();
mesh_t* mesh_load_unit_cone();
mesh_t* mesh_load_unit_cylinder();
mesh_t* mesh_load_unit_torus();
