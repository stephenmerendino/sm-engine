#include "Engine/Render/Mesh.h"
#include "Engine/Render/MeshBuilder.h"

static Mesh* BuildUnitAxes()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kLineList);
	builder.AddAxesLines3d(Vec3::ZERO, Vec3::FORWARD, Vec3::LEFT, Vec3::UP);
	return builder.Finish();
}

static Mesh* BuildUnitTetrahedron()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);

	Vec3 v0_pos(0.0f, 0.0f, 1.0f);

	F32 pitch = 0.0f;
	Vec3 v1_pos = (Vec4(1.0f, 0.0f, 0.0f, 0.0f) * Mat44::CreateRotationYDegs(pitch)).xyz;
	Vec3 v2_pos = (Vec4(1.0f, 0.0f, 0.0f, 0.0f) * Mat44::CreateRotationYDegs(pitch) * Mat44::CreateRotationZDegs(120.0f)).xyz;
	Vec3 v3_pos = (Vec4(1.0f, 0.0f, 0.0f, 0.0f) * Mat44::CreateRotationYDegs(pitch) * Mat44::CreateRotationZDegs(240.0f)).xyz;

	v1_pos.Normalize();
	v2_pos.Normalize();
	v3_pos.Normalize();

	U32 v0_index  = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
	U32 v1_index  = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
	U32 v2_index  = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

	U32 v3_index  = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
	U32 v4_index  = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
	U32 v5_index  = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

	U32 v6_index  = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
	U32 v7_index  = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
	U32 v8_index  = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

	U32 v9_index  = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
	U32 v10_index = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
	U32 v11_index = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));

	builder.AddTriangle(v0_index, v1_index, v2_index);
	builder.AddTriangle(v3_index, v4_index, v5_index);
	builder.AddTriangle(v6_index, v7_index, v8_index);
	builder.AddTriangle(v9_index, v10_index, v11_index);

	return builder.Finish();
}

static Mesh* BuldUnitCube()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);
	builder.AddCube(Vec3::ZERO, 1.0f);
	return builder.Finish();
}

static Mesh* BuildUnitOctahedron()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);

	Vec3 v0_pos = Vec3(0.0f, 0.0f, 1.0f);
	Vec3 v1_pos = Vec3(1.0f, 0.0f, 0.0f);
	Vec3 v2_pos = Vec3(0.0f, 1.0f, 0.0f);
	Vec3 v3_pos = Vec3(-1.0f, 0.0f, 0.0f);
	Vec3 v4_pos = Vec3(0.0f, -1.0f, 0.0f);
	Vec3 v5_pos = Vec3(0.0f, 0.0f, -1.0f);

	{
		U32 i0 = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		U32 i2 = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	{
		U32 i0 = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		U32 i2 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	{
		U32 i0 = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		U32 i2 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	{
		U32 i0 = builder.AddVertex(v0_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		U32 i2 = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	{
		U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		U32 i2 = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	{
		U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		U32 i2 = builder.AddVertex(v2_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	{
		U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		U32 i2 = builder.AddVertex(v3_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	{
		U32 i0 = builder.AddVertex(v5_pos, ColorF32::WHITE, Vec2(0.0f, 0.0f));
		U32 i1 = builder.AddVertex(v1_pos, ColorF32::WHITE, Vec2(1.0f, 0.0f));
		U32 i2 = builder.AddVertex(v4_pos, ColorF32::WHITE, Vec2(0.0f, 1.0f));
		builder.AddTriangle(i0, i1, i2);
	}

	return builder.Finish();
}

static Mesh* BuildUnitUvSphere()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);
	builder.AddUvSphere(Vec3::ZERO, 1.0f, 64);
	return builder.Finish();
}

static Mesh* BuildUnitPlane()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);
	builder.AddQuad3d(Vec3::ZERO, Vec3::RIGHT, Vec3::FORWARD, 1.0f, 1.0f, 1);
	return builder.Finish();
}

static Mesh* BuildUnitQuad()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);
	// build a quad that is already in ndc space that takes up the entire screen
	builder.AddQuad3d(Vec3::ZERO, Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), 1.0f, 1.0f, 1);
	return builder.Finish();
}

static Mesh* BuildUnitCone()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);
	builder.AddCone(Vec3::ZERO, Vec3::UP, 1.0f, 1.0f, 32);
	return builder.Finish();
}

static Mesh* BuildUnitCylinder()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);
	builder.AddCylinder(Vec3::ZERO, Vec3::UP, 1.0f, 1.0f, 32);
	return builder.Finish();
}

static Mesh* BuildUnitTorus()
{
	MeshBuilder builder;
	builder.Begin();
	builder.SetTopology(PrimitiveTopology::kTriangleList);

	U32 resolution = 128;
	F32 uScale = 3.0f;
	F32 vScale = 8.0f;

	for (U32 torusSlice = 0; torusSlice <= resolution; torusSlice++)
	{
		F32 percentAroundTorus = (F32)torusSlice / (F32)resolution;
		F32 curDeg = percentAroundTorus * 360.0f;
		Vec3 ringAnchorPos = Vec3(CosDeg(curDeg), SinDeg(curDeg), 0.0f);

		Vec3 i = ringAnchorPos.Normalized();
		Vec3 k = Cross(Vec3::UP, i).Normalized();
		Vec3 j = Cross(i, k);
		Mat44 ringWorldTransform = Mat44(i, j, k, ringAnchorPos);

		for (U32 torusRingPos = 0; torusRingPos <= resolution; torusRingPos++)
		{
			F32 percentAroundRing = (F32)torusRingPos / (F32)resolution;
			F32 deg = percentAroundRing * 360.0f;
			Vec2 ringPosLs = PolarToCartesianDegs(deg, 0.3f);
			Vec3 ringPosWs = ringWorldTransform.TransformPoint(Vec3(ringPosLs, 0.0f));
			U32 vertIndex = builder.AddVertex(ringPosWs, ColorF32::WHITE, Vec2(percentAroundRing * uScale, percentAroundTorus * vScale));

			if (torusSlice > 0 && torusRingPos > 0)
			{
				U32 prevVertIndex = vertIndex - 1;
				U32 vertIndexLastRing = (vertIndex - resolution) - 1;
				U32 prevVertIndexLastRing = vertIndexLastRing - 1;

				builder.AddTriangle(vertIndex, prevVertIndexLastRing, prevVertIndex);
				builder.AddTriangle(vertIndex, vertIndexLastRing, prevVertIndexLastRing);
			}
		}
	}

	return builder.Finish();
}

static Mesh* s_axesPrimitive = nullptr;
static Mesh* s_tetrahedronPrimitive = nullptr;
static Mesh* s_hexahedronPrimitive = nullptr;
static Mesh* s_octahedronPrimitive = nullptr;
static Mesh* s_uvSpherePrimitive = nullptr;
static Mesh* s_planePrimitive = nullptr;
static Mesh* s_quadPrimitive = nullptr;
static Mesh* s_conePrimitive = nullptr;
static Mesh* s_cylinderPrimitive = nullptr;
static Mesh* s_torusPrimitive = nullptr;

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

void Mesh::InitPrimitives()
{
	s_axesPrimitive = BuildUnitAxes();
	s_tetrahedronPrimitive = BuildUnitTetrahedron();
	s_hexahedronPrimitive = BuldUnitCube();
	s_octahedronPrimitive = BuildUnitOctahedron();
	s_uvSpherePrimitive = BuildUnitUvSphere();
	s_planePrimitive = BuildUnitPlane();
	s_quadPrimitive = BuildUnitQuad();
	s_conePrimitive = BuildUnitCone();
	s_cylinderPrimitive = BuildUnitCylinder();
	s_torusPrimitive = BuildUnitTorus();
}

const Mesh* Mesh::GetPrimitive(PrimitiveMeshType primitive)
{
	switch (primitive)
	{
		case kAxes: return s_axesPrimitive;
		case kTetrahedron: return s_tetrahedronPrimitive;
		case kHexahedron: return s_hexahedronPrimitive;
		case kOctahedron: return s_octahedronPrimitive;
		case kUvSphere: return s_uvSpherePrimitive;
		case kPlane: return s_planePrimitive;
		case kQuad: return s_quadPrimitive;
		case kCone: return s_conePrimitive;
		case kCylinder: return s_cylinderPrimitive;
		case kTorus: return s_torusPrimitive;
		default: SM_ERROR_MSG("Tried using unimplemented primitive mesh type\n");
	}

	return nullptr;
}
