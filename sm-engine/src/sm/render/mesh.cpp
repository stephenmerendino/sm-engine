#include "sm/render/mesh.h"
#include "sm/math/mat44.h"

using namespace sm;

bool s_did_init_primitive_shapes = false;
mesh_t* s_primitive_shapes[(u32)primitive_shape_t::kNumPrimitiveShapes];
arena_t* s_primitives_arena = nullptr;

static void add_quad_3d(mesh_t* mesh, const vec3_t& top_left, const vec3_t& top_right, const vec3_t& bottom_right, const vec3_t& bottom_left)
{
	u32 top_left_index = add_vertex(mesh, init_vertex(top_left, vec2_t(0.0f, 0.0f), color_f32_t::WHITE));
	u32 top_right_index = add_vertex(mesh, init_vertex(top_right, vec2_t(1.0f, 0.0f), color_f32_t::WHITE));
	u32 bottom_right_index = add_vertex(mesh, init_vertex(bottom_right, vec2_t(1.0f, 1.0f), color_f32_t::WHITE));
	u32 bottom_left_index = add_vertex(mesh, init_vertex(bottom_left, vec2_t(0.0f, 1.0f), color_f32_t::WHITE));

	add_triangle_indices(mesh, top_left_index, bottom_right_index, top_right_index);
	add_triangle_indices(mesh, top_left_index, bottom_left_index, bottom_right_index);
}

static void add_quad_3d(mesh_t* mesh, const vec3_t& centerPos, const vec3_t& right, const vec3_t& up, f32 half_width, f32 half_height, u32 resolution)
{
	vec3_t right_norm = normalized(right);
	vec3_t up_norm = normalized(up);

	f32 width_step = (half_width * 2.0f) / (f32)resolution;
	f32 height_step = (half_height * 2.0f) / (f32)resolution;

	vec3_t top_left = centerPos + (-right_norm * half_width) + (up_norm * half_height);

	for (u32 y = 0; y <= resolution; y++)
	{
		for (u32 x = 0; x <= resolution; x++)
		{
			f32 u = (f32)x / (f32)(resolution);
			f32 v = (f32)y / (f32)(resolution);
			vec3_t vert_pos = top_left + (right * width_step * (f32)x) + (-up * height_step * (f32)y);
			u32 vert_index = add_vertex(mesh, init_vertex(vert_pos, vec2_t(u, v), color_f32_t::WHITE));

			if (y > 0 && x > 0)
			{
				u32 prevVertIndex = vert_index - 1;
				u32 vertIndexLastRow = (vert_index - resolution) - 1;
				u32 prevVertIndexLastRow = vertIndexLastRow - 1;
				add_triangle_indices(mesh, vert_index, prevVertIndexLastRow, prevVertIndex);
				add_triangle_indices(mesh, vert_index, vertIndexLastRow, prevVertIndexLastRow);
			}
		}
	}
}

vertex_t sm::init_vertex(const vec3_t& pos, const vec2_t& uv, const vec3_t& color)
{
	return vertex_t(pos, uv, color);
}

vertex_t sm::init_vertex(const vec3_t& pos, const vec2_t& uv, const color_f32_t& color)
{
	return init_vertex(pos, uv, to_vec3(color));
}

u32 sm::add_vertex(mesh_t* mesh, const vertex_t& v)
{
    push(mesh->vertices, v);
    return (u32)mesh->vertices.cur_size - 1;
}

void sm::add_index(mesh_t* mesh, u32 index)
{
    push(mesh->indices, index);
}

void sm::add_vertex_and_index(mesh_t* mesh, const vertex_t& v)
{
    u32 index = add_vertex(mesh, v);
    add_index(mesh, index);
}

void sm::add_triangle_indices(mesh_t* mesh, u32 index0, u32 index1, u32 index2)
{
    add_index(mesh, index0);
    add_index(mesh, index1);
    add_index(mesh, index2);
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
		mesh_t* tetrahedron_mesh = alloc_struct(s_primitives_arena, mesh_t);
		tetrahedron_mesh->vertices = init_array<vertex_t>(s_primitives_arena, 64);
		tetrahedron_mesh->indices = init_array<u32>(s_primitives_arena, 64);
		tetrahedron_mesh->topology = primitive_topology_t::kTriangleList;

        vec3_t v0_pos{0.0f, 0.0f, 1.0f};

        f32 pitch = 0.0f;
        vec3_t v1_pos = to_vec3(vec4_t(1.0f, 0.0f, 0.0f, 0.0f) * init_rotation_y_degs(pitch));
        vec3_t v2_pos = to_vec3(vec4_t(1.0f, 0.0f, 0.0f, 0.0f) * init_rotation_y_degs(pitch) * init_rotation_z_degs(120.0f));
        vec3_t v3_pos = to_vec3(vec4_t(1.0f, 0.0f, 0.0f, 0.0f) * init_rotation_y_degs(pitch) * init_rotation_z_degs(240.0f));

        normalize(v1_pos);
        normalize(v2_pos);
        normalize(v3_pos);

        add_vertex_and_index(tetrahedron_mesh, init_vertex(v0_pos, vec2_t(0.0f, 0.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v1_pos, vec2_t(0.0f, 1.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v2_pos, vec2_t(1.0f, 0.0f), color_f32_t::WHITE));

        add_vertex_and_index(tetrahedron_mesh, init_vertex(v0_pos, vec2_t(0.0f, 0.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v2_pos, vec2_t(0.0f, 1.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v3_pos, vec2_t(1.0f, 0.0f), color_f32_t::WHITE));

        add_vertex_and_index(tetrahedron_mesh, init_vertex(v0_pos, vec2_t(0.0f, 0.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v3_pos, vec2_t(0.0f, 1.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v1_pos, vec2_t(1.0f, 0.0f), color_f32_t::WHITE));

        add_vertex_and_index(tetrahedron_mesh, init_vertex(v1_pos, vec2_t(0.0f, 0.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v3_pos, vec2_t(0.0f, 1.0f), color_f32_t::WHITE));
        add_vertex_and_index(tetrahedron_mesh, init_vertex(v2_pos, vec2_t(1.0f, 0.0f), color_f32_t::WHITE));

        s_primitive_shapes[(u32)primitive_shape_t::kTetrahedron] = tetrahedron_mesh;
	}

    // cube
    {
		mesh_t* cube_mesh = alloc_struct(s_primitives_arena, mesh_t);
		cube_mesh->vertices = init_array<vertex_t>(s_primitives_arena, 64);
		cube_mesh->indices = init_array<u32>(s_primitives_arena, 64);
		cube_mesh->topology = primitive_topology_t::kTriangleList;

        add_quad_3d(cube_mesh, vec3_t::WORLD_FORWARD * 0.5f,  vec3_t::WORLD_LEFT,         vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        add_quad_3d(cube_mesh, vec3_t::WORLD_LEFT * 0.5f,     vec3_t::WORLD_BACKWARD,     vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        add_quad_3d(cube_mesh, vec3_t::WORLD_BACKWARD * 0.5f, vec3_t::WORLD_RIGHT,        vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        add_quad_3d(cube_mesh, vec3_t::WORLD_RIGHT * 0.5f,    vec3_t::WORLD_FORWARD,      vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        add_quad_3d(cube_mesh, vec3_t::WORLD_UP * 0.5f,       vec3_t::WORLD_RIGHT,        vec3_t::WORLD_FORWARD, 0.5f, 0.5f, 1);
        add_quad_3d(cube_mesh, vec3_t::WORLD_DOWN * 0.5f,     vec3_t::WORLD_LEFT,         vec3_t::WORLD_FORWARD, 0.5f, 0.5f, 1);

        s_primitive_shapes[(u32)primitive_shape_t::kCube] = cube_mesh;
    }

    // octahedron
    {
		mesh_t* octahedron_mesh = alloc_struct(s_primitives_arena, mesh_t);
		octahedron_mesh->vertices = init_array<vertex_t>(s_primitives_arena, 64);
		octahedron_mesh->indices = init_array<u32>(s_primitives_arena, 64);
		octahedron_mesh->topology = primitive_topology_t::kTriangleList;

        vec3_t v0_pos(0.0f, 0.0f, 1.0f);
        vec3_t v1_pos(1.0f, 0.0f, 0.0f);
        vec3_t v2_pos(0.0f, 1.0f, 0.0f);
        vec3_t v3_pos(-1.0f, 0.0f, 0.0f);
        vec3_t v4_pos(0.0f, -1.0f, 0.0f);
        vec3_t v5_pos(0.0f, 0.0f, -1.0f);

        {
            add_vertex_and_index(octahedron_mesh, init_vertex(v0_pos, vec2_t(0.0f, 0.0f), color_f32_t::WHITE));
            add_vertex_and_index(octahedron_mesh, init_vertex(v1_pos, vec2_t(0.0f, 1.0f), color_f32_t::WHITE));
            add_vertex_and_index(octahedron_mesh, init_vertex(v2_pos, vec2_t(1.0f, 0.0f), color_f32_t::WHITE));
        }

        //{
        //    U32 i0 = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //    U32 i1 = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //    U32 i2 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
        //    builder.AddTriangle(i0, i1, i2);
        //}

        //{
        //    U32 i0 = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //    U32 i1 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //    U32 i2 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
        //    builder.AddTriangle(i0, i1, i2);
        //}

        //{
        //    U32 i0 = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //    U32 i1 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //    U32 i2 = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
        //    builder.AddTriangle(i0, i1, i2);
        //}

        //{
        //    U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //    U32 i1 = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
        //    U32 i2 = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //    builder.AddTriangle(i0, i1, i2);
        //}

        //{
        //    U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //    U32 i1 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
        //    U32 i2 = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //    builder.AddTriangle(i0, i1, i2);
        //}

        //{
        //    U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //    U32 i1 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
        //    U32 i2 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //    builder.AddTriangle(i0, i1, i2);
        //}

        //{
        //    U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
        //    U32 i1 = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
        //    U32 i2 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
        //    builder.AddTriangle(i0, i1, i2);
        //}
    }

    // uv sphere
    {
    }

    // plane
    {
    }

    // quad
    {
    }

    // cone
    {
    }

    // cylinder
    {
    }

    // torus
    {
    }

	s_did_init_primitive_shapes = true;
}

const mesh_t* sm::get_primitive_shape_mesh(primitive_shape_t shape)
{
	SM_ASSERT(s_did_init_primitive_shapes);
	return s_primitive_shapes[(u32)shape];
}