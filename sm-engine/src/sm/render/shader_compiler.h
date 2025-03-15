#pragma once

#include "sm/core/array.h"
#include "sm/core/types.h"

namespace sm
{
	enum class shader_type_t : u8
	{
		VERTEX,
		PIXEL,
		COMPUTE,
		NUM_SHADER_TYPES
	};

	struct shader_t
	{
		shader_type_t shader_type;
		array_t<byte_t> bytecode;
		const char* file_name;
		const char* entry_name;
	};

	void init_shader_compiler();
	bool compile_shader(arena_t* arena, shader_type_t shader_type, const char* file_name, const char* entry_name, shader_t** out_shader);
}
