cmake_minimum_required(VERSION 3.26)

project(sm-engine)

add_library(sm-engine STATIC src/engine/core/assert.cpp
                             src/engine/core/assert.h
                             src/engine/core/bit_flags.h
                             src/engine/core/config.h
                             src/engine/core/debug.cpp
                             src/engine/core/debug.h
                             src/engine/core/file.cpp
                             src/engine/core/file.h
                             src/engine/core/funcs.h
                             src/engine/core/macros.h
                             src/engine/core/time.cpp
                             src/engine/core/time.h
                             src/engine/core/types.h
                             src/engine/input/input.cpp
                             src/engine/input/input.h
                             src/engine/job/job_system.cpp
                             src/engine/job/job_system.h
                             src/engine/math/ivec2.h
                             src/engine/math/mat33.h
                             src/engine/math/mat44.h
                             src/engine/math/math_utils.cpp
                             src/engine/math/math_utils.h
                             src/engine/math/vec2.h
                             src/engine/math/vec3.h
                             src/engine/math/vec4.h
                             src/engine/platform/windows_include.h
                             src/engine/render/camera.cpp
                             src/engine/render/camera.h
                             src/engine/render/mesh.cpp
                             src/engine/render/mesh.h
                             src/engine/render/vertex.cpp
                             src/engine/render/vertex.h
                             src/engine/render/vulkan/vulkan_functions.cpp
                             src/engine/render/vulkan/vulkan_functions.h
                             src/engine/render/vulkan/vulkan_functions_manifest.inl
                             src/engine/render/vulkan/vulkan_include.h
                             src/engine/render/vulkan/vulkan_renderer.cpp
                             src/engine/render/vulkan/vulkan_renderer.h
                             src/engine/render/window.cpp
                             src/engine/render/window.h
                             src/engine/thread/atomic_util.h
                             src/engine/thread/critical_section.cpp
                             src/engine/thread/critical_section.h
                             src/engine/thread/event.cpp
                             src/engine/thread/event.h
                             src/engine/thread/thread.cpp
                             src/engine/thread/thread.h
                             src/engine/thread/thread_safe_queue.h
                             )

target_include_directories(sm-engine PUBLIC src/)
target_link_libraries(sm-engine PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib>")
