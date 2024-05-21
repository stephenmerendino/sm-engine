#pragma once

#include "Engine/Core/Types.h"

#include <vector>

enum class ShaderType : U8
{
	kVs,
	kPs,
	kCs,
	kNum
};

struct Shader
{
	std::vector<Byte> m_bytecode;
	const char* m_filename;
	const char* m_entryName;
	ShaderType m_type;
};

void InitShaderCompiler();
bool CompileShader(ShaderType type, const char* filepath, const char* entry, Shader* pOutShader);
