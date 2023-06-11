#include "engine/core/color.h"
#include "engine/math/mat44.h"
#include "engine/math/math_utils.h"
#include "engine/math/vec3.h"
#include "engine/render/mesh.h"
#include "engine/render/vertex.h"
#include "engine/render/vulkan/vulkan_renderer.h"
#include "engine/core/assert.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "engine/thirdparty/tinyobjloader/tiny_obj_loader.h"

static
u32 mesh_add_vertex(mesh_t& mesh, const vec3& pos, const vec4& color, const vec2& uv)
{
    mesh.vertices.push_back(make_vertex(pos, color, uv));
    return mesh.vertices.size() - 1;
}

static
void mesh_add_triangle(mesh_t& mesh, u32 v0, u32 v1, u32 v2)
{
    mesh.indices.push_back(v0);
    mesh.indices.push_back(v1);
    mesh.indices.push_back(v2);
}

static
void mesh_add_triangle_from_last_3_verts(mesh_t& mesh)
{
    SM_ASSERT_MSG(mesh.vertices.size() >= 3, "Trying to build triangle in mesh with less than 3 vertexes.\n");
    size_t size = mesh.vertices.size();
    mesh_add_triangle(mesh, size - 3, size - 2, size - 1);
}

size_t mesh_calc_vertex_buffer_size(mesh_t* mesh)
{
    SM_ASSERT(nullptr != mesh);
	return sizeof(mesh->vertices[0]) * mesh->vertices.size();
}

size_t mesh_calc_index_buffer_size(mesh_t* mesh)
{
    SM_ASSERT(nullptr != mesh);
	return sizeof(mesh->indices[0]) * mesh->indices.size();
}

mesh_t* mesh_load_from_obj(const char* obj_filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_filepath);
	SM_ASSERT(loaded);

    mesh_t* mesh = new mesh_t;

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			vertex_pct_t vertex = {};

			vertex.m_pos = make_vec3(attrib.vertices[3 * index.vertex_index + 0],
                                     attrib.vertices[3 * index.vertex_index + 1],
                                     attrib.vertices[3 * index.vertex_index + 2]);

			vertex.m_uv = make_vec2(attrib.texcoords[2 * index.texcoord_index + 0],
                                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);

			vertex.m_color = make_vec4(1.0f, 1.0f, 1.0f, 1.0f);

			mesh->vertices.push_back(vertex);
			mesh->indices.push_back((u32)mesh->indices.size());
		}
	}

    mesh->topology = PrimitiveTopology::kTriangleList;

	return mesh;
}

void mesh_build_quad_3d(mesh_t& mesh, const vec3& center_pos, const vec3& right, const vec3& up, f32 half_width, f32 half_height, u32 resolution)
{
    vec3 right_norm = get_normalized(right);
    vec3 up_norm = get_normalized(up);

    f32 width_step = (half_width * 2.0f) / (f32)resolution;
    f32 height_step = (half_height * 2.0f) / (f32)resolution;

    vec3 top_left = center_pos + (-right_norm * half_width) + (up_norm * half_height);

    for(i32 y = 0; y <= resolution; y++)
    {
        for(i32 x = 0; x <= resolution; x++)
        {
            f32 u = (f32)x / (f32)(resolution);
            f32 v = (f32)y / (f32)(resolution);
            vec3 vert_pos = top_left + (right * width_step * x) + (-up * height_step * y);
            u32 vert_index = mesh_add_vertex(mesh, vert_pos, color_get(Color::kWhite), make_vec2(u, v));

            if(y > 0 && x > 0)
            {
                u32 prev_vert_index = vert_index - 1;    
                u32 vert_index_last_row = (vert_index - resolution) - 1;
                u32 prev_vert_index_last_row = vert_index_last_row - 1;
                mesh_add_triangle(mesh, vert_index, prev_vert_index_last_row, prev_vert_index);
                mesh_add_triangle(mesh, vert_index, vert_index_last_row, prev_vert_index_last_row);
            }
        }
    }
}

void mesh_build_quad_3d(mesh_t& mesh, const vec3& top_left, const vec3& top_right, const vec3& bottom_right, const vec3& bottom_left)
{
    u32 top_left_index = mesh_add_vertex(mesh, top_left, color_get(Color::kWhite), make_vec2(0.0f, 0.0f));
    u32 top_right_index = mesh_add_vertex(mesh, top_right, color_get(Color::kWhite), make_vec2(1.0f, 0.0f));
    u32 bottom_right_index = mesh_add_vertex(mesh, bottom_right, color_get(Color::kWhite), make_vec2(1.0f, 1.0f));
    u32 bottom_left_index = mesh_add_vertex(mesh, bottom_left, color_get(Color::kWhite), make_vec2(0.0f, 1.0f));

    mesh_add_triangle(mesh, top_left_index, bottom_right_index, top_right_index);
    mesh_add_triangle(mesh, top_left_index, bottom_left_index, bottom_right_index);
}

void mesh_build_axes_lines_3d(mesh_t& mesh, const vec3& origin, const vec3& i, const vec3& j, const vec3& k)
{
    SM_ASSERT(mesh.topology == PrimitiveTopology::kLineList);
    vec2 uv = make_vec2(0.0f, 0.0f);
    vec4 red = color_get(Color::kRed);
    vec4 green = color_get(Color::kGreen);
    vec4 blue = color_get(Color::kBlue);

    mesh.indices.push_back(mesh_add_vertex(mesh, origin, red, uv));
    mesh.indices.push_back(mesh_add_vertex(mesh, i, red, uv));
    mesh.indices.push_back(mesh_add_vertex(mesh, origin, green, uv));
    mesh.indices.push_back(mesh_add_vertex(mesh, j, green, uv));
    mesh.indices.push_back(mesh_add_vertex(mesh, origin, blue, uv));
    mesh.indices.push_back(mesh_add_vertex(mesh, k, blue, uv));
}

void mesh_build_uv_sphere(mesh_t& mesh, const vec3& origin, f32 radius, u32 resolution)
{
    vec4 white_color = color_get(Color::kWhite);

    f32 u_delta_deg = 360.0f / (f32)resolution; 
    f32 v_delta_deg = 180.0f / (f32)resolution; 

    for(u32 v_slice = 0; v_slice <= resolution; v_slice++)
    {
        for(u32 u_slice = 0; u_slice <= resolution; u_slice++)
        {
            f32 u = (f32)u_slice / (f32)resolution;
            f32 v = (f32)v_slice / (f32)resolution;
            vec2 uv = make_vec2(u, v);
            
            f32 y_deg = -90.0f + ((f32)v_slice * v_delta_deg);
            f32 z_deg = (f32)u_slice * u_delta_deg;

            vec4 pos = make_vec4(radius, 0.0f, 0.0f, 0.0f) * make_rotation_y_deg(y_deg) * make_rotation_z_deg(z_deg);
            vec3 final_pos = origin + pos.xyz;

            u32 vert_index = mesh_add_vertex(mesh, final_pos, white_color, uv);

            if(v_slice == 0)
            {
                continue;
            }

            // normal triangle add
            if(u_slice > 0)
            {
                u32 prev_vert_index = vert_index - 1;                
                u32 vert_index_last_slice = (vert_index - resolution) - 1;
				u32 prev_vert_index_last_slice = vert_index_last_slice  - 1;

                mesh_add_triangle(mesh, vert_index, vert_index_last_slice, prev_vert_index_last_slice);
                mesh_add_triangle(mesh, vert_index, prev_vert_index_last_slice, prev_vert_index);
            }
        }
    }
}

void mesh_build_cube(mesh_t& mesh, const vec3& center, f32 radius, u32 resolution)
{
    mesh_build_quad_3d(mesh, center + VEC3_FORWARD * radius, VEC3_LEFT, VEC3_UP, radius, radius, resolution);
    mesh_build_quad_3d(mesh, center + VEC3_LEFT * radius, VEC3_BACKWARD, VEC3_UP, radius, radius, resolution);
    mesh_build_quad_3d(mesh, center + VEC3_BACKWARD * radius, VEC3_RIGHT, VEC3_UP, radius, radius, resolution);
    mesh_build_quad_3d(mesh, center + VEC3_RIGHT * radius, VEC3_FORWARD, VEC3_UP, radius, radius, resolution);
    mesh_build_quad_3d(mesh, center + VEC3_UP * radius, VEC3_RIGHT, VEC3_FORWARD, radius, radius, resolution);
    mesh_build_quad_3d(mesh, center + VEC3_DOWN * radius, VEC3_LEFT, VEC3_FORWARD, radius, radius, resolution);
}

void mesh_build_cylinder(mesh_t& mesh, const vec3& base_center, const vec3& dir, f32 height, f32 base_radius, f32 resolution)
{
    f32 theta_deg = 360.0f / (f32)resolution; 
    f32 side_u_scale = 3.0f;

    vec4 white = color_get(Color::kWhite);

    vec3 k = get_normalized(dir);
    mat44 local_to_world_transform = make_mat44_basis_from_k(k);
    scale_local(local_to_world_transform, base_radius, base_radius, height);

    for(u32 slice = 0; slice < resolution; slice++)
    {
        vec3 top = make_vec3(0.0f, 0.0f, 1.0f);
        vec2 p0_xy = make_vec2(cos_deg(theta_deg * slice), sin_deg(theta_deg * slice));
        vec2 p1_xy = make_vec2(cos_deg(theta_deg * (slice + 1)), sin_deg(theta_deg * (slice + 1)));
        vec3 bot = make_vec3(0.0f, 0.0f, 0.0f);

        vec3 top_ws = transform_point(local_to_world_transform, top);
        vec3 p0_top_ws = transform_point(local_to_world_transform, make_vec3(p0_xy, 1.0f));
        vec3 p1_top_ws = transform_point(local_to_world_transform, make_vec3(p1_xy, 1.0f));
        vec3 p0_bot_ws = transform_point(local_to_world_transform, make_vec3(p0_xy, 0.0f));
        vec3 p1_bot_ws = transform_point(local_to_world_transform, make_vec3(p1_xy, 0.0f));
        vec3 bot_ws = transform_point(local_to_world_transform, bot);

        // top triangle
        {
            f32 u0 = remap(p0_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 u1 = remap(p1_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v0 = remap(p0_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v1 = remap(p1_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

            mesh_add_vertex(mesh, top_ws, white, make_vec2(0.5, 0.5f));
            mesh_add_vertex(mesh, p0_top_ws, white, make_vec2(u0, v0));
            mesh_add_vertex(mesh, p1_top_ws, white, make_vec2(u1, v1));
            mesh_add_triangle_from_last_3_verts(mesh);
        }

        // side triangle 1
        {
            f32 u0 = (f32)slice / (f32)resolution;
            f32 u1 = (f32)(slice + 1) / (f32)resolution;
            
            u0 *= side_u_scale;
            u1 *= side_u_scale;

            mesh_add_vertex(mesh, p0_bot_ws, white, make_vec2(u0, 1.0f));
            mesh_add_vertex(mesh, p1_bot_ws, white, make_vec2(u1, 1.0f));
            mesh_add_vertex(mesh, p1_top_ws, white, make_vec2(u1, 0.0f));
            mesh_add_triangle_from_last_3_verts(mesh);
        }

        // side triangle 2
        {
            f32 u0 = (f32)slice / (f32)resolution;
            f32 u1 = (f32)(slice + 1) / (f32)resolution;

            u0 *= side_u_scale;
            u1 *= side_u_scale;

            mesh_add_vertex(mesh, p0_bot_ws, white, make_vec2(u0, 1.0f));
            mesh_add_vertex(mesh, p1_top_ws, white, make_vec2(u1, 0.0f));
            mesh_add_vertex(mesh, p0_top_ws, white, make_vec2(u0, 0.0f));
            mesh_add_triangle_from_last_3_verts(mesh);
        }

        // bottom triangle
        {
            f32 u0 = remap(p0_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 u1 = remap(p1_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v0 = remap(p0_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v1 = remap(p1_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

            mesh_add_vertex(mesh, bot_ws, white, make_vec2(0.5f, 0.5f));
            mesh_add_vertex(mesh, p1_bot_ws, white, make_vec2(u1, v1));
            mesh_add_vertex(mesh, p0_bot_ws, white, make_vec2(u0, v0));
            mesh_add_triangle_from_last_3_verts(mesh);
        }
    }
}

void mesh_build_cone(mesh_t& mesh, const vec3& base_center, const vec3& dir, f32 height, f32 base_radius, f32 resolution)
{
    vec3 k = get_normalized(dir) * height;
    vec3 i = get_normalized(cross(k, VEC3_UP)) * base_radius;
    vec3 j = get_normalized(cross(k, i)) * base_radius;
    mat44 local_to_world_transform = make_mat44(i, j, k, base_center);

    f32 theta_deg = 360.0f / (f32)resolution; 

    vec4 white = color_get(Color::kWhite);

    vec3 top_local = make_vec3(0.0f, 0.0f, 1.0f);
    vec3 bot_local = make_vec3(0.0f, 0.0f, 0.0f);
    vec3 top_ws = transform_point(local_to_world_transform, top_local);
    vec3 bot_ws = transform_point(local_to_world_transform, bot_local);

    for(u32 slice = 0; slice <= resolution; slice++)
    {
        vec3 p0 = make_vec3(cos_deg(theta_deg * slice), sin_deg(theta_deg * slice), 0.0f);
        vec3 p1 = make_vec3(cos_deg(theta_deg * (slice + 1)), sin_deg(theta_deg * (slice + 1)), 0.0f);

        vec3 p0_ws = transform_point(local_to_world_transform, p0);
        vec3 p1_ws = transform_point(local_to_world_transform, p1);

        f32 u0 = remap(p0.x, -1.0f, 1.0f, 0.0f, 1.0f);
        f32 u1 = remap(p1.x, -1.0f, 1.0f, 0.0f, 1.0f);

        f32 v0 = remap(p0.y, -1.0f, 1.0f, 0.0f, 1.0f);
        f32 v1 = remap(p1.y, -1.0f, 1.0f, 0.0f, 1.0f);

        // top triangle 1
        {
            mesh_add_vertex(mesh, top_ws, white, make_vec2(0.5, 0.5f));
            mesh_add_vertex(mesh, p0_ws, white, make_vec2(u0, v0));
            mesh_add_vertex(mesh, p1_ws, white, make_vec2(u1, v1));
            mesh_add_triangle_from_last_3_verts(mesh);
        }

        // top triangle 2
        {
            mesh_add_vertex(mesh, top_ws, white, make_vec2(0.5f, 0.5));
            mesh_add_vertex(mesh, p0_ws, white, make_vec2(u0, v0));
            mesh_add_vertex(mesh, p1_ws, white, make_vec2(u1, v1));
            mesh_add_triangle_from_last_3_verts(mesh);
        }

        // bottom triangle 1
        {
            mesh_add_vertex(mesh, bot_ws, white, make_vec2(0.5f, 0.5f));
            mesh_add_vertex(mesh, p1_ws, white, make_vec2(u1, v1));
            mesh_add_vertex(mesh, p0_ws, white, make_vec2(u0, v0));
            mesh_add_triangle_from_last_3_verts(mesh);
        }

        // bottom triangle 2
        {
            mesh_add_vertex(mesh, bot_ws, white, make_vec2(0.5f, 0.5f));
            mesh_add_vertex(mesh, p1_ws, white, make_vec2(u1, v1));
            mesh_add_vertex(mesh, p0_ws, white, make_vec2(u0, v0));
            mesh_add_triangle_from_last_3_verts(mesh);
        }
    }
}

void mesh_build_frustum(mesh_t& mesh, const mat44& view_projection)
{
    mat44 inverse_vp = get_inversed(view_projection);
    
    vec4 near_top_left      = make_vec4(-1.0f, -1.0f, 0.0f, 1.0f) * inverse_vp;
    vec4 near_top_right     = make_vec4(1.0f, -1.0f, 0.0f, 1.0f) * inverse_vp;
    vec4 near_bottom_right  = make_vec4(1.0f, 1.0f, 0.0f, 1.0f) * inverse_vp;
    vec4 near_bottom_left   = make_vec4(-1.0f, 1.0f, 0.0f, 1.0f) * inverse_vp;
    vec4 far_top_left       = make_vec4(-1.0f, -1.0f, 1.0f, 1.0f) * inverse_vp;
    vec4 far_top_right      = make_vec4(1.0f, -1.0f, 1.0f, 1.0f) * inverse_vp;
    vec4 far_bottom_right   = make_vec4(1.0f, 1.0f, 1.0f, 1.0f) * inverse_vp;
    vec4 far_bottom_left    = make_vec4(-1.0f, 1.0f, 1.0f, 1.0f) * inverse_vp;

    near_top_left /= near_top_left.w;
    near_top_right /= near_top_right.w;
    near_bottom_left /= near_bottom_left.w;
    near_bottom_right /= near_bottom_right.w;
    far_top_left /= far_top_left.w;
    far_top_right /= far_top_right.w; 
    far_bottom_right /= far_bottom_right.w;
    far_bottom_left /= far_bottom_left.w;

    //near plane
    mesh_build_quad_3d(mesh, near_top_left.xyz, near_top_right.xyz, near_bottom_right.xyz, near_bottom_left.xyz);

    // right plane
    mesh_build_quad_3d(mesh, near_top_right.xyz, far_top_right.xyz, far_bottom_right.xyz, near_bottom_right.xyz);

    // top plane
    mesh_build_quad_3d(mesh, near_top_left.xyz, far_top_left.xyz, far_top_right.xyz, near_top_right.xyz);

    // left plane
    mesh_build_quad_3d(mesh, near_bottom_left.xyz, far_bottom_left.xyz, far_top_left.xyz, near_top_left.xyz);
    
    // bottom plane
    mesh_build_quad_3d(mesh, near_bottom_right.xyz, far_bottom_right.xyz, far_bottom_left.xyz, near_bottom_left.xyz);
    
    // far plane
    mesh_build_quad_3d(mesh, far_top_right.xyz, far_top_left.xyz, far_bottom_left.xyz, far_bottom_right.xyz);
}

mesh_t* mesh_load_unit_axes()
{
    mesh_t* mesh = new mesh_t;
    mesh->topology = PrimitiveTopology::kLineList;
    mesh_build_axes_lines_3d(*mesh, VEC3_ZERO, VEC3_FORWARD, VEC3_LEFT, VEC3_UP);
    return mesh;
}

mesh_t* mesh_load_unit_tetrahedron()
{
    mesh_t* mesh = new mesh_t;

    vec3 v0_pos = make_vec3(0.0f, 0.0f, 1.0f);

    f32 pitch = 0.0f;
    vec3 v1_pos = (make_vec4(1.0f, 0.0f, 0.0f, 0.0f) * make_rotation_y_deg(pitch)).xyz;
    vec3 v2_pos = (make_vec4(1.0f, 0.0f, 0.0f, 0.0f) * make_rotation_y_deg(pitch) * make_rotation_z_deg(120.0f)).xyz;
    vec3 v3_pos = (make_vec4(1.0f, 0.0f, 0.0f, 0.0f) * make_rotation_y_deg(pitch) * make_rotation_z_deg(240.0f)).xyz;

    normalize(v1_pos);
    normalize(v2_pos);
    normalize(v3_pos);

    vec4 white_color = make_vec4(1.0f, 1.0f, 1.0f, 1.0f);

    u32 v0_index = mesh_add_vertex(*mesh, v0_pos, white_color, make_vec2(0.0f, 0.0f));
    u32 v1_index = mesh_add_vertex(*mesh, v1_pos, white_color, make_vec2(0.0f, 1.0f));
    u32 v2_index = mesh_add_vertex(*mesh, v2_pos, white_color, make_vec2(1.0f, 0.0f));

    u32 v3_index = mesh_add_vertex(*mesh, v0_pos, white_color, make_vec2(0.0f, 0.0f));
    u32 v4_index = mesh_add_vertex(*mesh, v2_pos, white_color, make_vec2(0.0f, 1.0f));
    u32 v5_index = mesh_add_vertex(*mesh, v3_pos, white_color, make_vec2(1.0f, 0.0f));

    u32 v6_index = mesh_add_vertex(*mesh, v0_pos, white_color, make_vec2(0.0f, 0.0f));
    u32 v7_index = mesh_add_vertex(*mesh, v3_pos, white_color, make_vec2(0.0f, 1.0f));
    u32 v8_index = mesh_add_vertex(*mesh, v1_pos, white_color, make_vec2(1.0f, 0.0f));

    u32 v9_index = mesh_add_vertex(*mesh, v1_pos, white_color, make_vec2(0.0f, 0.0f));
    u32 v10_index = mesh_add_vertex(*mesh, v3_pos, white_color, make_vec2(0.0f, 1.0f));
    u32 v11_index = mesh_add_vertex(*mesh, v2_pos, white_color, make_vec2(1.0f, 0.0f));

    mesh_add_triangle(*mesh, v0_index, v1_index, v2_index);
    mesh_add_triangle(*mesh, v3_index, v4_index, v5_index);
    mesh_add_triangle(*mesh, v6_index, v7_index, v8_index);
    mesh_add_triangle(*mesh, v9_index, v10_index, v11_index);

    mesh->topology = PrimitiveTopology::kTriangleList;

    return mesh;
}

mesh_t* mesh_load_unit_cube()
{
    mesh_t* mesh = new mesh_t;
    mesh->topology = PrimitiveTopology::kTriangleList;
    mesh_build_cube(*mesh, VEC3_ZERO, 1.0f);
    return mesh;
}

mesh_t* mesh_load_unit_octahedron()
{
    mesh_t* mesh = new mesh_t;

    vec3 v0_pos = make_vec3(0.0f, 0.0f, 1.0f);
    vec3 v1_pos = make_vec3(1.0f, 0.0f, 0.0f);
    vec3 v2_pos = make_vec3(0.0f, 1.0f, 0.0f);
    vec3 v3_pos = make_vec3(-1.0f, 0.0f, 0.0f);
    vec3 v4_pos = make_vec3(0.0f, -1.0f, 0.0f);
    vec3 v5_pos = make_vec3(0.0f, 0.0f, -1.0f);

    vec4 white_color = make_vec4(1.0f, 1.0f, 1.0f, 1.0f);

    {
        u32 i0 = mesh_add_vertex(*mesh, v0_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v1_pos, white_color, make_vec2(0.0f, 1.0f));
        u32 i2 = mesh_add_vertex(*mesh, v2_pos, white_color, make_vec2(1.0f, 0.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }

    {
        u32 i0 = mesh_add_vertex(*mesh, v0_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v2_pos, white_color, make_vec2(0.0f, 1.0f));
        u32 i2 = mesh_add_vertex(*mesh, v3_pos, white_color, make_vec2(1.0f, 0.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }

    {
        u32 i0 = mesh_add_vertex(*mesh, v0_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v3_pos, white_color, make_vec2(0.0f, 1.0f));
        u32 i2 = mesh_add_vertex(*mesh, v4_pos, white_color, make_vec2(1.0f, 0.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }
    
    {
        u32 i0 = mesh_add_vertex(*mesh, v0_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v4_pos, white_color, make_vec2(0.0f, 1.0f));
        u32 i2 = mesh_add_vertex(*mesh, v1_pos, white_color, make_vec2(1.0f, 0.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }

    {
        u32 i0 = mesh_add_vertex(*mesh, v5_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v2_pos, white_color, make_vec2(1.0f, 0.0f));
        u32 i2 = mesh_add_vertex(*mesh, v1_pos, white_color, make_vec2(0.0f, 1.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }

    {
        u32 i0 = mesh_add_vertex(*mesh, v5_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v3_pos, white_color, make_vec2(1.0f, 0.0f));
        u32 i2 = mesh_add_vertex(*mesh, v2_pos, white_color, make_vec2(0.0f, 1.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }

    {
        u32 i0 = mesh_add_vertex(*mesh, v5_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v4_pos, white_color, make_vec2(1.0f, 0.0f));
        u32 i2 = mesh_add_vertex(*mesh, v3_pos, white_color, make_vec2(0.0f, 1.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }
    
    {
        u32 i0 = mesh_add_vertex(*mesh, v5_pos, white_color, make_vec2(0.0f, 0.0f));
        u32 i1 = mesh_add_vertex(*mesh, v1_pos, white_color, make_vec2(1.0f, 0.0f));
        u32 i2 = mesh_add_vertex(*mesh, v4_pos, white_color, make_vec2(0.0f, 1.0f));
        mesh_add_triangle(*mesh, i0, i1, i2); 
    }

    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

mesh_t* mesh_load_unit_uv_sphere()
{
    mesh_t* mesh = new mesh_t;
    mesh_build_uv_sphere(*mesh, VEC3_ZERO, 1.0f, 64);
    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

mesh_t* mesh_load_unit_plane()
{
    mesh_t* mesh = new mesh_t;
    mesh_build_quad_3d(*mesh, VEC3_ZERO, VEC3_RIGHT, VEC3_FORWARD, 1.0f, 1.0f, 1);
    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

mesh_t* mesh_load_unit_cone()
{
    mesh_t* mesh = new mesh_t;

    u32 resolution = 64;
    f32 theta_deg = 360.0f / (f32)resolution; 

    vec4 white = color_get(Color::kWhite);

    for(u32 slice = 0; slice <= resolution; slice++)
    {
        vec3 top = make_vec3(0.0f, 0.0f, 1.0f);
        vec3 p0 = make_vec3(cos_deg(theta_deg * slice), sin_deg(theta_deg * slice), 0.0f);
        vec3 p1 = make_vec3(cos_deg(theta_deg * (slice + 1)), sin_deg(theta_deg * (slice + 1)), 0.0f);
        vec3 bot = make_vec3(0.0f, 0.0f, 0.0f);

        f32 u0 = remap(p0.x, -1.0f, 1.0f, 0.0f, 1.0f);
        f32 u1 = remap(p1.x, -1.0f, 1.0f, 0.0f, 1.0f);

        f32 v0 = remap(p0.y, -1.0f, 1.0f, 0.0f, 1.0f);
        f32 v1 = remap(p1.y, -1.0f, 1.0f, 0.0f, 1.0f);

        // top triangle 1
        {
            mesh_add_vertex(*mesh, top, white, make_vec2(0.5, 0.5f));
            mesh_add_vertex(*mesh, p0, white, make_vec2(u0, v0));
            mesh_add_vertex(*mesh, p1, white, make_vec2(u1, v1));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }

        // top triangle 2
        {
            mesh_add_vertex(*mesh, top, white, make_vec2(0.5f, 0.5));
            mesh_add_vertex(*mesh, p0, white, make_vec2(u0, v0));
            mesh_add_vertex(*mesh, p1, white, make_vec2(u1, v1));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }

        // bottom triangle 1
        {
            mesh_add_vertex(*mesh, bot, white, make_vec2(0.5f, 0.5f));
            mesh_add_vertex(*mesh, p1, white, make_vec2(u1, v1));
            mesh_add_vertex(*mesh, p0, white, make_vec2(u0, v0));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }

        // bottom triangle 2
        {
            mesh_add_vertex(*mesh, bot, white, make_vec2(0.5f, 0.5f));
            mesh_add_vertex(*mesh, p1, white, make_vec2(u1, v1));
            mesh_add_vertex(*mesh, p0, white, make_vec2(u0, v0));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }
    }

    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

mesh_t* mesh_load_unit_cylinder()
{
    mesh_t* mesh = new mesh_t;
    mesh->topology = PrimitiveTopology::kTriangleList;
    mesh_build_cylinder(*mesh, VEC3_ZERO, VEC3_UP, 1.0f, 1.0f, 32);
    return mesh;
}

mesh_t* mesh_load_unit_torus()
{
    mesh_t* mesh = new mesh_t;

    u32 resolution = 128;
    f32 u_scale = 3.0f;
    f32 v_scale = 8.0f;

    for(u32 torus_slice = 0; torus_slice <= resolution; torus_slice++)
    {
        f32 percent_around_torus = (f32)torus_slice / (f32)resolution; 
        f32 cur_deg = percent_around_torus * 360.0f;
        vec3 ring_anchor_pos = make_vec3(cos_deg(cur_deg), sin_deg(cur_deg), 0.0f);

        vec3 i = get_normalized(ring_anchor_pos);
        vec3 k = get_normalized(cross(VEC3_UP, i));
        vec3 j = cross(i, k);
        mat44 ring_world_transform = make_mat44(i, j, k, ring_anchor_pos);

        for(u32 torus_ring_pos = 0; torus_ring_pos <= resolution; torus_ring_pos++)
        {
            f32 percent_around_ring = (f32)torus_ring_pos / (f32) resolution;
            f32 deg = percent_around_ring * 360.0f;
            vec2 ring_pos_ls = calc_2d_polar_to_xy_deg(deg, 0.3f);
            vec3 ring_pos_ws = transform_point(ring_world_transform, make_vec3(ring_pos_ls, 0.0f));
            u32 vert_index = mesh_add_vertex(*mesh, ring_pos_ws, color_get(Color::kWhite), make_vec2(percent_around_ring * u_scale, percent_around_torus * v_scale));

            if(torus_slice > 0 && torus_ring_pos > 0)
            {
                u32 prev_vert_index = vert_index - 1; 
                u32 vert_index_last_ring = (vert_index - resolution) - 1;
                u32 prev_vert_index_last_ring = vert_index_last_ring - 1;

                mesh_add_triangle(*mesh, vert_index, prev_vert_index_last_ring, prev_vert_index);
                mesh_add_triangle(*mesh, vert_index, vert_index_last_ring, prev_vert_index_last_ring);
            }
        }
    }

    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

//mesh_t* mesh_load_xxxx()
//{
//    mesh_t* mesh = new mesh_t;
//
//
//    mesh->topology = PrimitiveTopology::kTriangleList;
//    return mesh;
//}
