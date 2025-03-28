#include "Engine/Render/Vertex.h"

VertexPCT::VertexPCT(const Vec3& pos, const Vec4& color, const Vec2& uv)
	:m_pos(pos)
	,m_color(color)
	,m_uv(uv)
{
}

VertexPCT::VertexPCT(const Vec3& pos, const ColorF32& color, const Vec2& uv)
	:m_pos(pos)
	,m_color(color.ToVec4())
	,m_uv(uv)
{
}
