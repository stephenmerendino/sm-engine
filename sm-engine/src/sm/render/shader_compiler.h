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

	void shader_compiler_init();
	bool shader_compiler_compile(arena_t* arena, shader_type_t shader_type, const char* file_name, const char* entry_name, shader_t** out_shader);
}
