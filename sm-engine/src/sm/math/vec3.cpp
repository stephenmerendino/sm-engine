#include "sm/math/vec3.h"

using namespace sm;

const vec3_t vec3_t::ZERO			{ 0.0f, 0.0f, 0.0f };
const vec3_t vec3_t::WORLD_FORWARD	{ 1.0f, 0.0f, 0.0f };
const vec3_t vec3_t::WORLD_BACKWARD	{ -1.0f, 0.0f, 0.0f };
const vec3_t vec3_t::WORLD_UP		{ 0.0f, 0.0f, 1.0f };
const vec3_t vec3_t::WORLD_DOWN		{ 0.0f, 0.0f, -1.0f };
const vec3_t vec3_t::WORLD_LEFT		{ 0.0f, 1.0f, 0.0f };
const vec3_t vec3_t::WORLD_RIGHT	{ 0.0f, -1.0f, 0.0f };
