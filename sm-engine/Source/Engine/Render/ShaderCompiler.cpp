#include "Engine/Render/ShaderCompiler.h"
#include "Engine/Config.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug_Old.h"
#include "Engine/Platform/WindowsInclude.h"
#include "Engine/Platform/WindowsUtils.h"
#include "Engine/ThirdParty/dxc/dxcapi.h"

#include <string>
#include <atlbase.h>

CComPtr<IDxcLibrary> g_library;
CComPtr<IDxcCompiler3> g_compiler;
CComPtr<IDxcUtils> g_utils;

void InitShaderCompiler()
{
	// Initialize DXC library
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&g_library))));

	// Initialize DXC compiler
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&g_compiler))));

	// Initialize DXC utility
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&g_utils))));

}

bool CompileShader(ShaderType type, const char* filename, const char* entry, Shader* pOutShader)
{
	HRESULT hres;

	// append on the root shader directory
	std::string fullpath = std::string(SHADERS_PATH) + std::string(filename);

	// need to convert filepath from const char * to LPCWSTR
	wchar_t* filepathW = AllocAndConvertUnicodeStr(fullpath.c_str());
	wchar_t* entryW = AllocAndConvertUnicodeStr(entry);

	// Load the HLSL text shader from disk
	uint32_t codePage = DXC_CP_ACP;
	CComPtr<IDxcBlobEncoding> sourceBlob;
	hres = g_utils->LoadFile(filepathW, &codePage, &sourceBlob);
	SM_ASSERT(SUCCEEDED(hres));

	LPCWSTR targetProfile;
	switch (type)
	{
        case ShaderType::kVs: targetProfile = L"vs_6_6"; break;
        case ShaderType::kPs: targetProfile = L"ps_6_6"; break;
		case ShaderType::kCs: targetProfile = L"cs_6_6"; break;
        default: targetProfile = L"unknown"; break;
	}

	// Configure the compiler arguments for compiling the HLSL shader to SPIR-V
	std::vector<LPCWSTR> arguments = {
		// (Optional) name of the shader file to be displayed e.g. in an error message
		filepathW,
		// Shader main entry point
		L"-E", entryW,
		// Shader target profile
		L"-T", targetProfile,
		// Compile to SPIRV
		L"-spirv"
	};

	// Compile shader
	DxcBuffer buffer{};
	buffer.Encoding = DXC_CP_ACP;
	buffer.Ptr = sourceBlob->GetBufferPointer();
	buffer.Size = sourceBlob->GetBufferSize();

	CComPtr<IDxcResult> result{ nullptr };
	hres = g_compiler->Compile(&buffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(&result));

	if (SUCCEEDED(hres)) {
		result->GetStatus(&hres);
	}

	// Output error if compilation failed
	if (FAILED(hres) && (result)) {
		CComPtr<IDxcBlobEncoding> errorBlob;
		hres = result->GetErrorBuffer(&errorBlob);
		if (SUCCEEDED(hres) && errorBlob) {
			DebugPrintf("Shader compilation failed for %s\n%s\n", filename, (const char*)errorBlob->GetBufferPointer());
            delete entry;
            delete filepathW;
			return false;
		}
	}

	// Get compilation result
	CComPtr<IDxcBlob> code;
	result->GetResult(&code);

	pOutShader->m_filename = filename;
	pOutShader->m_entryName = entry;
	pOutShader->m_type = type;

	pOutShader->m_bytecode.resize(code->GetBufferSize());
	memcpy(pOutShader->m_bytecode.data(), code->GetBufferPointer(), code->GetBufferSize());

	delete entryW;
	delete filepathW;
	return true;
}