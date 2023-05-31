#pragma once

#include "engine/math/vec2.h"
#include "engine/math/vec3.h"
#include "engine/math/vec4.h"

#include <vector>

struct vertex_pct_t
{
   vec3 m_pos;
   vec4 m_color;
   vec2 m_uv;
};

inline
vertex_pct_t make_vertex(const vec3& pos, const vec4& color, const vec2& uv)
{
    vertex_pct_t vertex = { pos, color, uv }; 
    return vertex;
}
