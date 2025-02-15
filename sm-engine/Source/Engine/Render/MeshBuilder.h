#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Core/Color_old.h"
#include "Engine/Math/Mat44_old.h"
#include "Engine/Render/Mesh_old.h"
#include "Engine/Render/Vertex.h"

class MeshBuilder
{
public:
	MeshBuilder();

	void Reset();
	void Begin();

	void SetTopology(PrimitiveTopology topology);

	U32  AddVertex(const Vec3& pos, const ColorF32& color, const Vec2& uv);
	U32  AddVertex(const VertexPCT& vertex);
	U32  AddVertexAndIndex(const Vec3& pos, const ColorF32& color, const Vec2& uv);
	U32  AddVertexAndIndex(const VertexPCT& vertex);
	void AddIndex(U32 index);
	void AddTriangle(U32 index0, U32 index1, U32 index2);
	void AddTriangleFromLast3Vertexes();

	void AddQuad3d(const Vec3& centerPos, const Vec3& right, const Vec3& up, F32 halfWidth, F32 halfHeight, U32 resolution = 1);
	void AddQuad3d(const Vec3& topLeft, const Vec3& topRight, const Vec3& bottomRight, const Vec3& bottomLeft);
	void AddAxesLines3d(const Vec3& origin, const Vec3& i, const Vec3& j, const Vec3& k);
	void AddUvSphere(const Vec3& origin, F32 radius, U32 resolution = 32);
	void AddCube(const Vec3& center, F32 radius, U32 resolution = 1);
	void AddFrustum(const Mat44& viewProjection);
	void AddCylinder(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, U32 resolution);
	void AddCone(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, U32 resolution);
	void AddArrow(const Vec3& origin, const Vec3& dir, F32 length, F32 bodyRadius, F32 tipRadius);
	void AddArrowWithTipAtPosition(const Vec3& tipPosition, const Vec3& dirFromTipToArrowBody, F32 length, F32 bodyRadius, F32 tipRadius);

	void AddFromObj(const char* objFilepath);

	Mesh* Finish();

	static Mesh* BuildFromObj(const char* objFilepath);

	Mesh* m_workingMesh;
	PrimitiveTopology m_workingTopology;
};
