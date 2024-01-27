#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Core/Color.h"
#include "Engine/Math/Mat44.h"
#include "Engine/Render/Vertex.h"

class Mesh;

class MeshBuilder
{
public:
	void Reset();
	U32 AddVertex(const Vec3& pos, const ColorF32& color, const Vec2& uv);
	void AddTriangle(U32 index0, U32 index1, U32 index2);
	void AddTriangleFromLast3Vertexes();
	Mesh* BuildMesh();

	static Mesh* BuildQuad3d(const Vec3& centerPos, const Vec3& right, const Vec3& up, F32 halfWidth, F32 halfHeight, U32 resolution = 1);
	static Mesh* BuildQuad3d(const Vec3& topLeft, const Vec3& topRight, const Vec3& bottomRight, const Vec3& bottomLeft);
	static Mesh* BuildAxesLines3d(const Vec3& origin, const Vec3& i, const Vec3& j, const Vec3& k);
	static Mesh* BuildUvSphere(Vec3& origin, F32 radius, U32 resolution = 32);
	static Mesh* BuildCube(const Vec3& center, F32 radius, U32 resolution = 1);
	static Mesh* BuildFrustum(const Mat44& viewProjection);
	static Mesh* BuildCylinder(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, F32 resolution);
	static Mesh* BuildCone(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, F32 resolution);
	static Mesh* BuildArrow(const Vec3& origin, const Vec3& dir, F32 length, F32 bodyRadius, F32 tipRadius);
	static Mesh* BuildArrowTipToOrigin(const Vec3& tipPosition, const Vec3& dirToArrowOrigin, F32 length, F32 bodyRadius, F32 tipRadius);

	std::vector<VertexPCT> m_vertices;
	std::vector<U32> m_indices;
};
