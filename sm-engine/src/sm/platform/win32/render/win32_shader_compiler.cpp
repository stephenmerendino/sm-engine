#include "sm/render/shader_compiler.h"
#include "sm/core/assert.h"
#include "sm/core/debug.h"
#include "sm/core/string.h"
#include "sm/config.h"

#include "sm/platform/win32/win32_include.h"
#include "third_party/dxc/dxcapi.h"
#include <atlbase.h>

using namespace sm;

CComPtr<IDxcLibrary> s_library;
CComPtr<IDxcCompiler3> s_compiler;
CComPtr<IDxcUtils> s_utils;

void sm::shader_compiler_init()
{
	// Initialize DXC library
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&s_library))));

	// Initialize DXC compiler
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_compiler))));

	// Initialize DXC utility
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_utils))));
}

bool sm::shader_compiler_compile(arena_t* arena, shader_type_t shader_type, const char* file_name, const char* entry_name, shader_t** out_shader)
{
	HRESULT hres;

	// append on the root shader directory
	// todo: move this out of platform specific code
	string_t full_filepath = string_init(arena);
	string_append(full_filepath, SHADERS_PATH);
	string_append(full_filepath, file_name);

	// need to convert filepath from const char * to LPCWSTR
	wchar_t* full_filepath_w = string_to_wchar(arena, full_filepath);
	wchar_t* entry_name_w = string_to_wchar(arena, entry_name);

	// Load the HLSL text shader from disk
	uint32_t code_page = DXC_CP_ACP;
	CComPtr<IDxcBlobEncoding> source_blob;
	hres = s_utils->LoadFile(full_filepath_w, &code_page, &source_blob);
	SM_ASSERT(SUCCEEDED(hres));

	LPCWSTR target_profile;
	switch (shader_type)
	{
        case shader_type_t::VERTEX: target_profile = L"vs_6_6"; break;
        case shader_type_t::PIXEL: target_profile = L"ps_6_6"; break;
		case shader_type_t::COMPUTE: target_profile = L"cs_6_6"; break;
        default: target_profile = L"unknown"; break;
	}

	// configure the compiler arguments for compiling the HLSL shader to SPIR-V
	array_t<LPCWSTR> arguments = array_init<LPCWSTR>(arena, 8);
	array_push(arguments, (LPCWSTR)full_filepath_w);
	array_push(arguments, L"-E");
	array_push(arguments, (LPCWSTR)entry_name_w);
	array_push(arguments, L"-T");
	array_push(arguments, target_profile);
	array_push(arguments, L"-spirv");

	if(is_running_in_debug())
	{
        array_push(arguments, L"-Zi");
        array_push(arguments, L"-Od");
	}

	// Compile shader
	DxcBuffer buffer{};
	buffer.Encoding = DXC_CP_ACP;
	buffer.Ptr = source_blob->GetBufferPointer();
	buffer.Size = source_blob->GetBufferSize();

	CComPtr<IDxcResult> result{ nullptr };
	hres = s_compiler->Compile(&buffer, arguments.data, (uint32_t)arguments.cur_size, nullptr, IID_PPV_ARGS(&result));

	if (SUCCEEDED(hres)) 
	{
		result->GetStatus(&hres);
	}

	// Output error if compilation failed
	if (FAILED(hres) && (result)) 
	{
		CComPtr<IDxcBlobEncoding> error_blob;
		hres = result->GetErrorBuffer(&error_blob);
		if (SUCCEEDED(hres) && error_blob) 
		{
			debug_printf("Shader compilation failed for %s\n%s\n", file_name, (const char*)error_blob->GetBufferPointer());
			return false;
		}
	}

	// Get compilation result
	CComPtr<IDxcBlob> code;
	result->GetResult(&code);

	(*out_shader)->file_name = file_name;
	(*out_shader)->entry_name = entry_name;
	(*out_shader)->shader_type = shader_type;

	(*out_shader)->bytecode = array_init<byte_t>(arena, code->GetBufferSize());
	array_push((*out_shader)->bytecode, (byte_t*)code->GetBufferPointer(), code->GetBufferSize());

	return true;
}