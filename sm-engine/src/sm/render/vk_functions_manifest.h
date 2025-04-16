#if defined _WIN32
#include "sm/platform/win32/win32_include.h"
#include "third_party/vulkan/vulkan_win32.h"
#endif

//---------------------------------------------------------------
#if !defined(VK_EXPORTED_FUNCTION)
#define VK_EXPORTED_FUNCTION(fun)
#endif

VK_EXPORTED_FUNCTION(vkGetInstanceProcAddr)

//---------------------------------------------------------------
#if !defined(VK_GLOBAL_FUNCTION)
#define VK_GLOBAL_FUNCTION(fun)
#endif

VK_GLOBAL_FUNCTION(vkCreateInstance)
VK_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties)
VK_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties)

//---------------------------------------------------------------
#if !defined(VK_INSTANCE_FUNCTION)
#define VK_INSTANCE_FUNCTION(fun)
#endif

VK_INSTANCE_FUNCTION(vkDestroyInstance)
VK_INSTANCE_FUNCTION(vkCreateDebugUtilsMessengerEXT)
VK_INSTANCE_FUNCTION(vkDestroyDebugUtilsMessengerEXT)
VK_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)
VK_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties);
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFormatProperties);
VK_INSTANCE_FUNCTION(vkCreateDevice)
VK_INSTANCE_FUNCTION(vkGetDeviceProcAddr)
VK_INSTANCE_FUNCTION(vkDestroySurfaceKHR)

//---------------------------------------------------------------
#if !defined(VK_DEVICE_FUNCTION)
#define VK_DEVICE_FUNCTION(fun)
#endif

VK_DEVICE_FUNCTION(vkDestroyDevice)
VK_DEVICE_FUNCTION(vkGetDeviceQueue)
VK_DEVICE_FUNCTION(vkCreateSwapchainKHR)
VK_DEVICE_FUNCTION(vkDestroySwapchainKHR)
VK_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)
VK_DEVICE_FUNCTION(vkCreateImageView)
VK_DEVICE_FUNCTION(vkDestroyImageView)
VK_DEVICE_FUNCTION(vkCreateShaderModule);
VK_DEVICE_FUNCTION(vkDestroyShaderModule);
VK_DEVICE_FUNCTION(vkCreatePipelineLayout);
VK_DEVICE_FUNCTION(vkDestroyPipelineLayout);
VK_DEVICE_FUNCTION(vkCreateRenderPass);
VK_DEVICE_FUNCTION(vkCreateRenderPass2);
VK_DEVICE_FUNCTION(vkDestroyRenderPass);
VK_DEVICE_FUNCTION(vkCreateGraphicsPipelines);
VK_DEVICE_FUNCTION(vkCreateComputePipelines);
VK_DEVICE_FUNCTION(vkDestroyPipeline);
VK_DEVICE_FUNCTION(vkCreateFramebuffer);
VK_DEVICE_FUNCTION(vkDestroyFramebuffer);
VK_DEVICE_FUNCTION(vkCreateCommandPool);
VK_DEVICE_FUNCTION(vkDestroyCommandPool);
VK_DEVICE_FUNCTION(vkAllocateCommandBuffers);
VK_DEVICE_FUNCTION(vkCmdBeginRenderPass);
VK_DEVICE_FUNCTION(vkBeginCommandBuffer);
VK_DEVICE_FUNCTION(vkResetCommandBuffer);
VK_DEVICE_FUNCTION(vkEndCommandBuffer);
VK_DEVICE_FUNCTION(vkCmdBindPipeline);
VK_DEVICE_FUNCTION(vkCmdDraw);
VK_DEVICE_FUNCTION(vkCmdEndRenderPass);
VK_DEVICE_FUNCTION(vkCreateSemaphore);
VK_DEVICE_FUNCTION(vkDestroySemaphore);
VK_DEVICE_FUNCTION(vkAcquireNextImageKHR);
VK_DEVICE_FUNCTION(vkQueueSubmit);
VK_DEVICE_FUNCTION(vkQueuePresentKHR);
VK_DEVICE_FUNCTION(vkDeviceWaitIdle);
VK_DEVICE_FUNCTION(vkCreateFence);
VK_DEVICE_FUNCTION(vkDestroyFence);
VK_DEVICE_FUNCTION(vkWaitForFences);
VK_DEVICE_FUNCTION(vkResetFences);
VK_DEVICE_FUNCTION(vkFreeCommandBuffers);
VK_DEVICE_FUNCTION(vkCreateBuffer);
VK_DEVICE_FUNCTION(vkDestroyBuffer);
VK_DEVICE_FUNCTION(vkGetBufferMemoryRequirements);
VK_DEVICE_FUNCTION(vkAllocateMemory);
VK_DEVICE_FUNCTION(vkFreeMemory);
VK_DEVICE_FUNCTION(vkBindBufferMemory);
VK_DEVICE_FUNCTION(vkMapMemory);
VK_DEVICE_FUNCTION(vkUnmapMemory);
VK_DEVICE_FUNCTION(vkCmdBindVertexBuffers);
VK_DEVICE_FUNCTION(vkCmdCopyBuffer);
VK_DEVICE_FUNCTION(vkCmdCopyImage);
VK_DEVICE_FUNCTION(vkQueueWaitIdle);
VK_DEVICE_FUNCTION(vkCmdBindIndexBuffer);
VK_DEVICE_FUNCTION(vkCmdDrawIndexed);
VK_DEVICE_FUNCTION(vkCreateDescriptorSetLayout);
VK_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout);
VK_DEVICE_FUNCTION(vkCreateDescriptorPool);
VK_DEVICE_FUNCTION(vkDestroyDescriptorPool);
VK_DEVICE_FUNCTION(vkResetDescriptorPool);
VK_DEVICE_FUNCTION(vkAllocateDescriptorSets);
VK_DEVICE_FUNCTION(vkUpdateDescriptorSets);
VK_DEVICE_FUNCTION(vkCmdBindDescriptorSets);
VK_DEVICE_FUNCTION(vkCreateImage);
VK_DEVICE_FUNCTION(vkGetImageMemoryRequirements);
VK_DEVICE_FUNCTION(vkBindImageMemory);
VK_DEVICE_FUNCTION(vkCmdPipelineBarrier);
VK_DEVICE_FUNCTION(vkCmdCopyBufferToImage);
VK_DEVICE_FUNCTION(vkDestroyImage);
VK_DEVICE_FUNCTION(vkCreateSampler);
VK_DEVICE_FUNCTION(vkDestroySampler);
VK_DEVICE_FUNCTION(vkCmdBlitImage);
VK_DEVICE_FUNCTION(vkSetDebugUtilsObjectNameEXT)
VK_DEVICE_FUNCTION(vkQueueBeginDebugUtilsLabelEXT)
VK_DEVICE_FUNCTION(vkQueueEndDebugUtilsLabelEXT)
VK_DEVICE_FUNCTION(vkQueueInsertDebugUtilsLabelEXT)
VK_DEVICE_FUNCTION(vkCmdBeginDebugUtilsLabelEXT)
VK_DEVICE_FUNCTION(vkCmdEndDebugUtilsLabelEXT)
VK_DEVICE_FUNCTION(vkCmdInsertDebugUtilsLabelEXT)
VK_DEVICE_FUNCTION(vkCmdDispatch)
VK_DEVICE_FUNCTION(vkCmdBeginRendering)
VK_DEVICE_FUNCTION(vkCmdEndRendering)

#if defined _WIN32
#include "sm/platform/win32/render/win32_vk_functions_manifest.h"
#endif

#undef VK_EXPORTED_FUNCTION
#undef VK_GLOBAL_FUNCTION
#undef VK_INSTANCE_FUNCTION
#undef VK_DEVICE_FUNCTION
