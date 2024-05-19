#include "Engine/Render/ShaderCompiler.h"
#include "Engine/Core/Assert.h"
#include "Engine/Platform/WindowsInclude.h"
#include "Engine/Platform/WindowsUtils.h"
#include "Engine/ThirdParty/dxc/dxcapi.h"

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

bool CompileShader(ShaderType type, const char* filepath, const char* entry, Shader* pOutShader)
{
	HRESULT hres;

	// need to convert filepath from const char * to LPCWSTR
	wchar_t* filepathW = AllocAndConvertUnicodeStr(filepath);

	// Load the HLSL text shader from disk
	uint32_t codePage = DXC_CP_ACP;
	CComPtr<IDxcBlobEncoding> sourceBlob;
	hres = g_utils->LoadFile(filepathW, &codePage, &sourceBlob);
	SM_ASSERT(SUCCEEDED(hres));

	delete filepathW;
	return false;
}