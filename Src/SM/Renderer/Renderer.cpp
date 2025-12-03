#include "SM/Renderer/Renderer.h"
#include "SM/Renderer/VulkanRenderer.h"
#include "SM/Memory.h"

using namespace SM;

Renderer* SM::InitRenderer(RendererType type)
{
    switch(type)
    {
        case kVulkan: 
        {
            return SM::Alloc<VulkanRenderer>(kEngineGlobal);
        }

        default: return nullptr;
    }
}
