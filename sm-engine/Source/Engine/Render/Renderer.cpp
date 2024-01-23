#include "Engine/Render/Renderer.h"
#include "Engine/Render/Vulkan/VulkanRenderer.h"

Renderer* g_renderer = new VulkanRenderer();
