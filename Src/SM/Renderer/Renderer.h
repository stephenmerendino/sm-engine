#pragma once

#include "SM/Platform.h"

namespace SM
{
    enum RendererType
    {
        kVulkan,
        kNumRendererTypes
    };

    class Renderer
    {
        public:
        virtual bool Init(Platform::Window* pWindow) = 0;
    };

    Renderer* CreateRenderer(RendererType type);
}
