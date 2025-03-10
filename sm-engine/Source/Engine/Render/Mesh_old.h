#pragma once

#include "Engine/Core/Hash_old.h"
#include "Engine/Core/Types.h"
#include "Engine/Math/Mat44_old.h"
#include "Engine/Render/Vertex.h"

enum class PrimitiveTopology : U8
{
	kTriangleList,
	kLineList,
	kPointList
};

enum PrimitiveMeshType : U32
{
	kAxes,
	kTetrahedron,
	kHexahedron,
	kCube = kHexahedron,
	kOctahedron,
	kUvSphere,
	kPlane,
	kQuad,
	kCone,
	kCylinder,
	kTorus
};

class Mesh
{
public:
	Mesh();

	size_t CalcVertexBufferSize() const;
	size_t CalcIndexBufferSize() const;

	static void	InitPrimitives();
	static const Mesh* GetPrimitive(PrimitiveMeshType primtive);

	PrimitiveTopology m_topology;
	std::vector<VertexPCT> m_vertices;
	std::vector<U32> m_indices;
};
