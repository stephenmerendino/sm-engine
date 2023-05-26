#pragma once

#include "engine/render/vulkan/vulkan_types.h"

std::vector<VkCommandBuffer> command_buffers_allocate(context_t& context, VkCommandBufferLevel level, u32 num_buffers);
void command_buffers_free(context_t& context, std::vector<VkCommandBuffer>& command_buffers);
void command_buffer_begin(VkCommandBuffer command_buffer);
void command_buffer_end(VkCommandBuffer command_buffer);
VkCommandBuffer command_begin_single_time(context_t& context);
void command_end_single_time(context_t& context, VkCommandBuffer command_buffer);

void command_copy_buffer(context_t& context, buffer_t& src, buffer_t& dst, VkDeviceSize size);
void command_generate_mip_maps(context_t& context, VkImage image, VkFormat format, u32 width, u32 height, u32 num_mips);
void command_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, u32 num_mips);
void command_copy_buffer_to_image(context_t& context, VkBuffer buffer, VkImage image, u32 width, u32 height);
void command_buffer_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clear_colors);
void command_buffer_end_render_pass(VkCommandBuffer command_buffer);
void command_draw_mesh_instance(VkCommandBuffer command_buffer, const mesh_instance_t& mesh_instance, const mesh_render_data_t& mesh_render_data, const std::vector<VkDescriptorSet>& descriptor_sets);
void command_copy_image(VkCommandBuffer command_buffer, VkImage src_image, VkImageLayout src_layout, VkImage dst_image, VkImageLayout dst_layout, u32 width, u32 height, u32 depth = 1);
