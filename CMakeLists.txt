cmake_minimum_required(VERSION 3.26)

project(sm-engine)

add_library(sm-engine STATIC src/engine/core/assert.cpp
                             src/engine/core/bit_flags.h
                             src/engine/core/debug.cpp
                             src/engine/core/file.cpp
                             src/engine/core/time.cpp
                             src/engine/input/input.cpp
                             src/engine/job/job_system.cpp
                             src/engine/math/math_utils.cpp
                             #src/engine/render/Camera.cpp
                             #src/engine/render/Mesh.cpp
                             #src/engine/render/Mesh.h
                             #src/engine/render/RenderableMesh.cpp
                             #src/engine/render/RenderableMesh.h
                             #src/engine/render/Vertex.cpp
                             #src/engine/render/Vertex.h
                             src/engine/render/window.cpp
                             src/engine/render/window.h
                             #src/engine/render/vulkan/VulkanBuffer.cpp
                             #src/engine/render/vulkan/VulkanBuffer.h
                             #src/engine/render/vulkan/VulkanCommandPool.cpp
                             #src/engine/render/vulkan/VulkanCommandPool.h
                             #src/engine/render/vulkan/VulkanCommands.cpp
                             #src/engine/render/vulkan/VulkanCommands.h
                             #src/engine/render/vulkan/VulkanDescriptorSet.cpp
                             #src/engine/render/vulkan/VulkanDescriptorSet.h
                             #src/engine/render/vulkan/VulkanDevice.cpp
                             #src/engine/render/vulkan/VulkanDevice.h
                             #src/engine/render/vulkan/VulkanFunctions.cpp
                             #src/engine/render/vulkan/VulkanFunctions.h
                             #src/engine/render/vulkan/VulkanInclude.h
                             #src/engine/render/vulkan/VulkanInstance.cpp
                             #src/engine/render/vulkan/VulkanInstance.h
                             #src/engine/render/vulkan/VulkanPipeline.cpp
                             #src/engine/render/vulkan/VulkanPipeline.h
                             #src/engine/render/vulkan/VulkanRenderer.cpp
                             #src/engine/render/vulkan/VulkanRenderer.h
                             #src/engine/render/vulkan/VulkanRenderer2.cpp
                             #src/engine/render/vulkan/VulkanRenderer2.h
                             #src/engine/render/vulkan/VulkanRenderPass.cpp
                             #src/engine/render/vulkan/VulkanRenderPass.h
                             #src/engine/render/vulkan/VulkanSampler.cpp
                             #src/engine/render/vulkan/VulkanSampler.h
                             #src/engine/render/vulkan/VulkanSurface.cpp
                             #src/engine/render/vulkan/VulkanSurface.h
                             #src/engine/render/vulkan/VulkanSwapchain.cpp
                             #src/engine/render/vulkan/VulkanSwapchain.h
                             #src/engine/render/vulkan/VulkanTexture.cpp
                             #src/engine/render/vulkan/VulkanTexture.h
                             #src/engine/render/vulkan/VulkanUtils.cpp
                             #src/engine/render/vulkan/VulkanUtils.h
                             src/engine/thread/critical_section.cpp
                             src/engine/thread/event.cpp
                             src/engine/thread/thread.cpp
                             )

target_include_directories(sm-engine PUBLIC src/)
target_link_libraries(sm-engine PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib>")
