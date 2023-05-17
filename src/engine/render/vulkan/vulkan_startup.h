#pragma once

#include "engine/render/vulkan/vulkan_types.h"

context_t* context_create(window_t* window);
void context_destroy(context_t* context);
void context_refresh_swapchain(context_t& context);
