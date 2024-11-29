#include "Engine/Render/MeshBuilder.h"
#include "Engine/Config_old.h"
#include "Engine/Core/Color_old.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Engine/ThirdParty/tinyobjloader/tiny_obj_loader.h"

#define CHECK_WORKING_MESH() SM_ASSERT_MSG(m_workingMesh != nullptr, "Begin() hasn't been called on MeshBuilder to start building a new mesh\n")

MeshBuilder::MeshBuilder()
	:m_workingMesh(nullptr)
	,m_workingTopology(PrimitiveTopology::kTriangleList)
{
}

void MeshBuilder::Reset()
{
	if (m_workingMesh) delete m_workingMesh;
	m_workingTopology = PrimitiveTopology::kTriangleList;
}

void MeshBuilder::Begin()
{
	m_workingMesh = new Mesh();
}

void MeshBuilder::SetTopology(PrimitiveTopology topology)
{
	m_workingTopology = topology;
}

U32 MeshBuilder::AddVertex(const Vec3& pos, const ColorF32& color, const Vec2& uv)
{
	return AddVertex(VertexPCT(pos, color.ToVec4(), uv));
}

U32 MeshBuilder::AddVertex(const VertexPCT& vertex)
{
	CHECK_WORKING_MESH();
	m_workingMesh->m_vertices.push_back(vertex);
	return (U32)m_workingMesh->m_vertices.size() - 1;
}

U32 MeshBuilder::AddVertexAndIndex(const Vec3& pos, const ColorF32& color, const Vec2& uv)
{
	return AddVertexAndIndex(VertexPCT(pos, color.ToVec4(), uv));
}

U32 MeshBuilder::AddVertexAndIndex(const VertexPCT& vertex)
{
	CHECK_WORKING_MESH();
	m_workingMesh->m_vertices.push_back(vertex);
	U32 newVertexIndex = (U32)m_workingMesh->m_vertices.size() - 1;
	m_workingMesh->m_indices.push_back(newVertexIndex);
	return newVertexIndex;
}

void MeshBuilder::AddIndex(U32 index)
{
	CHECK_WORKING_MESH();
	m_workingMesh->m_indices.push_back(index);
}

void MeshBuilder::AddTriangle(U32 index0, U32 index1, U32 index2)
{
	CHECK_WORKING_MESH();
	m_workingMesh->m_indices.push_back(index0);
	m_workingMesh->m_indices.push_back(index1);
	m_workingMesh->m_indices.push_back(index2);
}

void MeshBuilder::AddTriangleFromLast3Vertexes()
{
	CHECK_WORKING_MESH();
	U32 size = (U32)m_workingMesh->m_vertices.size();
	SM_ASSERT_MSG(size >= 3, "Trying to build triangle in mesh with less than 3 vertexes.\n");
	AddTriangle(size - 3, size - 2, size - 1);
}

Mesh* MeshBuilder::Finish()
{
	CHECK_WORKING_MESH();
	Mesh* meshToReturn = m_workingMesh;
	meshToReturn->m_topology = m_workingTopology;
	m_workingMesh = nullptr;
	return meshToReturn;
}

void MeshBuilder::AddQuad3d(const Vec3& centerPos, const Vec3& right, const Vec3& up, F32 halfWidth, F32 halfHeight, U32 resolution)
{
	Vec3 rightNorm = right.Normalized();
	Vec3 upNorm = up.Normalized();

	F32 widthStep = (halfWidth * 2.0f) / (F32)resolution;
	F32 heightStep = (halfHeight * 2.0f) / (F32)resolution;

	Vec3 topLeft = centerPos + (-rightNorm * halfWidth) + (upNorm * halfHeight);

	for (U32 y = 0; y <= resolution; y++)
	{
		for (U32 x = 0; x <= resolution; x++)
		{
			F32 u = (F32)x / (F32)(resolution);
			F32 v = (F32)y / (F32)(resolution);
			Vec3 vertPos = topLeft + (right * widthStep * (F32)x) + (-up * heightStep * (F32)y);
			U32 vertIndex = AddVertex(vertPos, ColorF32::WHITE, Vec2(u, v));

			if (y > 0 && x > 0)
			{
				U32 prevVertIndex = vertIndex - 1;
				U32 vertIndexLastRow = (vertIndex - resolution) - 1;
				U32 prevVertIndexLastRow = vertIndexLastRow - 1;
				AddTriangle(vertIndex, prevVertIndexLastRow, prevVertIndex);
				AddTriangle(vertIndex, vertIndexLastRow, prevVertIndexLastRow);
			}
		}
	}
}

void MeshBuilder::AddQuad3d(const Vec3& topLeft, const Vec3& topRight, const Vec3& bottomRight, const Vec3& bottomLeft)
{
	U32 topLeftIndex = AddVertex(topLeft, ColorF32::WHITE, Vec2(0.0f, 0.0f));
	U32 topRightIndex = AddVertex(topRight, ColorF32::WHITE, Vec2(1.0f, 0.0f));
	U32 bottomRightIndex = AddVertex(bottomRight, ColorF32::WHITE, Vec2(1.0f, 1.0f));
	U32 bottomLeftIndex = AddVertex(bottomLeft, ColorF32::WHITE, Vec2(0.0f, 1.0f));

	AddTriangle(topLeftIndex, bottomRightIndex, topRightIndex);
	AddTriangle(topLeftIndex, bottomLeftIndex, bottomRightIndex);
}

void MeshBuilder::AddAxesLines3d(const Vec3& origin, const Vec3& i, const Vec3& j, const Vec3& k)
{
	Vec2 uv = Vec2::ZERO;
	ColorF32 red = ColorF32::RED;
	ColorF32 green = ColorF32::GREEN;
	ColorF32 blue = ColorF32::BLUE;

	AddIndex(AddVertex(origin, red, uv));
	AddIndex(AddVertex(i, red, uv));
	AddIndex(AddVertex(origin, green, uv));
	AddIndex(AddVertex(j, green, uv));
	AddIndex(AddVertex(origin, blue, uv));
	AddIndex(AddVertex(k, blue, uv));
}

void MeshBuilder::AddUvSphere(const Vec3& origin, F32 radius, U32 resolution)
{
	F32 uDeltaDeg = 360.0f / (F32)resolution;
	F32 vDeltaDeg = 180.0f / (F32)resolution;

	for (U32 vSlice = 0; vSlice <= resolution; vSlice++)
	{
		for (U32 uSlice = 0; uSlice <= resolution; uSlice++)
		{
			F32 u = (F32)uSlice / (F32)resolution;
			F32 v = (F32)vSlice / (F32)resolution;
			Vec2 uv(u, v);

			F32 yDeg = -90.0f + ((F32)vSlice * vDeltaDeg);
			F32 zDeg = (F32)uSlice * uDeltaDeg;

			Vec4 pos = Vec4(radius, 0.0f, 0.0f, 0.0f) * Mat44::CreateRotationYDegs(yDeg) * Mat44::CreateRotationZDegs(zDeg);
			Vec3 finalPos = origin + pos.xyz;

			U32 vertIndex = AddVertex(finalPos, ColorF32::WHITE, uv);

			if (vSlice == 0)
			{
				continue;
			}

			// normal triangle add
			if (uSlice > 0)
			{
				U32 prevVertIndex = vertIndex - 1;
				U32 vertIndexLastSlice = (vertIndex - resolution) - 1;
				U32 prevVertIndexLastSlice = vertIndexLastSlice - 1;

				AddTriangle(vertIndex, vertIndexLastSlice, prevVertIndexLastSlice);
				AddTriangle(vertIndex, prevVertIndexLastSlice, prevVertIndex);
			}
		}
	}
}

void MeshBuilder::AddCube(const Vec3& center, F32 radius, U32 resolution)
{
	AddQuad3d(center + Vec3::FORWARD * radius, Vec3::LEFT, Vec3::UP, radius, radius, resolution);
	AddQuad3d(center + Vec3::LEFT * radius, Vec3::BACKWARD, Vec3::UP, radius, radius, resolution);
	AddQuad3d(center + Vec3::BACKWARD * radius, Vec3::RIGHT, Vec3::UP, radius, radius, resolution);
	AddQuad3d(center + Vec3::RIGHT * radius, Vec3::FORWARD, Vec3::UP, radius, radius, resolution);
	AddQuad3d(center + Vec3::UP * radius, Vec3::RIGHT, Vec3::FORWARD, radius, radius, resolution);
	AddQuad3d(center + Vec3::DOWN * radius, Vec3::LEFT, Vec3::FORWARD, radius, radius, resolution);
}

void MeshBuilder::AddFrustum(const Mat44& viewProjection)
{
	Mat44 inverseViewProj = viewProjection.Inversed();

	Vec4 nearTopLeft = Vec4(-1.0f, -1.0f, 0.0f, 1.0f) * inverseViewProj;
	Vec4 nearTopRight = Vec4(1.0f, -1.0f, 0.0f, 1.0f) * inverseViewProj;
	Vec4 nearBottomRight = Vec4(1.0f, 1.0f, 0.0f, 1.0f) * inverseViewProj;
	Vec4 nearBottomLeft = Vec4(-1.0f, 1.0f, 0.0f, 1.0f) * inverseViewProj;
	Vec4 farTopLeft = Vec4(-1.0f, -1.0f, 1.0f, 1.0f) * inverseViewProj;
	Vec4 farTopRight = Vec4(1.0f, -1.0f, 1.0f, 1.0f) * inverseViewProj;
	Vec4 farBottomRight = Vec4(1.0f, 1.0f, 1.0f, 1.0f) * inverseViewProj;
	Vec4 farBottomLeft = Vec4(-1.0f, 1.0f, 1.0f, 1.0f) * inverseViewProj;

	nearTopLeft /= nearTopLeft.w;
	nearTopRight /= nearTopRight.w;
	nearBottomLeft /= nearBottomLeft.w;
	nearBottomRight /= nearBottomRight.w;
	farTopLeft /= farTopLeft.w;
	farTopRight /= farTopRight.w;
	farBottomRight /= farBottomRight.w;
	farBottomLeft /= farBottomLeft.w;

	//near plane
	AddQuad3d(nearTopLeft.xyz, nearTopRight.xyz, nearBottomRight.xyz, nearBottomLeft.xyz);

	// right plane
	AddQuad3d(nearTopRight.xyz, farTopRight.xyz, farBottomRight.xyz, nearBottomRight.xyz);

	// top plane
	AddQuad3d(nearTopLeft.xyz, farTopLeft.xyz, farTopRight.xyz, nearTopRight.xyz);

	// left plane
	AddQuad3d(nearBottomLeft.xyz, farBottomLeft.xyz, farTopLeft.xyz, nearTopLeft.xyz);

	// bottom plane
	AddQuad3d(nearBottomRight.xyz, farBottomRight.xyz, farBottomLeft.xyz, nearBottomLeft.xyz);

	// far plane
	AddQuad3d(farTopRight.xyz, farTopLeft.xyz, farBottomLeft.xyz, farBottomRight.xyz);
}

void MeshBuilder::AddCylinder(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, U32 resolution)
{
	F32 thetaDeg = 360.0f / (F32)resolution;
	F32 sideUScale = 3.0f;

	Vec3 k = dir.Normalized();
	Mat44 localToWorld = Mat44::CreateBasisFromK(k);
	localToWorld.Scale(baseRadius, baseRadius, height);
	localToWorld.Translate(baseCenter);

	for (U32 slice = 0; slice < resolution; slice++)
	{
		Vec3 top(0.0f, 0.0f, 1.0f);
		Vec2 p0xy(CosDeg(thetaDeg * slice), SinDeg(thetaDeg * slice));
		Vec2 p1xy(CosDeg(thetaDeg * (slice + 1)), SinDeg(thetaDeg * (slice + 1)));
		Vec3 bot(0.0f, 0.0f, 0.0f);

		Vec3 topWs = localToWorld.TransformPoint(top);
		Vec3 p0TopWs = localToWorld.TransformPoint(Vec3(p0xy, 1.0f));
		Vec3 p1TopWs = localToWorld.TransformPoint(Vec3(p1xy, 1.0f));
		Vec3 p0BotWs = localToWorld.TransformPoint(Vec3(p0xy, 0.0f));
		Vec3 p1BotWs = localToWorld.TransformPoint(Vec3(p1xy, 0.0f));
		Vec3 botWs = localToWorld.TransformPoint(bot);

		// top triangle
		{
			F32 u0 = Remap(p0xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
			F32 u1 = Remap(p1xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
			F32 v0 = Remap(p0xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
			F32 v1 = Remap(p1xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

			AddVertex(topWs, ColorF32::WHITE, Vec2(0.5, 0.5f));
			AddVertex(p0TopWs, ColorF32::WHITE, Vec2(u0, v0));
			AddVertex(p1TopWs, ColorF32::WHITE, Vec2(u1, v1));
			AddTriangleFromLast3Vertexes();
		}

		// side triangle 1
		{
			F32 u0 = (F32)slice / (F32)resolution;
			F32 u1 = (F32)(slice + 1) / (F32)resolution;

			u0 *= sideUScale;
			u1 *= sideUScale;

			AddVertex(p0BotWs, ColorF32::WHITE, Vec2(u0, 1.0f));
			AddVertex(p1BotWs, ColorF32::WHITE, Vec2(u1, 1.0f));
			AddVertex(p1TopWs, ColorF32::WHITE, Vec2(u1, 0.0f));
			AddTriangleFromLast3Vertexes();
		}

		// side triangle 2
		{
			F32 u0 = (F32)slice / (F32)resolution;
			F32 u1 = (F32)(slice + 1) / (F32)resolution;

			u0 *= sideUScale;
			u1 *= sideUScale;

			AddVertex(p0BotWs, ColorF32::WHITE, Vec2(u0, 1.0f));
			AddVertex(p1TopWs, ColorF32::WHITE, Vec2(u1, 0.0f));
			AddVertex(p0TopWs, ColorF32::WHITE, Vec2(u0, 0.0f));
			AddTriangleFromLast3Vertexes();
		}

		// bottom triangle
		{
			F32 u0 = Remap(p0xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
			F32 u1 = Remap(p1xy.x, -1.0f, 1.0f, 0.0f, 1.0f);
			F32 v0 = Remap(p0xy.y, -1.0f, 1.0f, 0.0f, 1.0f);
			F32 v1 = Remap(p1xy.y, -1.0f, 1.0f, 0.0f, 1.0f);

			AddVertex(botWs, ColorF32::WHITE, Vec2(0.5f, 0.5f));
			AddVertex(p1BotWs, ColorF32::WHITE, Vec2(u1, v1));
			AddVertex(p0BotWs, ColorF32::WHITE, Vec2(u0, v0));
			AddTriangleFromLast3Vertexes();
		}
	}
}

void MeshBuilder::AddCone(const Vec3& baseCenter, const Vec3& dir, F32 height, F32 baseRadius, U32 resolution)
{
	Vec3 k = dir.Normalized();
	Mat44 localToWorld = Mat44::CreateBasisFromK(k);
	localToWorld.Scale(baseRadius, baseRadius, height);
	localToWorld.Translate(baseCenter);

	F32 thetaDeg = 360.0f / (F32)resolution;

	Vec3 topLocal(0.0f, 0.0f, 1.0f);
	Vec3 botLocal(0.0f, 0.0f, 0.0f);
	Vec3 topWs = localToWorld.TransformPoint(topLocal);
	Vec3 botWs = localToWorld.TransformPoint(botLocal);

	for (U32 slice = 0; slice <= resolution; slice++)
	{
		Vec3 p0(CosDeg(thetaDeg * slice), SinDeg(thetaDeg * slice), 0.0f);
		Vec3 p1(CosDeg(thetaDeg * (slice + 1)), SinDeg(thetaDeg * (slice + 1)), 0.0f);

		Vec3 p0ws = localToWorld.TransformPoint(p0);
		Vec3 p1ws = localToWorld.TransformPoint(p1);

		F32 u0 = Remap(p0.x, -1.0f, 1.0f, 0.0f, 1.0f);
		F32 u1 = Remap(p1.x, -1.0f, 1.0f, 0.0f, 1.0f);

		F32 v0 = Remap(p0.y, -1.0f, 1.0f, 0.0f, 1.0f);
		F32 v1 = Remap(p1.y, -1.0f, 1.0f, 0.0f, 1.0f);

		// top triangle 1
		{
			AddVertex(topWs, ColorF32::WHITE, Vec2(0.5, 0.5f));
			AddVertex(p0ws, ColorF32::WHITE, Vec2(u0, v0));
			AddVertex(p1ws, ColorF32::WHITE, Vec2(u1, v1));
			AddTriangleFromLast3Vertexes();
		}

		// top triangle 2
		{
			AddVertex(topWs, ColorF32::WHITE, Vec2(0.5f, 0.5));
			AddVertex(p0ws, ColorF32::WHITE, Vec2(u0, v0));
			AddVertex(p1ws, ColorF32::WHITE, Vec2(u1, v1));
			AddTriangleFromLast3Vertexes();
		}

		// bottom triangle 1
		{
			AddVertex(botWs, ColorF32::WHITE, Vec2(0.5f, 0.5f));
			AddVertex(p1ws, ColorF32::WHITE, Vec2(u1, v1));
			AddVertex(p0ws, ColorF32::WHITE, Vec2(u0, v0));
			AddTriangleFromLast3Vertexes();
		}

		// bottom triangle 2
		{
			AddVertex(botWs, ColorF32::WHITE, Vec2(0.5f, 0.5f));
			AddVertex(p1ws, ColorF32::WHITE, Vec2(u1, v1));
			AddVertex(p0ws, ColorF32::WHITE, Vec2(u0, v0));
			AddTriangleFromLast3Vertexes();
		}
	}
}

void MeshBuilder::AddArrow(const Vec3& origin, const Vec3& dir, F32 length, F32 bodyRadius, F32 tipRadius)
{
	Vec3 dirNorm = dir.Normalized();

	U32 resolution = 32;

	F32 percentLengthCylinder = 0.8f;

	F32 cylinderLen = percentLengthCylinder * length;
	F32 coneLen = length - cylinderLen;
	Vec3 coneOrigin = origin + (dirNorm * cylinderLen);

	AddCylinder(origin, dirNorm, cylinderLen, bodyRadius, resolution);
	AddCone(coneOrigin, dirNorm, coneLen, tipRadius, resolution);
}

void MeshBuilder::AddArrowWithTipAtPosition(const Vec3& tipPosition, const Vec3& dirFromTipToArrowBody, F32 length, F32 bodyRadius, F32 tipRadius)
{
	Vec3 dirToArrowOriginNorm = dirFromTipToArrowBody.Normalized();
	Vec3 arrowOrigin = tipPosition + (dirToArrowOriginNorm * length);
	AddArrow(arrowOrigin, -dirToArrowOriginNorm, length, bodyRadius, tipRadius);
}

void MeshBuilder::AddFromObj(const char* objFilepath)
{
	CHECK_WORKING_MESH();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::string fullFilepath = std::string(MODELS_PATH) + objFilepath;

	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fullFilepath.c_str());
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

			AddVertexAndIndex(vertex);
		}
	}
}

Mesh* MeshBuilder::BuildFromObj(const char* objFilepath)
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);
	builder.AddFromObj(objFilepath);
	return builder.Finish();
}
