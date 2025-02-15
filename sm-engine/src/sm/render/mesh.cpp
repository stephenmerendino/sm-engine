#include "sm/render/mesh.h"

using namespace sm;

bool s_did_init_primitive_shapes = false;
mesh_t* s_primitive_shapes[(u32)primitive_shape_t::kNumPrimitiveShapes];
arena_t* s_primitives_arena = nullptr;

void sm::init_primitive_shapes()
{
	s_primitives_arena = init_arena(MiB(1));

	// axes
	{
		mesh_t* axes_mesh = alloc_struct(s_primitives_arena, mesh_t);
		axes_mesh->vertices = init_array<vertex_t>(s_primitives_arena, 10);
		axes_mesh->indices = init_array<u32>(s_primitives_arena, 10);
	}

	s_did_init_primitive_shapes = true;
}

const mesh_t* sm::get_primitive_shape_mesh(primitive_shape_t shape)
{
	SM_ASSERT(s_did_init_primitive_shapes);
	return s_primitive_shapes[(u32)shape];
}