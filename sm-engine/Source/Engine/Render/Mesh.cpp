#include "Engine/Render/Mesh.h"
#include "Engine/ThirdParty/tinyobjloader/tiny_obj_loader.h"

Mesh::Mesh()
	:m_topology(PrimitiveTopology::kTriangleList)
{
}

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

void Mesh::LoadPrimitives()
{
}

const Mesh* Mesh::GetPrimitive(PrimitiveMeshType primitive)
{
	return nullptr;
}
