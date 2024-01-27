#include "Engine/Render/MeshBuilder.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Core/Color.h"

void MeshBuilder::Reset()
{
	m_vertices.clear();
	m_indices.clear();
}

U32 MeshBuilder::AddVertex(const Vec3& pos, const ColorF32& color, const Vec2& uv)
{
	m_vertices.push_back(VertexPCT(pos, color.ToVec4(), uv));
	return m_vertices.size() - 1;
}

void MeshBuilder::AddTriangle(U32 index0, U32 index1, U32 index2)
{
	m_indices.push_back(index0);
	m_indices.push_back(index1);
	m_indices.push_back(index2);
}

void MeshBuilder::AddTriangleFromLast3Vertexes()
{
	size_t size = m_vertices.size();
	SM_ASSERT_MSG(size >= 3, "Trying to build triangle in mesh with less than 3 vertexes.\n");
	AddTriangle(size - 3, size - 2, size - 1);
}

Mesh* MeshBuilder::BuildMesh()
{
	Mesh* mesh = new Mesh();
	mesh->m_vertices = m_vertices;
	mesh->m_indices = m_indices;
	return mesh;
}

Mesh* MeshBuilder::BuildQuad3d(const Vec3& centerPos, const Vec3& right, const Vec3& up, F32 halfWidth, F32 halfHeight, U32 resolution)
{
	MeshBuilder builder;

	Vec3 rightNorm = right.Normalized();
	Vec3 upNorm = up.Normalized();

	F32 widthStep = (halfWidth * 2.0f) / (F32)resolution;
	F32 heightStep = (halfHeight * 2.0f) / (F32)resolution;

	Vec3 topLeft = centerPos + (-rightNorm * halfWidth) + (upNorm * halfHeight);

	for (I32 y = 0; y <= resolution; y++)
	{
		for (I32 x = 0; x <= resolution; x++)
		{
			F32 u = (F32)x / (F32)(resolution);
			F32 v = (F32)y / (F32)(resolution);
			Vec3 vertPos = topLeft + (right * widthStep * x) + (-up * heightStep * y);
			U32 vertIndex = builder.AddVertex(vertPos, ColorF32::WHITE, Vec2(u, v));

			if (y > 0 && x > 0)
			{
				U32 prevVertIndex = vertIndex - 1;
				U32 vertIndexLastRow = (vertIndex - resolution) - 1;
				U32 prevVertIndexLastRow = vertIndexLastRow - 1;
				builder.AddTriangle(vertIndex, prevVertIndexLastRow, prevVertIndex);
				builder.AddTriangle(vertIndex, vertIndexLastRow, prevVertIndexLastRow);
			}
		}
	}

	return builder.BuildMesh();
}

Mesh* MeshBuilder::BuildQuad3d(const Vec3& topLeft, const Vec3& topRight, const Vec3& bottomRight, const Vec3& bottomLeft)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildAxesLines3d(const Vec3& origin, const Vec3& i, const Vec3& j, const Vec3& k)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildUvSphere(Vec3& origin, F32 radius, U32 resolution = 32)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildCube(const Vec3& center, F32 radius, U32 resolution = 1)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildFrustum(const Mat44& viewProjection)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildCylinder(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, F32 resolution)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildCone(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, F32 resolution)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildArrow(const Vec3& origin, const Vec3& dir, F32 length, F32 bodyRadius, F32 tipRadius)
{
	return nullptr;
}

Mesh* MeshBuilder::BuildArrowTipToOrigin(const Vec3& tipPosition, const Vec3& dirToArrowOrigin, F32 length, F32 bodyRadius, F32 tipRadius)
{
	return nullptr;
}
