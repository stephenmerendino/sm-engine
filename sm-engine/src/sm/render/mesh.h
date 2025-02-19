#pragma once

#include "sm/math/vec2.h"
#include "sm/math/vec3.h"
#include "sm/core/array.h"
#include "sm/core/color.h"

namespace sm
{
    enum class primitive_topology_t : u8
    {
        kTriangleList,
        kLineList,
        kPointList
    };

    enum class primitive_shape_t : u32
    {
        kAxes,
        kTetrahedron,
        kCube,
        kOctahedron,
        kUvSphere,
        kPlane,
        kQuad,
        kCone,
        kCylinder,
        kTorus,
        kNumPrimitiveShapes
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

    vertex_t init_vertex(const vec3_t& pos, const vec2_t& uv, const vec3_t& color);
    vertex_t init_vertex(const vec3_t& pos, const vec2_t& uv, const color_f32_t& color);
    void add_vertex_and_index(mesh_t* mesh, const vertex_t& v);

    void init_primitive_shapes();
    const mesh_t* get_primitive_shape_mesh(primitive_shape_t shape);

	//mesh_t* init_from_obj(const char* obj_filepath);
}
