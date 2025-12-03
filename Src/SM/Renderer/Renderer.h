#pragma once

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
            virtual bool Init() = 0;
    };

    Renderer* InitRenderer(RendererType type);
}
