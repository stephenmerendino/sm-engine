#pragma once

#include "SM/Renderer/Renderer.h"

namespace SM
{
    class VulkanRenderer : public Renderer
    {
        public:
            virtual bool Init() final;
    };
}
