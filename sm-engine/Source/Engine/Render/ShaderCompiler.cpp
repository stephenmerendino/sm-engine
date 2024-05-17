#include "Engine/Render/ShaderCompiler.h"
#include "Engine/Core/Assert.h"
#include "Engine/Platform/WindowsInclude.h"
#include "Engine/ThirdParty/dxc/dxcapi.h"

#include <atlbase.h>

void InitShaderCompiler()
{
	HRESULT hres;

	// Initialize DXC library
	CComPtr<IDxcLibrary> library;
	hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	SM_ASSERT(SUCCEEDED(hres));
}

bool CompileShader(ShaderType type, const char* filepath, const char* entry, Shader* pOutShader)
{
	return false;
}