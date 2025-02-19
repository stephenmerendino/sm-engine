#include "sm/render/mesh.h"

using namespace sm;

bool s_did_init_primitive_shapes = false;
mesh_t* s_primitive_shapes[(u32)primitive_shape_t::kNumPrimitiveShapes];
arena_t* s_primitives_arena = nullptr;

vertex_t sm::init_vertex(const vec3_t& pos, const vec2_t& uv, const vec3_t& color)
{
	return { pos, uv, color };
}

vertex_t sm::init_vertex(const vec3_t& pos, const vec2_t& uv, const color_f32_t& color)
{
	return init_vertex(pos, uv, to_vec3(color));
}

void sm::add_vertex_and_index(mesh_t* mesh, const vertex_t& v)
{
	push(mesh->vertices, v);
	push(mesh->indices, (u32)mesh->vertices.cur_size);
}

void sm::init_primitive_shapes()
{
	s_primitives_arena = init_arena(MiB(1));

	// axes
	{
		mesh_t* axes_mesh = alloc_struct(s_primitives_arena, mesh_t);
		axes_mesh->vertices = init_array<vertex_t>(s_primitives_arena, 64);
		axes_mesh->indices = init_array<u32>(s_primitives_arena, 64);
		axes_mesh->topology = primitive_topology_t::kLineList;

		add_vertex_and_index(axes_mesh, init_vertex(vec3_t::ZERO, vec2_t::ZERO, color_f32_t::RED));
		add_vertex_and_index(axes_mesh, init_vertex(vec3_t::X_AXIS, vec2_t::ZERO, color_f32_t::RED));

		add_vertex_and_index(axes_mesh, init_vertex(vec3_t::ZERO, vec2_t::ZERO, color_f32_t::GREEN));
		add_vertex_and_index(axes_mesh, init_vertex(vec3_t::Y_AXIS, vec2_t::ZERO, color_f32_t::GREEN));

		add_vertex_and_index(axes_mesh, init_vertex(vec3_t::ZERO, vec2_t::ZERO, color_f32_t::BLUE));
		add_vertex_and_index(axes_mesh, init_vertex(vec3_t::Z_AXIS, vec2_t::ZERO, color_f32_t::BLUE));

		s_primitive_shapes[(u32)primitive_shape_t::kAxes] = axes_mesh;
	}

    // tetrahedron
	{
        //MeshBuilder builder;
        //builder.Begin();
        //builder.SetTopology(PrimitiveTopology::kTriangleList);

        //Vec3 v0_pos(0.0f, 0.0f, 1.0f);

        //F32 pitch = 0.0f;
        //Vec3 v1_pos = (Vec4(1.0f, 0.0f, 0.0f, 0.0f) * Mat44::CreateRotationYDegs(pitch)).xyz;
        //Vec3 v2_pos = (Vec4(1.0f, 0.0f, 0.0f, 0.0f) * Mat44::CreateRotationYDegs(pitch) * Mat44::CreateRotationZDegs(120.0f)).xyz;
        //Vec3 v3_pos = (Vec4(1.0f, 0.0f, 0.0f, 0.0f) * Mat44::CreateRotationYDegs(pitch) * Mat44::CreateRotationZDegs(240.0f)).xyz;

        //v1_pos.Normalize();
        //v2_pos.Normalize();
        //v3_pos.Normalize();

        //U32 v0_index  = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //U32 v1_index  = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //U32 v2_index  = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

        //U32 v3_index  = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //U32 v4_index  = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //U32 v5_index  = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

        //U32 v6_index  = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //U32 v7_index  = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //U32 v8_index  = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

        //U32 v9_index  = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //U32 v10_index = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //U32 v11_index = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

        //builder.AddTriangle(v0_index, v1_index, v2_index);
        //builder.AddTriangle(v3_index, v4_index, v5_index);
        //builder.AddTriangle(v6_index, v7_index, v8_index);
        //builder.AddTriangle(v9_index, v10_index, v11_index);

		mesh_t* axes_mesh = alloc_struct(s_primitives_arena, mesh_t);
		axes_mesh->vertices = init_array<vertex_t>(s_primitives_arena, 64);
		axes_mesh->indices = init_array<u32>(s_primitives_arena, 64);
		axes_mesh->topology = primitive_topology_t::kLineList;

        vec3_t v0_pos{0.0f, 0.0f, 1.0f};
	}

    //kCube
    //kOctahedron
    //kUvSphere
    //kPlane
    //kQuad
    //kCone
    //kCylinder
    //kTorus

	s_did_init_primitive_shapes = true;
}

const mesh_t* sm::get_primitive_shape_mesh(primitive_shape_t shape)
{
	SM_ASSERT(s_did_init_primitive_shapes);
	return s_primitive_shapes[(u32)shape];
}