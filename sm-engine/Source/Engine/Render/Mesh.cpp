#include "Engine/Render/Mesh.h"
#include "Engine/ThirdParty/tinyobjloader/tiny_obj_loader.h"

size_t Mesh::CalcVertexBufferSize() const
{
	if (m_vertices.size() == 0) return 0;
	return sizeof(m_vertices[0]) * m_vertices.size();
}

size_t Mesh::CalcIndexBufferSize() const
{
	if (m_indices.size() == 0) return 0;
	return sizeof(m_indices[0]) * m_indices.size();
}

void Mesh::InitFromObj(const char* objFilepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFilepath);
	SM_ASSERT(loaded);

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			VertexPCT vertex;

			vertex.m_pos = Vec3(attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]);

			vertex.m_uv = Vec2(attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);

			vertex.m_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

			m_vertices.push_back(vertex);
			m_indices.push_back((U32)m_indices.size());
		}
	}

	m_topology = PrimitiveTopology::kTriangleList;
}

void Mesh::LoadPrimitives()
{
}

const Mesh* Mesh::GetPrimitive(PrimitiveMeshType primtive)
{
	return nullptr;
}
