//#include "SM/Renderer/Renderer.h"
//#include "SM/Renderer/VulkanRenderer.h"
//#include "SM/Memory.h"
//#include <new>
//
//using namespace SM;
//
//Renderer* SM::CreateRenderer(RendererType type)
//{
//    switch(type)
//    {
//        case kVulkan: 
//        {
//            SM::PushAllocator(kEngineGlobal);
//            void* rendererMemory = SM::Alloc<VulkanRenderer>();
//            VulkanRenderer* vulkanRenderer = new (rendererMemory) VulkanRenderer;
//            SM::PopAllocator();
//            return vulkanRenderer;
//        }
//
//        default: return nullptr;
//    }
//}
