cmake_minimum_required(VERSION 3.26)

project(sm-engine)

add_library(sm-engine STATIC src/Engine/Core/Assert.cpp
                             src/Engine/Core/Clock.cpp
                             src/Engine/Core/Common.cpp
                             src/Engine/Core/Debug.cpp
                             src/Engine/Core/FileUtils.cpp
                             src/Engine/Input/InputSystem.cpp
                             src/Engine/Job/Job.cpp
                             src/Engine/Job/JobSystem.cpp
                             src/Engine/Math/IntVec2.cpp
                             src/Engine/Math/Mat33.cpp
                             src/Engine/Math/Mat44.cpp
                             src/Engine/Math/MathUtils.cpp
                             src/Engine/Math/Vec2.cpp
                             src/Engine/Math/Vec3.cpp
                             src/Engine/Math/Vec4.cpp
                             src/Engine/Render/Camera.cpp
                             src/Engine/Render/Mesh.cpp
                             src/Engine/Render/RenderableMesh.cpp
                             src/Engine/Render/Vertex.cpp
                             src/Engine/Render/Window.cpp
                             src/Engine/Render/Vulkan/VulkanBuffer.cpp
                             src/Engine/Render/Vulkan/VulkanCommandPool.cpp
                             src/Engine/Render/Vulkan/VulkanCommands.cpp
                             src/Engine/Render/Vulkan/VulkanDescriptorSet.cpp
                             src/Engine/Render/Vulkan/VulkanDevice.cpp
                             src/Engine/Render/Vulkan/VulkanFunctions.cpp
                             src/Engine/Render/Vulkan/VulkanInstance.cpp
                             src/Engine/Render/Vulkan/VulkanPipeline.cpp
                             src/Engine/Render/Vulkan/VulkanRenderer.cpp
                             src/Engine/Render/Vulkan/VulkanRenderPass.cpp
                             src/Engine/Render/Vulkan/VulkanSampler.cpp
                             src/Engine/Render/Vulkan/VulkanSurface.cpp
                             src/Engine/Render/Vulkan/VulkanSwapchain.cpp
                             src/Engine/Render/Vulkan/VulkanTexture.cpp
                             src/Engine/Render/Vulkan/VulkanUtils.cpp
                             src/Engine/Thread/AtomicInt.cpp
                             src/Engine/Thread/CriticalSection.cpp
                             src/Engine/Thread/Event.cpp
                             src/Engine/Thread/Thread.cpp
                             )

target_include_directories(sm-engine PUBLIC src/)
target_link_libraries(sm-engine PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib>")
