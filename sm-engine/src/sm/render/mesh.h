#pragma once

#include "sm/math/vec2.h"
#include "sm/math/vec3.h"
#include "sm/core/array.h"

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

    void init_primitive_shapes();
    const mesh_t* get_primitive_shape_mesh(primitive_shape_t shape);

	//mesh_t* init_from_obj(const char* obj_filepath);
}
