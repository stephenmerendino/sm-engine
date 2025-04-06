#include "sm/render/mesh.h"
#include "sm/math/mat44.h"
#include "sm/config.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "third_party/tinyobjloader/tiny_obj_loader.h"

// Used here because its necessary for tinyobj
#include <vector>
#include <string>

using namespace sm;

bool s_did_init_primitive_shapes = false;
mesh_t* s_primitive_shapes[(u32)primitive_shape_t::NUM_PRIMITIVE_SHAPES];
arena_t* s_primitives_arena = nullptr;

void sm::primitive_shapes_init()
{
	s_primitives_arena = arena_init(MiB(2));

	// axes
	{
		mesh_t* axes_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		axes_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		axes_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		axes_mesh->topology = primitive_topology_t::LINE_LIST;

        mesh_add_vertex_and_index(axes_mesh, { .pos = vec3_t::ZERO, .uv = vec2_t::ZERO, .color = to_vec3(color_f32_t::RED) });
		mesh_add_vertex_and_index(axes_mesh, { .pos = vec3_t::X_AXIS, .uv = vec2_t::ZERO, .color = to_vec3(color_f32_t::RED) });

		mesh_add_vertex_and_index(axes_mesh, { .pos = vec3_t::ZERO, .uv = vec2_t::ZERO, .color = to_vec3(color_f32_t::GREEN) });
		mesh_add_vertex_and_index(axes_mesh, { .pos = vec3_t::Y_AXIS, .uv = vec2_t::ZERO, .color = to_vec3(color_f32_t::GREEN) });

		mesh_add_vertex_and_index(axes_mesh, { .pos = vec3_t::ZERO, .uv = vec2_t::ZERO, .color = to_vec3(color_f32_t::BLUE) });
		mesh_add_vertex_and_index(axes_mesh, { .pos = vec3_t::Z_AXIS, .uv = vec2_t::ZERO, .color = to_vec3(color_f32_t::BLUE) });

		s_primitive_shapes[(u32)primitive_shape_t::AXES] = axes_mesh;
	}

    // tetrahedron
	{
		mesh_t* tetrahedron_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		tetrahedron_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		tetrahedron_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		tetrahedron_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        vec3_t v0_pos{0.0f, 0.0f, 1.0f};

        f32 pitch = 0.0f;
        vec3_t v1_pos = to_vec3(vec4_t(1.0f, 0.0f, 0.0f, 0.0f) * init_rotation_y_degs(pitch));
        vec3_t v2_pos = to_vec3(vec4_t(1.0f, 0.0f, 0.0f, 0.0f) * init_rotation_y_degs(pitch) * init_rotation_z_degs(120.0f));
        vec3_t v3_pos = to_vec3(vec4_t(1.0f, 0.0f, 0.0f, 0.0f) * init_rotation_y_degs(pitch) * init_rotation_z_degs(240.0f));

        normalize(v1_pos);
        normalize(v2_pos);
        normalize(v3_pos);

        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v1_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v2_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });

        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v2_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v3_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });

        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v3_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v1_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });

        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v1_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v3_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        mesh_add_vertex_and_index(tetrahedron_mesh, { .pos = v2_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });

        s_primitive_shapes[(u32)primitive_shape_t::TETRAHEDRON] = tetrahedron_mesh;
	}

    // cube
    {
		mesh_t* cube_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		cube_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		cube_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		cube_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        mesh_add_quad_3d(cube_mesh, vec3_t::WORLD_FORWARD * 0.5f,  vec3_t::WORLD_LEFT,         vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        mesh_add_quad_3d(cube_mesh, vec3_t::WORLD_LEFT * 0.5f,     vec3_t::WORLD_BACKWARD,     vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        mesh_add_quad_3d(cube_mesh, vec3_t::WORLD_BACKWARD * 0.5f, vec3_t::WORLD_RIGHT,        vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        mesh_add_quad_3d(cube_mesh, vec3_t::WORLD_RIGHT * 0.5f,    vec3_t::WORLD_FORWARD,      vec3_t::WORLD_UP, 0.5f, 0.5f, 1);
        mesh_add_quad_3d(cube_mesh, vec3_t::WORLD_UP * 0.5f,       vec3_t::WORLD_RIGHT,        vec3_t::WORLD_FORWARD, 0.5f, 0.5f, 1);
        mesh_add_quad_3d(cube_mesh, vec3_t::WORLD_DOWN * 0.5f,     vec3_t::WORLD_LEFT,         vec3_t::WORLD_FORWARD, 0.5f, 0.5f, 1);

        s_primitive_shapes[(u32)primitive_shape_t::CUBE] = cube_mesh;
    }

    // octahedron
    {
		mesh_t* octahedron_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		octahedron_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		octahedron_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		octahedron_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        vec3_t v0_pos(0.0f, 0.0f, 1.0f);
        vec3_t v1_pos(1.0f, 0.0f, 0.0f);
        vec3_t v2_pos(0.0f, 1.0f, 0.0f);
        vec3_t v3_pos(-1.0f, 0.0f, 0.0f);
        vec3_t v4_pos(0.0f, -1.0f, 0.0f);
        vec3_t v5_pos(0.0f, 0.0f, -1.0f);

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v1_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v2_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v2_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v3_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v3_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v4_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v4_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v1_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v0_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v4_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v1_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v5_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v3_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v2_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v5_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v4_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v3_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        {
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v5_pos, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v1_pos, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            mesh_add_vertex_and_index(octahedron_mesh, { .pos = v4_pos, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
        }

        s_primitive_shapes[(u32)primitive_shape_t::OCTAHEDRON] = octahedron_mesh;
    }

    // uv sphere
    {
		mesh_t* uv_sphere_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		uv_sphere_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		uv_sphere_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		uv_sphere_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        vec3_t origin(0.0f, 0.0f, 0.0f);
        u32 resolution = 64;
        f32 radius = 1.0f;

        f32 u_delta_deg = 360.0f / (f32)resolution;
        f32 v_delta_deg = 180.0f / (f32)resolution;

        for (u32 v_slice = 0; v_slice <= resolution; v_slice++)
        {
            for (u32 u_slice = 0; u_slice <= resolution; u_slice++)
            {
                f32 u = (f32)u_slice / (f32)resolution;
                f32 v = (f32)v_slice / (f32)resolution;
                vec2_t uv(u, v);

                f32 y_deg = -90.0f + ((f32)v_slice * v_delta_deg);
                f32 z_deg = (f32)u_slice * u_delta_deg;

                vec4_t pos = vec4_t(radius, 0.0f, 0.0f, 0.0f) * init_rotation_y_degs(y_deg) * init_rotation_z_degs(z_deg);
                vec3_t finalPos = origin + to_vec3(pos);

                u32 vert_index = mesh_add_vertex(uv_sphere_mesh, { .pos = finalPos, .uv = uv, .color = to_vec3(color_f32_t::WHITE) });

                if (v_slice == 0)
                {
                    continue;
                }

                // normal triangle add
                if (u_slice > 0)
                {
                    u32 prev_vert_index = vert_index - 1;
                    u32 vert_index_last_slice = (vert_index - resolution) - 1;
                    u32 prev_vert_index_last_slice = vert_index_last_slice - 1;

                    mesh_add_triangle_indices(uv_sphere_mesh, vert_index, vert_index_last_slice, prev_vert_index_last_slice);
                    mesh_add_triangle_indices(uv_sphere_mesh, vert_index, prev_vert_index_last_slice, prev_vert_index);
                }
            }
        }

        s_primitive_shapes[(u32)primitive_shape_t::UV_SPHERE] = uv_sphere_mesh;
    }

    // plane
    {
		mesh_t* plane_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		plane_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		plane_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		plane_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        mesh_add_quad_3d(plane_mesh, vec3_t::ZERO, vec3_t::WORLD_RIGHT, vec3_t::WORLD_FORWARD, 0.5f, 0.5f, 1);

        s_primitive_shapes[(u32)primitive_shape_t::PLANE] = plane_mesh;
    }

    // quad
    {
		mesh_t* quad_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		quad_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		quad_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		quad_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        // this is a full screen quad in ndc space
        // todo: probably should break this out into its own thing
        mesh_add_quad_3d(quad_mesh, vec3_t::ZERO, vec3_t(1.0f, 0.0f, 0.0f), vec3_t(0.0f, -1.0f, 0.0f), 1.0f, 1.0f, 1);

        s_primitive_shapes[(u32)primitive_shape_t::QUAD] = quad_mesh;
    }

    // cone
    {
		mesh_t* cone_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		cone_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		cone_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		cone_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        vec3_t base_center = vec3_t::ZERO;
        vec3_t dir = vec3_t::WORLD_UP;
        f32 height = 1.0f;
        f32 base_radius = 1.0f;
        u32 resolution = 32;

        vec3_t k = normalized(dir);
        mat44_t local_to_world = init_basis_from_k(k);
        scale(local_to_world, base_radius, base_radius, height);
        translate(local_to_world, base_center);

        f32 theta_deg = 360.0f / (f32)resolution;

        vec3_t top_local(0.0f, 0.0f, 1.0f);
        vec3_t bot_local(0.0f, 0.0f, 0.0f);
        vec3_t top_ws = transform_point(local_to_world, top_local);
        vec3_t bot_ws = transform_point(local_to_world, bot_local);

        for (u32 slice = 0; slice <= resolution; slice++)
        {
            vec3_t p0(cos_deg(theta_deg * slice), sin_deg(theta_deg * slice), 0.0f);
            vec3_t p1(cos_deg(theta_deg * (slice + 1)), sin_deg(theta_deg * (slice + 1)), 0.0f);

            vec3_t p0_ws = transform_point(local_to_world, p0);
            vec3_t p1_ws = transform_point(local_to_world, p1);

            f32 u0 = remap(p0.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 u1 = remap(p1.x, -1.0f, 1.0f, 0.0f, 1.0f);

            f32 v0 = remap(p0.y, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v1 = remap(p1.y, -1.0f, 1.0f, 0.0f, 1.0f);

            // top triangle 1
            {
                mesh_add_vertex_and_index(cone_mesh, { .pos = top_ws, .uv = vec2_t(0.5, 0.5f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p0_ws, .uv = vec2_t(u0, v0), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p1_ws, .uv = vec2_t(u1, v1), .color = to_vec3(color_f32_t::WHITE) });
            }

            // top triangle 2
            {
                mesh_add_vertex_and_index(cone_mesh, { .pos = top_ws, .uv = vec2_t(0.5f, 0.5), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p0_ws, .uv = vec2_t(u0, v0), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p1_ws, .uv = vec2_t(u1, v1), .color = to_vec3(color_f32_t::WHITE) });
            }

            // bottom triangle 1
            {
                mesh_add_vertex_and_index(cone_mesh, { .pos = bot_ws, .uv = vec2_t(0.5f, 0.5f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p1_ws, .uv = vec2_t(u1, v1), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p0_ws, .uv = vec2_t(u0, v0), .color = to_vec3(color_f32_t::WHITE) });
            }

            // bottom triangle 2
            {
                mesh_add_vertex_and_index(cone_mesh, { .pos = bot_ws, .uv = vec2_t(0.5f, 0.5f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p1_ws, .uv = vec2_t(u1, v1), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cone_mesh, { .pos = p0_ws, .uv = vec2_t(u0, v0), .color = to_vec3(color_f32_t::WHITE) });
            }
        }

        s_primitive_shapes[(u32)primitive_shape_t::CONE] = cone_mesh;
    }

    // cylinder
    {
		mesh_t* cylinder_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		cylinder_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		cylinder_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		cylinder_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        vec3_t base_center = vec3_t::ZERO;
        vec3_t dir = vec3_t::WORLD_UP;
        f32 height = 1.0f;
        f32  base_radius = 1.0f;
        u32 resolution = 30;

        f32 theta_deg = 360.0f / (f32)resolution;
        f32 side_u_scale = 3.0f;

        vec3_t k = normalized(dir);
        mat44_t local_to_world = init_basis_from_k(k);
        scale(local_to_world, base_radius, base_radius, height);
        translate(local_to_world, base_center);

        for (u32 slice = 0; slice < resolution; slice++)
        {
            vec3_t top(0.0f, 0.0f, 1.0f);
            vec2_t p0_xy(cos_deg(theta_deg * slice), sin_deg(theta_deg * slice));
            vec2_t p1_xy(cos_deg(theta_deg * (slice + 1)), sin_deg(theta_deg * (slice + 1)));
            vec3_t bot(0.0f, 0.0f, 0.0f);

            vec3_t top_ws  = transform_point(local_to_world, top);
            vec3_t p0_top_ws = transform_point(local_to_world, init_vec3(p0_xy, 1.0f));
            vec3_t p1_top_ws = transform_point(local_to_world, init_vec3(p1_xy, 1.0f));
            vec3_t p0_bot_ws = transform_point(local_to_world, init_vec3(p0_xy, 0.0f));
            vec3_t p1_bot_ws = transform_point(local_to_world, init_vec3(p1_xy, 0.0f));
            vec3_t bot_ws   = transform_point(local_to_world, bot);

            // top triangle
            {
                f32 u0 = remap(p0_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
                f32 u1 = remap(p1_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
                f32 v0 = remap(p0_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
                f32 v1 = remap(p1_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

                mesh_add_vertex_and_index(cylinder_mesh, { .pos = top_ws, .uv = vec2_t(0.5, 0.5f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p0_top_ws, .uv = vec2_t(u0, v0), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p1_top_ws, .uv = vec2_t(u1, v1), .color = to_vec3(color_f32_t::WHITE) });
            }

            // side triangle 1
            {
                f32 u0 = (f32)slice / (f32)resolution;
                f32 u1 = (f32)(slice + 1) / (f32)resolution;

                u0 *= side_u_scale;
                u1 *= side_u_scale;

                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p0_bot_ws, .uv = vec2_t(u0, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p1_bot_ws, .uv = vec2_t(u1, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p1_top_ws, .uv = vec2_t(u1, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            }

            // side triangle 2
            {
                f32 u0 = (f32)slice / (f32)resolution;
                f32 u1 = (f32)(slice + 1) / (f32)resolution;

                u0 *= side_u_scale;
                u1 *= side_u_scale;

                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p0_bot_ws, .uv = vec2_t(u0, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p1_top_ws, .uv = vec2_t(u1, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p0_top_ws, .uv = vec2_t(u0, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
            }

            // bottom triangle
            {
                f32 u0 = remap(p0_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
                f32 u1 = remap(p1_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
                f32 v0 = remap(p0_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
                f32 v1 = remap(p1_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

                mesh_add_vertex_and_index(cylinder_mesh, { .pos = bot_ws, .uv = vec2_t(0.5f, 0.5f), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p1_bot_ws, .uv = vec2_t(u1, v1), .color = to_vec3(color_f32_t::WHITE) });
                mesh_add_vertex_and_index(cylinder_mesh, { .pos = p0_bot_ws, .uv = vec2_t(u0, v0), .color = to_vec3(color_f32_t::WHITE) });
            }
        }

        s_primitive_shapes[(u32)primitive_shape_t::CYLINDER] = cylinder_mesh;
    }

    // torus
    {
		mesh_t* torus_mesh = arena_alloc_struct(s_primitives_arena, mesh_t);
		torus_mesh->vertices = array_init<vertex_t>(s_primitives_arena, 64);
		torus_mesh->indices = array_init<u32>(s_primitives_arena, 64);
		torus_mesh->topology = primitive_topology_t::TRIANGLE_LIST;

        u32 resolution = 64;
        f32 u_scale = 3.0f;
        f32 v_scale = 8.0f;

        for (u32 torus_slice = 0; torus_slice <= resolution; torus_slice++)
        {
            f32 percent_around_torus = (f32)torus_slice / (f32)resolution;
            f32 cur_deg = percent_around_torus * 360.0f;
            vec3_t ring_anchor_pos = vec3_t(cos_deg(cur_deg), sin_deg(cur_deg), 0.0f);

            vec3_t i = normalized(ring_anchor_pos);
            vec3_t k = normalized(cross(vec3_t::WORLD_UP, i));
            vec3_t j = cross(i, k);
            mat44_t ring_world_transform = init_mat44(i, j, k, ring_anchor_pos);

            for (u32 torus_ring_pos = 0; torus_ring_pos <= resolution; torus_ring_pos++)
            {
                f32 percent_around_ring = (f32)torus_ring_pos / (f32)resolution;
                f32 deg = percent_around_ring * 360.0f;
                vec2_t ring_pos_ls = polar_to_cartesian_degs(deg, 0.3f);
                vec3_t ring_pos_ws = transform_point(ring_world_transform, init_vec3(ring_pos_ls, 0.0f));
                u32 vert_index = mesh_add_vertex(torus_mesh, { .pos = ring_pos_ws, 
                                                               .uv = vec2_t(percent_around_ring * u_scale, percent_around_torus * v_scale), 
                                                               .color = to_vec3(color_f32_t::WHITE) });

                if (torus_slice > 0 && torus_ring_pos > 0)
                {
                    u32 prev_vert_index = vert_index - 1;
                    u32 vert_index_last_ring = (vert_index - resolution) - 1;
                    u32 prev_vert_index_last_ring = vert_index_last_ring - 1;

                    mesh_add_triangle_indices(torus_mesh, vert_index, prev_vert_index_last_ring, prev_vert_index);
                    mesh_add_triangle_indices(torus_mesh, vert_index, vert_index_last_ring, prev_vert_index_last_ring);
                }
            }
        }

        s_primitive_shapes[(u32)primitive_shape_t::TORUS] = torus_mesh;
    }

	s_did_init_primitive_shapes = true;
}

const mesh_t* sm::primitive_shape_get_mesh(primitive_shape_t shape)
{
	SM_ASSERT(s_did_init_primitive_shapes);
	return s_primitive_shapes[(u32)shape];
}

mesh_t* sm::mesh_init(arena_t* arena, primitive_topology_t topology)
{
    mesh_t* mesh = arena_alloc_struct(arena, mesh_t);
    mesh->vertices = array_init<vertex_t>(arena, 64);
    mesh->indices = array_init<u32>(arena, 64);
    mesh->topology = topology;
    return mesh;
}

mesh_t* sm::mesh_init_from_obj(sm::arena_t* arena, const char* obj_filepath)
{
    mesh_t* mesh = mesh_init(arena);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::string full_filepath = std::string(MODELS_PATH) + obj_filepath;

	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, full_filepath.c_str());
	SM_ASSERT(loaded);

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			vertex_t vertex;

			vertex.pos = vec3_t(attrib.vertices[3 * index.vertex_index + 0],
								attrib.vertices[3 * index.vertex_index + 1],
								attrib.vertices[3 * index.vertex_index + 2]);

			vertex.uv = vec2_t(attrib.texcoords[2 * index.texcoord_index + 0],
							   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);

			vertex.color = to_vec3(color_f32_t::WHITE);

			mesh_add_vertex_and_index(mesh, vertex);
		}
	}

    return mesh;
}

u32 sm::mesh_add_vertex(mesh_t* mesh, const vertex_t& v)
{
    array_push(mesh->vertices, v);
    return (u32)mesh->vertices.cur_size - 1;
}

void sm::mesh_add_index(mesh_t* mesh, u32 index)
{
    array_push(mesh->indices, index);
}

void sm::mesh_add_vertex_and_index(mesh_t* mesh, const vertex_t& v)
{
    u32 index = mesh_add_vertex(mesh, v);
    mesh_add_index(mesh, index);
}

void sm::mesh_add_triangle_indices(mesh_t* mesh, u32 index0, u32 index1, u32 index2)
{
    mesh_add_index(mesh, index0);
    mesh_add_index(mesh, index1);
    mesh_add_index(mesh, index2);
}

void sm::mesh_add_quad_3d(mesh_t* mesh, const vec3_t& top_left, const vec3_t& top_right, const vec3_t& bottom_right, const vec3_t& bottom_left)
{
	u32 top_left_index = mesh_add_vertex(mesh, { .pos = top_left, .uv = vec2_t(0.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
	u32 top_right_index = mesh_add_vertex(mesh, { .pos = top_right, .uv = vec2_t(1.0f, 0.0f), .color = to_vec3(color_f32_t::WHITE) });
	u32 bottom_right_index = mesh_add_vertex(mesh, { .pos = bottom_right, .uv = vec2_t(1.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });
	u32 bottom_left_index = mesh_add_vertex(mesh, { .pos = bottom_left, .uv = vec2_t(0.0f, 1.0f), .color = to_vec3(color_f32_t::WHITE) });

	mesh_add_triangle_indices(mesh, top_left_index, bottom_right_index, top_right_index);
	mesh_add_triangle_indices(mesh, top_left_index, bottom_left_index, bottom_right_index);
}

void sm::mesh_add_quad_3d(mesh_t* mesh, const vec3_t& centerPos, const vec3_t& right, const vec3_t& up, f32 half_width, f32 half_height, u32 resolution)
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
			u32 vert_index = mesh_add_vertex(mesh, { .pos = vert_pos, .uv = vec2_t(u, v), .color = to_vec3(color_f32_t::WHITE) });

			if (y > 0 && x > 0)
			{
				u32 prevVertIndex = vert_index - 1;
				u32 vertIndexLastRow = (vert_index - resolution) - 1;
				u32 prevVertIndexLastRow = vertIndexLastRow - 1;
				mesh_add_triangle_indices(mesh, vert_index, prevVertIndexLastRow, prevVertIndex);
				mesh_add_triangle_indices(mesh, vert_index, vertIndexLastRow, prevVertIndexLastRow);
			}
		}
	}
}

size_t sm::mesh_calc_vertex_buffer_size(const mesh_t* mesh)
{
    return mesh->vertices.cur_size * sizeof(vertex_t);
}

size_t sm::mesh_calc_index_buffer_size(const mesh_t* mesh)
{
    return mesh->indices.cur_size * sizeof(u32);
}
