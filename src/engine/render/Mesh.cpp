#include "Engine/Render/Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Engine/ThirdParty/tinyobjloader/tiny_obj_loader.h"

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

bool Mesh::Setup(const std::vector<Vertex>& vertices, const std::vector<U32>& indices)
{
	m_vertices = vertices;
	m_indices = indices;
	return true;
}

void Mesh::Teardown()
{
}

const std::vector<Vertex>& Mesh::GetVertices() const
{
	return m_vertices;
}

const std::vector<U32>& Mesh::GetIndices() const
{
	return m_indices;
}

U32 Mesh::GetNumVertices() const
{
	return (U32)m_vertices.size();
}

U32 Mesh::GetNumIndices() const
{
	return (U32)m_indices.size();
}

size_t Mesh::CalculateVertexBufferSize() const
{
	return sizeof(m_vertices[0]) * m_vertices.size();
}

size_t Mesh::CalculateIndexBufferSize() const 
{
	return sizeof(m_indices[0]) * m_indices.size();
}

std::vector<VkVertexInputBindingDescription> Mesh::GetVertexInputBindingDesc() const
{
	std::vector<VkVertexInputBindingDescription> bindingDescs = { GetBindingDescription(m_vertices[0]) };
	return bindingDescs;
}

std::vector<VkVertexInputAttributeDescription> Mesh::GetVertexAttrDescs() const
{
	return GetAttributeDescs(m_vertices[0]);
}

Mesh* MeshLoaders::LoadMeshFromObj(const char* objFilepath)
{
	Mesh* pMesh = new Mesh();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool didLoad = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFilepath);
	ASSERT(didLoad);

	std::vector<Vertex> vertices;
	std::vector<U32> indices;

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.m_pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.m_texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.m_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

			vertices.push_back(vertex);
			indices.push_back(static_cast<U32>(indices.size()));
		}
	}

	ASSERT(pMesh->Setup(vertices, indices));
	return pMesh;
}

Mesh* MeshLoaders::LoadCubeMesh()
{
	Mesh* pMesh = new Mesh();

	const F32 kSize = 1.0f;
	const Vec4 kWhiteColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

	std::vector<Vertex> vertices;
	std::vector<U32> indices;

	// x axis face
	{
		// Top left
		Vertex v1{};

		v1.m_pos = Vec3(1.0f, -1.0f, 1.0f) * kSize;
		v1.m_texCoord = Vec2(0.0f, 0.0f);
		v1.m_color = kWhiteColor;

		// Bottom left
		Vertex v2{};

		v2.m_pos = Vec3(1.0f, -1.0f, -1.0f) * kSize;
		v2.m_texCoord = Vec2(0.0f, 1.0f);
		v2.m_color = kWhiteColor;

		// Bottom right
		Vertex v3{};

		v3.m_pos = Vec3(1.0f, 1.0f, -1.0f) * kSize;
		v3.m_texCoord = Vec2(1.0f, 1.0f);
		v3.m_color = kWhiteColor;

		// Top right
		Vertex v4{};

		v4.m_pos = Vec3(1.0f, 1.0f, 1.0f) * kSize;
		v4.m_texCoord = Vec2(1.0f, 0.0f);
		v4.m_color = kWhiteColor;

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
		Vertex v1{};

		v1.m_pos = Vec3(-1.0f, 1.0f, 1.0f) * kSize;
		v1.m_texCoord = Vec2(0.0f, 0.0f);
		v1.m_color = kWhiteColor;

		// Bottom left
		Vertex v2{};

		v2.m_pos = Vec3(-1.0f, 1.0f, -1.0f) * kSize;
		v2.m_texCoord = Vec2(0.0f, 1.0f);
		v2.m_color = kWhiteColor;

		// Bottom right
		Vertex v3{};

		v3.m_pos = Vec3(-1.0f, -1.0f, -1.0f) * kSize;
		v3.m_texCoord = Vec2(1.0f, 1.0f);
		v3.m_color = kWhiteColor;

		// Top right
		Vertex v4{};

		v4.m_pos = Vec3(-1.0f, -1.0f, 1.0f) * kSize;
		v4.m_texCoord = Vec2(1.0f, 0.0f);
		v4.m_color = kWhiteColor;

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
		Vertex v1{};

		v1.m_pos = Vec3(1.0f, 1.0f, 1.0f) * kSize;
		v1.m_texCoord = Vec2(0.0f, 0.0f);
		v1.m_color = kWhiteColor;

		// Bottom left
		Vertex v2{};

		v2.m_pos = Vec3(1.0f, 1.0f, -1.0f) * kSize;
		v2.m_texCoord = Vec2(0.0f, 1.0f);
		v2.m_color = kWhiteColor;

		// Bottom right
		Vertex v3{};

		v3.m_pos = Vec3(-1.0f, 1.0f, -1.0f) * kSize;
		v3.m_texCoord = Vec2(1.0f, 1.0f);
		v3.m_color = kWhiteColor;

		// Top right
		Vertex v4{};

		v4.m_pos = Vec3(-1.0f, 1.0f, 1.0f) * kSize;
		v4.m_texCoord = Vec2(1.0f, 0.0f);
		v4.m_color = kWhiteColor;

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
		Vertex v1{};

		v1.m_pos = Vec3(-1.0f, -1.0f, 1.0f) * kSize;
		v1.m_texCoord = Vec2(0.0f, 0.0f);
		v1.m_color = kWhiteColor;

		// Bottom left
		Vertex v2{};

		v2.m_pos = Vec3(-1.0f, -1.0f, -1.0f) * kSize;
		v2.m_texCoord = Vec2(0.0f, 1.0f);
		v2.m_color = kWhiteColor;

		// Bottom right
		Vertex v3{};

		v3.m_pos = Vec3(1.0f, -1.0f, -1.0f) * kSize;
		v3.m_texCoord = Vec2(1.0f, 1.0f);
		v3.m_color = kWhiteColor;

		// Top right
		Vertex v4{};

		v4.m_pos = Vec3(1.0f, -1.0f, 1.0f) * kSize;
		v4.m_texCoord = Vec2(1.0f, 0.0f);
		v4.m_color = kWhiteColor;

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
		Vertex v1{};

		v1.m_pos = Vec3(1.0f, -1.0f, 1.0f) * kSize;
		v1.m_texCoord = Vec2(0.0f, 0.0f);
		v1.m_color = kWhiteColor;

		// Bottom left
		Vertex v2{};

		v2.m_pos = Vec3(1.0f, 1.0f, 1.0f) * kSize;
		v2.m_texCoord = Vec2(0.0f, 1.0f);
		v2.m_color = kWhiteColor;

		// Bottom right
		Vertex v3{};

		v3.m_pos = Vec3(-1.0f, 1.0f, 1.0f) * kSize;
		v3.m_texCoord = Vec2(1.0f, 1.0f);
		v3.m_color = kWhiteColor;

		// Top right
		Vertex v4{};

		v4.m_pos = Vec3(-1.0f, -1.0f, 1.0f) * kSize;
		v4.m_texCoord = Vec2(1.0f, 0.0f);
		v4.m_color = kWhiteColor;

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
		Vertex v1{};

		v1.m_pos = Vec3(1.0f, 1.0f, -1.0f) * kSize;
		v1.m_texCoord = Vec2(0.0f, 0.0f);
		v1.m_color = kWhiteColor;

		// Bottom left
		Vertex v2{};

		v2.m_pos = Vec3(1.0f, -1.0f, -1.0f) * kSize;
		v2.m_texCoord = Vec2(0.0f, 1.0f);
		v2.m_color = kWhiteColor;

		// Bottom right
		Vertex v3{};

		v3.m_pos = Vec3(-1.0f, -1.0f, -1.0f) * kSize;
		v3.m_texCoord = Vec2(1.0f, 1.0f);
		v3.m_color = kWhiteColor;

		// Top right
		Vertex v4{};

		v4.m_pos = Vec3(-1.0f, 1.0f, -1.0f) * kSize;
		v4.m_texCoord = Vec2(1.0f, 0.0f);
		v4.m_color = kWhiteColor;

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

	ASSERT(pMesh->Setup(vertices, indices));
	return pMesh;
}

Mesh* MeshLoaders::LoadWorldAxesMesh()
{
	Mesh* pMesh = new Mesh();

	std::vector<Vertex> vertices;
	std::vector<U32> indices;

	// X
	Vertex xOrigin{};
	xOrigin.m_pos = Vec3(0.0f, 0.0f, 0.0f);
	xOrigin.m_texCoord = Vec2(0.0f, 0.0f);
	xOrigin.m_color = Vec4(1.0f, 0.0f, 0.0f, 1.0f);

	Vertex xAxis{};
	xAxis.m_pos = Vec3(1.0f, 0.0f, 0.0f);
	xAxis.m_texCoord = Vec2(0.0f, 0.0f);
	xAxis.m_color = Vec4(1.0f, 0.0f, 0.0f, 1.0f);

	// Y
	Vertex yOrigin{};
	yOrigin.m_pos = Vec3(0.0f, 0.0f, 0.0f);
	yOrigin.m_texCoord = Vec2(0.0f, 0.0f);
	yOrigin.m_color = Vec4(0.0f, 1.0f, 0.0f, 1.0f);

	Vertex yAxis{};
	yAxis.m_pos = Vec3(0.0f, 1.0f, 0.0f);
	yAxis.m_texCoord = Vec2(0.0f, 0.0f);
	yAxis.m_color = Vec4(0.0f, 1.0f, 0.0f, 1.0f);

	// Z
	Vertex zOrigin{};
	zOrigin.m_pos = Vec3(0.0f, 0.0f, 0.0f);
	zOrigin.m_texCoord = Vec2(0.0f, 0.0f);
	zOrigin.m_color = Vec4(0.0f, 0.0f, 1.0f, 1.0f);

	Vertex zAxis{};
	zAxis.m_pos = Vec3(0.0f, 0.0f, 1.0f);
	zAxis.m_texCoord = Vec2(0.0f, 0.0f);
	zAxis.m_color = Vec4(0.0f, 0.0f, 1.0f, 1.0f);

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

	ASSERT(pMesh->Setup(vertices, indices));
	return pMesh;
}