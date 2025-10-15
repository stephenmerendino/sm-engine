#pragma once

#include "sm/core/types.h"
#include "sm/render/vulkan/vk_include.h"
#include "sm/render/vulkan/vk_renderer.h"
#include "sm/render/vulkan/vk_context.h"

namespace sm
{
    struct cpu_mesh_t;

    struct buffer_t
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    struct texture_t
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;
        u32 num_mips = 0;
    };

    struct gpu_mesh_t 
    {
        buffer_t vertex_buffer;
        buffer_t index_buffer;
        u32 num_indices = 0;
    };

    struct material_t
    {
        VkDescriptorSet descriptor_sets[(u32)render_pass_t::NUM_RENDER_PASSES];
        VkPipelineLayout pipeline_layouts[(u32)render_pass_t::NUM_RENDER_PASSES];
        VkPipeline pipelines[(u32)render_pass_t::NUM_RENDER_PASSES];
    };

    u32 calculate_num_mips(u32 width, u32 height, u32 depth);
    u32 calculate_num_mips(VkExtent3D size);

    void texture_init(render_context_t& context,
                      texture_t& out_texture,
					  VkFormat format,
					  VkExtent3D size,
					  VkImageUsageFlags usage_flags,
					  VkImageAspectFlags image_aspect,
					  VkSampleCountFlagBits sample_count,
					  bool with_mips_enabled);
    void texture_init_from_file(render_context_t& context,
                                texture_t& out_texture, 
                                const char* filename, 
                                bool generate_mips = true);
    void texture_release(render_context_t& context, texture_t& texture);

    void buffer_init(render_context_t& context,
                     buffer_t& out_buffer,
                     size_t size,
                     VkBufferUsageFlags usage_flags,
                     VkMemoryPropertyFlags memory_flags);
    void buffer_upload_data(render_context_t& context, VkBuffer dst_buffer, void* src_data, size_t src_data_size);
    void buffer_release(render_context_t& context, buffer_t& buffer);

    void gpu_mesh_init(render_context_t& context, const cpu_mesh_t& mesh_data, gpu_mesh_t& out_mesh);

    void material_defaults_init(const render_context_t& context);
    extern VkPipelineVertexInputStateCreateInfo g_default_vertex_input_state;
    extern VkPipelineInputAssemblyStateCreateInfo g_default_triangle_input_assembly;
    extern VkPipelineTessellationStateCreateInfo g_default_no_tesselation_state;
};
