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

    extern VkDescriptorPool g_global_descriptor_pool;
    extern VkDescriptorPool g_frame_descriptor_pool;
    extern VkDescriptorPool g_material_descriptor_pool;
    extern VkDescriptorPool g_imgui_descriptor_pool;
    extern VkDescriptorSetLayout g_empty_descriptor_set_layout;
    extern VkDescriptorSetLayout g_global_descriptor_set_layout;
    extern VkDescriptorSetLayout g_frame_descriptor_set_layout;
    extern VkDescriptorSetLayout g_material_descriptor_set_layout;
    extern VkDescriptorSetLayout g_mesh_instance_descriptor_set_layout;
    extern VkDescriptorSetLayout g_post_process_descriptor_set_layout;
    extern VkDescriptorSetLayout g_infinite_grid_descriptor_set_layout;
    extern VkDescriptorSet g_empty_descriptor_set;
    extern VkDescriptorSet g_global_descriptor_set;
    extern VkSampler g_linear_sampler;

    extern gpu_mesh_t g_viking_room_mesh;
    extern texture_t g_viking_room_diffuse_texture;
    extern VkPipelineLayout g_infinite_grid_forward_pass_pipeline_layout;
    extern VkPipelineLayout g_post_process_pipeline_layout;
    extern VkPipeline g_post_process_compute_pipeline;

    extern material_t* g_viking_room_material;
    extern material_t* g_debug_draw_material;

    struct debug_draw_push_constants_t
    {
        vec4_t color;
    };

    void material_init(render_context_t& context);
    void pipelines_recreate(render_context_t& context);
};
