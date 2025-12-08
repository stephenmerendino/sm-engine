#pragma once

#include "SM/StandardTypes.h"

namespace SM
{
    enum ShaderType
    {
        kVertex,
        kPixel,
        kCompute,
        kNumShaderTypes
    };

    struct Shader
    {
        const char* m_fileName = nullptr; 
        const char* m_entryFunctionName = nullptr;
        Byte* m_byteCode = nullptr;
        ShaderType m_type;
    };
}
