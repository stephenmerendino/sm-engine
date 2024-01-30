#pragma once

#include "Engine/Core/Color.h"
#include "Engine/Math/Vec2.h"
#include "Engine/Math/Vec3.h"
#include "Engine/Math/Vec4.h"

#include <vector>

enum class VertexType : U8
{
	kPCT,
	kNumTypes
};

class VertexPCT
{
public:
	VertexPCT() = default;
	VertexPCT(const Vec3& pos, const Vec4& color, const Vec2& uv);
	VertexPCT(const Vec3& pos, const ColorF32& color, const Vec2& uv);
	
	Vec3 m_pos;
	Vec4 m_color;
	Vec2 m_uv;
};
