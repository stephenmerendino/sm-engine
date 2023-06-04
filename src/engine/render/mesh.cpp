#include "engine/core/color.h"
#include "engine/math/math_utils.h"
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

mesh_t* mesh_load_axes()
{
    mesh_t* mesh = new mesh_t;

	// X
	vertex_pct_t xOrigin{};
	xOrigin.m_pos = make_vec3(0.0f, 0.0f, 0.0f);
	xOrigin.m_uv = make_vec2(0.0f, 0.0f);
	xOrigin.m_color = make_vec4(1.0f, 0.0f, 0.0f, 1.0f);

	vertex_pct_t xAxis{};
	xAxis.m_pos = make_vec3(1.0f, 0.0f, 0.0f);
	xAxis.m_uv = make_vec2(0.0f, 0.0f);
	xAxis.m_color = make_vec4(1.0f, 0.0f, 0.0f, 1.0f);

	// Y
	vertex_pct_t yOrigin{};
	yOrigin.m_pos = make_vec3(0.0f, 0.0f, 0.0f);
	yOrigin.m_uv = make_vec2(0.0f, 0.0f);
	yOrigin.m_color = make_vec4(0.0f, 1.0f, 0.0f, 1.0f);

	vertex_pct_t yAxis{};
	yAxis.m_pos = make_vec3(0.0f, 1.0f, 0.0f);
	yAxis.m_uv = make_vec2(0.0f, 0.0f);
	yAxis.m_color = make_vec4(0.0f, 1.0f, 0.0f, 1.0f);

	// Z
	vertex_pct_t zOrigin{};
	zOrigin.m_pos = make_vec3(0.0f, 0.0f, 0.0f);
	zOrigin.m_uv = make_vec2(0.0f, 0.0f);
	zOrigin.m_color = make_vec4(0.0f, 0.0f, 1.0f, 1.0f);

	vertex_pct_t zAxis{};
	zAxis.m_pos = make_vec3(0.0f, 0.0f, 1.0f);
	zAxis.m_uv = make_vec2(0.0f, 0.0f);
	zAxis.m_color = make_vec4(0.0f, 0.0f, 1.0f, 1.0f);

	mesh->vertices.push_back(xOrigin);
	mesh->vertices.push_back(xAxis);
	mesh->vertices.push_back(yOrigin);
	mesh->vertices.push_back(yAxis);
	mesh->vertices.push_back(zOrigin);
	mesh->vertices.push_back(zAxis);	

	mesh->indices.push_back(0);
	mesh->indices.push_back(1);
	mesh->indices.push_back(2);
	mesh->indices.push_back(3);
	mesh->indices.push_back(4);
	mesh->indices.push_back(5);

    mesh->topology = PrimitiveTopology::kLineList;

    return mesh;
}

mesh_t* mesh_load_tetrahedron()
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

mesh_t* mesh_load_cube()
{
	const f32 kSize = 1.0f;
	const vec4 white = make_vec4(1.0f, 1.0f, 1.0f, 1.0f);

    mesh_t* mesh = new mesh_t;

	// x axis face
	{
		// Top left
		vertex_pct_t v1{};

		v1.m_pos = make_vec3(1.0f, -1.0f, 1.0f) * kSize;
		v1.m_uv = make_vec2(0.0f, 0.0f);
		v1.m_color = white;

		// Bottom left
		vertex_pct_t v2{};

		v2.m_pos = make_vec3(1.0f, -1.0f, -1.0f) * kSize;
		v2.m_uv = make_vec2(0.0f, 1.0f);
		v2.m_color = white;

		// Bottom right
		vertex_pct_t v3{};

		v3.m_pos = make_vec3(1.0f, 1.0f, -1.0f) * kSize;
		v3.m_uv = make_vec2(1.0f, 1.0f);
		v3.m_color = white;

		// Top right
		vertex_pct_t v4{};

		v4.m_pos = make_vec3(1.0f, 1.0f, 1.0f) * kSize;
		v4.m_uv = make_vec2(1.0f, 0.0f);
		v4.m_color = white;

		mesh->vertices.push_back(v1);
		mesh->vertices.push_back(v2);
		mesh->vertices.push_back(v3);
		mesh->vertices.push_back(v4);
		mesh->indices.push_back(0);
		mesh->indices.push_back(1);
		mesh->indices.push_back(3);
		mesh->indices.push_back(3);
		mesh->indices.push_back(1);
		mesh->indices.push_back(2);
	}
	
	// neg x axis face
	{
		// Top left
		vertex_pct_t v1{};

		v1.m_pos = make_vec3(-1.0f, 1.0f, 1.0f) * kSize;
		v1.m_uv = make_vec2(0.0f, 0.0f);
		v1.m_color = white;

		// Bottom left
		vertex_pct_t v2{};

		v2.m_pos = make_vec3(-1.0f, 1.0f, -1.0f) * kSize;
		v2.m_uv = make_vec2(0.0f, 1.0f);
		v2.m_color = white;

		// Bottom right
		vertex_pct_t v3{};

		v3.m_pos = make_vec3(-1.0f, -1.0f, -1.0f) * kSize;
		v3.m_uv = make_vec2(1.0f, 1.0f);
		v3.m_color = white;

		// Top right
		vertex_pct_t v4{};

		v4.m_pos = make_vec3(-1.0f, -1.0f, 1.0f) * kSize;
		v4.m_uv = make_vec2(1.0f, 0.0f);
		v4.m_color = white;

		mesh->vertices.push_back(v1);
		mesh->vertices.push_back(v2);
		mesh->vertices.push_back(v3);
		mesh->vertices.push_back(v4);
		mesh->indices.push_back(4);
		mesh->indices.push_back(5);
		mesh->indices.push_back(7);
		mesh->indices.push_back(7);
		mesh->indices.push_back(5);
		mesh->indices.push_back(6);
	}

	// y axis face
	{
		// Top left
		vertex_pct_t v1{};

		v1.m_pos = make_vec3(1.0f, 1.0f, 1.0f) * kSize;
		v1.m_uv = make_vec2(0.0f, 0.0f);
		v1.m_color = white;

		// Bottom left
		vertex_pct_t v2{};

		v2.m_pos = make_vec3(1.0f, 1.0f, -1.0f) * kSize;
		v2.m_uv = make_vec2(0.0f, 1.0f);
		v2.m_color = white;

		// Bottom right
		vertex_pct_t v3{};

		v3.m_pos = make_vec3(-1.0f, 1.0f, -1.0f) * kSize;
		v3.m_uv = make_vec2(1.0f, 1.0f);
		v3.m_color = white;

		// Top right
		vertex_pct_t v4{};

		v4.m_pos = make_vec3(-1.0f, 1.0f, 1.0f) * kSize;
		v4.m_uv = make_vec2(1.0f, 0.0f);
		v4.m_color = white;

		mesh->vertices.push_back(v1);
		mesh->vertices.push_back(v2);
		mesh->vertices.push_back(v3);
		mesh->vertices.push_back(v4);
		mesh->indices.push_back(8);
		mesh->indices.push_back(9);
		mesh->indices.push_back(11);
		mesh->indices.push_back(11);
		mesh->indices.push_back(9);
		mesh->indices.push_back(10);
	}

	// neg y axis face
	{
		// Top left
		vertex_pct_t v1{};

		v1.m_pos = make_vec3(-1.0f, -1.0f, 1.0f) * kSize;
		v1.m_uv = make_vec2(0.0f, 0.0f);
		v1.m_color = white;

		// Bottom left
		vertex_pct_t v2{};

		v2.m_pos = make_vec3(-1.0f, -1.0f, -1.0f) * kSize;
		v2.m_uv = make_vec2(0.0f, 1.0f);
		v2.m_color = white;

		// Bottom right
		vertex_pct_t v3{};

		v3.m_pos = make_vec3(1.0f, -1.0f, -1.0f) * kSize;
		v3.m_uv = make_vec2(1.0f, 1.0f);
		v3.m_color = white;

		// Top right
		vertex_pct_t v4{};

		v4.m_pos = make_vec3(1.0f, -1.0f, 1.0f) * kSize;
		v4.m_uv = make_vec2(1.0f, 0.0f);
		v4.m_color = white;

		mesh->vertices.push_back(v1);
		mesh->vertices.push_back(v2);
		mesh->vertices.push_back(v3);
		mesh->vertices.push_back(v4);
		mesh->indices.push_back(12);
		mesh->indices.push_back(13);
		mesh->indices.push_back(15);
		mesh->indices.push_back(15);
		mesh->indices.push_back(13);
		mesh->indices.push_back(14);
	}

	// z axis face
	{
		// Top left
		vertex_pct_t v1{};

		v1.m_pos = make_vec3(1.0f, -1.0f, 1.0f) * kSize;
		v1.m_uv = make_vec2(0.0f, 0.0f);
		v1.m_color = white;

		// Bottom left
		vertex_pct_t v2{};

		v2.m_pos = make_vec3(1.0f, 1.0f, 1.0f) * kSize;
		v2.m_uv = make_vec2(0.0f, 1.0f);
		v2.m_color = white;

		// Bottom right
		vertex_pct_t v3{};

		v3.m_pos = make_vec3(-1.0f, 1.0f, 1.0f) * kSize;
		v3.m_uv = make_vec2(1.0f, 1.0f);
		v3.m_color = white;

		// Top right
		vertex_pct_t v4{};

		v4.m_pos = make_vec3(-1.0f, -1.0f, 1.0f) * kSize;
		v4.m_uv = make_vec2(1.0f, 0.0f);
		v4.m_color = white;

		mesh->vertices.push_back(v1);
		mesh->vertices.push_back(v2);
		mesh->vertices.push_back(v3);
		mesh->vertices.push_back(v4);
		mesh->indices.push_back(16);
		mesh->indices.push_back(17);
		mesh->indices.push_back(19);
		mesh->indices.push_back(19);
		mesh->indices.push_back(17);
		mesh->indices.push_back(18);
	}

	// neg z axis face
	{
		// Top left
		vertex_pct_t v1{};

		v1.m_pos = make_vec3(1.0f, 1.0f, -1.0f) * kSize;
		v1.m_uv = make_vec2(0.0f, 0.0f);
		v1.m_color = white;

		// Bottom left
		vertex_pct_t v2{};

		v2.m_pos = make_vec3(1.0f, -1.0f, -1.0f) * kSize;
		v2.m_uv = make_vec2(0.0f, 1.0f);
		v2.m_color = white;

		// Bottom right
		vertex_pct_t v3{};

		v3.m_pos = make_vec3(-1.0f, -1.0f, -1.0f) * kSize;
		v3.m_uv = make_vec2(1.0f, 1.0f);
		v3.m_color = white;

		// Top right
		vertex_pct_t v4{};

		v4.m_pos = make_vec3(-1.0f, 1.0f, -1.0f) * kSize;
		v4.m_uv = make_vec2(1.0f, 0.0f);
		v4.m_color = white;

		mesh->vertices.push_back(v1);
		mesh->vertices.push_back(v2);
		mesh->vertices.push_back(v3);
		mesh->vertices.push_back(v4);
		mesh->indices.push_back(20);
		mesh->indices.push_back(21);
		mesh->indices.push_back(23);
		mesh->indices.push_back(23);
		mesh->indices.push_back(21);
		mesh->indices.push_back(22);
	}

    mesh->topology = PrimitiveTopology::kTriangleList;

    return mesh;
}

mesh_t* mesh_load_octahedron()
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

mesh_t* mesh_load_uv_sphere()
{
    mesh_t* mesh = new mesh_t;

    u32 resolution = 64;  

    vec4 white_color = make_vec4(1.0f, 1.0f, 1.0f, 1.0f);

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

            vec4 pos = make_vec4(1.0f, 0.0f, 0.0f, 0.0f) * make_rotation_y_deg(y_deg) * make_rotation_z_deg(z_deg);

            u32 vert_index = mesh_add_vertex(*mesh, pos.xyz, white_color, uv);

            // top of the sphere
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

                mesh_add_triangle(*mesh, vert_index, vert_index_last_slice, prev_vert_index_last_slice);
                mesh_add_triangle(*mesh, vert_index, prev_vert_index_last_slice, prev_vert_index);
            }
        }
    }

    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

mesh_t* mesh_load_plane()
{
    mesh_t* mesh = new mesh_t;

    vec4 white = make_vec4(1.0f, 1.0f, 1.0f, 1.0f);
    mesh_add_vertex(*mesh, make_vec3(1.0f, 1.0f, 0.0f), white, make_vec2(0.0f, 0.0f));
    mesh_add_vertex(*mesh, make_vec3(-1.0f, 1.0f, 0.0f), white, make_vec2(0.0f, 1.0f));
    mesh_add_vertex(*mesh, make_vec3(-1.0f, -1.0f, 0.0f), white, make_vec2(1.0f, 1.0f));
    mesh_add_vertex(*mesh, make_vec3(1.0f, -1.0f, 0.0f), white, make_vec2(1.0f, 0.0f));

    mesh_add_triangle(*mesh, 0, 1, 2);
    mesh_add_triangle(*mesh, 0, 2, 3);

    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

mesh_t* mesh_load_cone()
{
    mesh_t* mesh = new mesh_t;

    u32 resolution = 64;
    f32 theta_deg = 360.0f / (f32)resolution; 

    vec4 white = get_color(Color::kWhite);

    for(u32 slice = 0; slice <= resolution; slice++)
    {
        vec3 top = make_vec3(0.0f, 0.0f, 1.0f);
        vec3 p0 = make_vec3(cos_deg(theta_deg * slice), sin_deg(theta_deg * slice), 0.0f);
        vec3 p1 = make_vec3(cos_deg(theta_deg * (slice + 1)), sin_deg(theta_deg * (slice + 1)), 0.0f);
        vec3 bot = make_vec3(0.0f, 0.0f, 0.0f);

        //f32 u0 = (f32)slice / (f32)resolution;
        //f32 u1 = (f32)(slice + 1) / (f32)resolution;

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

mesh_t* mesh_load_cylinder()
{
    mesh_t* mesh = new mesh_t;

    u32 resolution = 64;
    f32 theta_deg = 360.0f / (f32)resolution; 
    f32 side_u_scale = 3.0f;

    vec4 white = get_color(Color::kWhite);

    for(u32 slice = 0; slice <= resolution; slice++)
    {
        vec3 top = make_vec3(0.0f, 0.0f, 1.0f);
        vec2 p0_xy = make_vec2(cos_deg(theta_deg * slice), sin_deg(theta_deg * slice));
        vec2 p1_xy = make_vec2(cos_deg(theta_deg * (slice + 1)), sin_deg(theta_deg * (slice + 1)));
        vec3 bot = make_vec3(0.0f, 0.0f, -1.0f);

        // top triangle
        {
            f32 u0 = remap(p0_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 u1 = remap(p1_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v0 = remap(p0_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v1 = remap(p1_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

            mesh_add_vertex(*mesh, top, white, make_vec2(0.5, 0.5f));
            mesh_add_vertex(*mesh, make_vec3(p0_xy, 1.0f), white, make_vec2(u0, v0));
            mesh_add_vertex(*mesh, make_vec3(p1_xy, 1.0f), white, make_vec2(u1, v1));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }

        // side triangle 1
        {
            f32 u0 = (f32)slice / (f32)resolution;
            f32 u1 = (f32)(slice + 1) / (f32)resolution;
            
            u0 *= side_u_scale;
            u1 *= side_u_scale;

            mesh_add_vertex(*mesh, make_vec3(p0_xy, -1.0f), white, make_vec2(u0, 1.0f));
            mesh_add_vertex(*mesh, make_vec3(p1_xy, -1.0f), white, make_vec2(u1, 1.0f));
            mesh_add_vertex(*mesh, make_vec3(p1_xy, 1.0f), white, make_vec2(u1, 0.0f));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }

        // side triangle 2
        {
            f32 u0 = (f32)slice / (f32)resolution;
            f32 u1 = (f32)(slice + 1) / (f32)resolution;

            u0 *= side_u_scale;
            u1 *= side_u_scale;

            mesh_add_vertex(*mesh, make_vec3(p0_xy, -1.0f), white, make_vec2(u0, 1.0f));
            mesh_add_vertex(*mesh, make_vec3(p1_xy, 1.0f), white, make_vec2(u1, 0.0f));
            mesh_add_vertex(*mesh, make_vec3(p0_xy, 1.0f), white, make_vec2(u0, 0.0f));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }

        // bottom triangle
        {
            f32 u0 = remap(p0_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 u1 = remap(p1_xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v0 = remap(p0_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
            f32 v1 = remap(p1_xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

            mesh_add_vertex(*mesh, bot, white, make_vec2(0.5f, 0.5f));
            mesh_add_vertex(*mesh, make_vec3(p1_xy, -1.0f), white, make_vec2(u1, v1));
            mesh_add_vertex(*mesh, make_vec3(p0_xy, -1.0f), white, make_vec2(u0, v0));
            mesh_add_triangle_from_last_3_verts(*mesh);
        }
    }

    mesh->topology = PrimitiveTopology::kTriangleList;
    return mesh;
}

static
vec2 calc_2d_polar_to_xy_rad(f32 radians, f32 radius = 1.0f)
{
    return radius * make_vec2(cosf(radians), sinf(radians)); 
}

static
vec2 calc_2d_polar_to_xy_deg(f32 deg, f32 radius = 1.0f)
{
    return radius * make_vec2(cos_deg(deg), sin_deg(deg)); 
}

mesh_t* mesh_load_torus()
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
            u32 vert_index = mesh_add_vertex(*mesh, ring_pos_ws, get_color(Color::kWhite), make_vec2(percent_around_ring * u_scale, percent_around_torus * v_scale));

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
