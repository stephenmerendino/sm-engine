#pragma once

#include "SM/Platform.h"

namespace SM
{
    enum RendererType
    {
        kVulkan,
        kNumRendererTypes
    };

    struct Renderer
    {
        virtual bool Init(Platform::Window* pWindow) = 0;
    };

    Renderer* CreateRenderer(RendererType type);
}
