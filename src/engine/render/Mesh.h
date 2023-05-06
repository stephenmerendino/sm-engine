#pragma once

#include "engine/core/Types.h"
#include "engine/math/Vec3.h"
#include "engine/render/Vertex.h"

#include <vector>

class Mesh
{
public:
	Mesh();
	~Mesh();

	bool Setup(const std::vector<Vertex>& vertexes, const std::vector<U32>& indices);
	void Teardown();

	const std::vector<Vertex>& GetVertices() const;
	const std::vector<U32>& GetIndices() const;
	U32 GetNumVertices() const;
	U32 GetNumIndices() const;
	size_t CalculateVertexBufferSize() const;
	size_t CalculateIndexBufferSize() const;
	std::vector<VkVertexInputBindingDescription> GetVertexInputBindingDesc() const;
	std::vector<VkVertexInputAttributeDescription> GetVertexAttrDescs() const;

private:
	std::vector<Vertex> m_vertices;
	std::vector<U32> m_indices;
};

namespace MeshLoaders
{
	// Load from file
	Mesh* LoadMeshFromObj(const char* objFilepath);

	// Load basic shapes
	Mesh* LoadCubeMesh();
	// LoadConeMesh
	// LoadPyramidMesh
	// LoadCylinderMesh
	// LoadSphereMesh
	// Load...

	// Load misc types
	Mesh* LoadWorldAxesMesh();
};
