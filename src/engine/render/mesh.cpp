#include "engine/render/mesh.h"
#include "engine/render/vertex.h"
#include "engine/render/vulkan/vulkan_renderer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "engine/thirdparty/tinyobjloader/tiny_obj_loader.h"

struct loaded_mesh_t
{
    u32 ref_count = 0;
    mesh_t* mesh = nullptr;
};

static std::vector<loaded_mesh_t> s_loaded_meshes;

static
mesh_t* loaded_mesh_acquire_or_create(mesh_id_t mesh_id, bool* out_was_already_loaded)
{
    mesh_t* mesh = nullptr;

    for(i32 i = 0; i < (i32)s_loaded_meshes.size(); i++)
    {
        if(s_loaded_meshes[i].mesh->id == mesh_id)
        {
            *out_was_already_loaded = true;
            s_loaded_meshes[i].ref_count++;
            return s_loaded_meshes[i].mesh;
        }
    }

    *out_was_already_loaded = false;

    mesh = new mesh_t;
    mesh->id = mesh_id;

    loaded_mesh_t loaded_mesh;
    loaded_mesh.mesh = mesh;
    loaded_mesh.ref_count = 1;
    s_loaded_meshes.push_back(loaded_mesh);

    return mesh;
}

void mesh_release(mesh_t* mesh)
{
    mesh_release(mesh->id);
}

void mesh_release(mesh_id_t mesh_id)
{
    for(i32 i = 0; i < (i32)s_loaded_meshes.size(); i++)
    {
        if(s_loaded_meshes[i].mesh->id == mesh_id)
        {
            s_loaded_meshes[i].ref_count--;
            if(s_loaded_meshes[i].ref_count == 0)
            {
                renderer_unload_mesh(mesh_id);
                delete s_loaded_meshes[i].mesh;
                s_loaded_meshes.erase(s_loaded_meshes.begin() + i);
            }
        }
    }
}

size_t mesh_calc_vertex_buffer_size(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return sizeof(mesh->m_vertices[0]) * mesh->m_vertices.size();
}

size_t mesh_calc_index_buffer_size(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return sizeof(mesh->m_indices[0]) * mesh->m_indices.size();
}

mesh_t* mesh_load_from_obj(const char* obj_filepath)
{
    mesh_id_t mesh_id = hash(obj_filepath);
    bool was_already_loaded = false;
    mesh_t* mesh = loaded_mesh_acquire_or_create(mesh_id, &was_already_loaded);
    if(was_already_loaded)
    {
        return mesh;
    }

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_filepath);
	ASSERT(loaded);

	std::vector<vertex_pct_t> vertices;
	std::vector<u32> indices;

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

			vertices.push_back(vertex);
			indices.push_back((u32)indices.size());
		}
	}

    mesh->m_vertices = vertices;
    mesh->m_indices = indices;
    mesh->topology = PrimitiveTopology::kTriangleList;

    renderer_load_mesh(mesh);

	return mesh;
}

mesh_t* mesh_load_cube()
{
    static mesh_id_t mesh_id = hash("cube");
    bool was_already_loaded = false;
    mesh_t* mesh = loaded_mesh_acquire_or_create(mesh_id, &was_already_loaded);
    if(was_already_loaded)
    {
        return mesh;
    }

	const f32 kSize = 1.0f;
	const vec4 white = make_vec4(1.0f, 1.0f, 1.0f, 1.0f);

	std::vector<vertex_pct_t> vertices;
	std::vector<u32> indices;

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

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(3);
		indices.push_back(3);
		indices.push_back(1);
		indices.push_back(2);
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

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);
		indices.push_back(4);
		indices.push_back(5);
		indices.push_back(7);
		indices.push_back(7);
		indices.push_back(5);
		indices.push_back(6);
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

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);
		indices.push_back(8);
		indices.push_back(9);
		indices.push_back(11);
		indices.push_back(11);
		indices.push_back(9);
		indices.push_back(10);
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

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);
		indices.push_back(12);
		indices.push_back(13);
		indices.push_back(15);
		indices.push_back(15);
		indices.push_back(13);
		indices.push_back(14);
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

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);
		indices.push_back(16);
		indices.push_back(17);
		indices.push_back(19);
		indices.push_back(19);
		indices.push_back(17);
		indices.push_back(18);
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

		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v3);
		vertices.push_back(v4);
		indices.push_back(20);
		indices.push_back(21);
		indices.push_back(23);
		indices.push_back(23);
		indices.push_back(21);
		indices.push_back(22);
	}

    mesh->m_vertices = vertices;
    mesh->m_indices = indices;
    mesh->topology = PrimitiveTopology::kTriangleList;

    renderer_load_mesh(mesh);

    return mesh;
}

mesh_t* mesh_load_axes()
{
    static mesh_id_t mesh_id = hash("axes");
    bool was_already_loaded = false;
    mesh_t* mesh = loaded_mesh_acquire_or_create(mesh_id, &was_already_loaded);
    if(was_already_loaded)
    {
        return mesh;
    }

	std::vector<vertex_pct_t> vertices;
	std::vector<u32> indices;

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

	vertices.push_back(xOrigin);
	vertices.push_back(xAxis);
	vertices.push_back(yOrigin);
	vertices.push_back(yAxis);
	vertices.push_back(zOrigin);
	vertices.push_back(zAxis);	

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(4);
	indices.push_back(5);

    mesh->m_vertices = vertices;
    mesh->m_indices = indices;
    mesh->topology = PrimitiveTopology::kLineList;

    renderer_load_mesh(mesh);

    return mesh;
}
