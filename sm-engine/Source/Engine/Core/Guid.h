#pragma once

#include "Engine/Core/Assert_old.h"
#include "Engine/Core/Types.h"
#include "Engine/Platform/WindowsInclude.h"

typedef GUID Guid;

inline Guid GuidGenerate()
{
	GUID guid;
	HRESULT res = ::CoCreateGuid(&guid);
	SM_ASSERT(SUCCEEDED(res));
	return guid;
}
