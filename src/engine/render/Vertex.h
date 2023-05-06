#pragma once

#include "Engine/Core/Common.h"
#include "Engine/Math/Vec2.h"
#include "Engine/Math/Vec3.h"
#include "Engine/Math/Vec4.h"
#include "Engine/Render/Vulkan/VulkanFunctions.h"

#include <vector>

class Vertex
{
public:
	Vec3 m_pos;
	Vec4 m_color;
	Vec2 m_texCoord;

	Vertex(){};
	inline Vertex(const Vec3& pos, const Vec4& color, const Vec2& texCoord);

	inline bool operator==(const Vertex& other) const;
};

inline 
Vertex::Vertex(const Vec3& pos, const Vec4& color, const Vec2& texCoord)
	:m_pos(pos)
	,m_color(color)
	,m_texCoord(texCoord)
{
}

inline
bool Vertex::operator==(const Vertex& other) const
{
	return m_pos == other.m_pos && m_texCoord == other.m_texCoord && m_color == other.m_color;
}

VkVertexInputBindingDescription GetBindingDescription(const Vertex& vtx);
std::vector<VkVertexInputAttributeDescription> GetAttributeDescs(const Vertex& vtx);