#pragma once

#include "engine/render/vulkan/vulkan_types.h"

// globals
void renderer_globals_create(context_t& context);
void renderer_globals_destroy(context_t& context);
renderer_globals_t* renderer_globals_get();

// textures/images
VkImageView image_view_create(device_t& device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips);
VkImageView image_view_create(context_t& context, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips);
void image_create(context_t& context, u32 width, u32 height, u32 num_mips, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags mem_props, VkImage& out_image, VkDeviceMemory& out_memory);
texture_t texture_create_from_file(context_t& context, const char* filepath);
texture_t texture_create_color_target(context_t& context, VkFormat format, u32 width, u32 height, VkImageUsageFlags usage, VkSampleCountFlagBits num_samples);
texture_t texture_create_depth_target(context_t& context, VkFormat format, u32 width, u32 height, VkImageUsageFlags usage, VkSampleCountFlagBits num_samples);
void texture_destroy(context_t& context, texture_t& texture);

// buffers
void buffer_create(context_t& context, VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags, VkBuffer& out_buffer, VkDeviceMemory& out_memory);
buffer_t buffer_create(context_t& context, BufferType type, size_t size);
void buffer_destroy(context_t& context, VkBuffer vk_buffer, VkDeviceMemory vk_memory);
void buffer_destroy(context_t& context, buffer_t& buffer);
void buffer_update(context_t& context, buffer_t& buffer, command_pool_t& command_pool, void* data, VkDeviceSize gpu_memory_offset = 0);
void buffer_update_data(context_t& context, buffer_t& buffer, void* data, VkDeviceSize gpu_memory_offset = 0);

// samplers
sampler_t sampler_create(context_t& context, u32 max_mip_level);
void sampler_destroy(context_t& context, sampler_t& sampler);

// descriptor sets / pools
descriptor_set_layout_t descriptor_set_layout_create(context_t& context, descriptor_set_layout_bindings_t& bindings);
void descriptor_set_layout_destroy(context_t& context, descriptor_set_layout_t& layout);
void descriptor_set_layout_add_binding(descriptor_set_layout_bindings_t& bindings, u32 binding_index, u32 descriptor_count, VkDescriptorType descriptor_type, VkShaderStageFlagBits shader_stages);

descriptor_pool_t descriptor_pool_create(context_t& context, descriptor_pool_sizes_t& pool_sizes, u32 max_sets);
void descriptor_pool_destroy(context_t& context, descriptor_pool_t& descriptor_pool);
void descriptor_pool_reset(context_t& context, descriptor_pool_t& descriptor_pool, VkDescriptorPoolResetFlags flags = 0);
void descriptor_pool_add_size(descriptor_pool_sizes_t& pool_sizes, VkDescriptorType type, u32 count);

std::vector<descriptor_set_t> descriptor_sets_allocate(context_t& context, descriptor_pool_t& descriptor_pool, std::vector<descriptor_set_layout_t>& layouts);
descriptor_set_t descriptor_set_allocate(context_t& context, descriptor_pool_t& descriptor_pool, descriptor_set_layout_t& layout);

void descriptor_sets_writes_add_uniform_buffer(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, buffer_t& buffer, u32 buffer_offset, u32 dst_binding, u32 dst_array_element, u32 descriptor_count);
void descriptor_sets_writes_add_combined_image_sampler(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, texture_t& texture, sampler_t& sampler, VkImageLayout image_layout, u32 dst_binding, u32 dst_array_element, u32 descriptor_count);
void descriptor_sets_writes_add_sampler(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, sampler_t& sampler, u32 dst_binding, u32 dst_array_element, u32 descriptor_count);
void descriptor_sets_writes_add_sampled_image(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, texture_t& texture, VkImageLayout image_layout, u32 dst_binding, u32 dst_array_element, u32 descriptor_count);
void descriptor_sets_write(context_t& context, descriptor_sets_writes_t& descriptor_sets_writes);

// shaders
VkShaderModule shader_module_create(context_t& context, const char* shader_filepath);
void shader_module_destroy(context_t& context, VkShaderModule shader);

// render passes
render_pass_t render_pass_create(context_t& context, render_pass_attachments_t& attachments, subpasses_t& subpasses, subpass_dependencies_t& subpass_dependencies);
void render_pass_destroy(context_t& context, render_pass_t& render_pass);
VkAttachmentDescription attachment_description_create(VkFormat format, VkSampleCountFlagBits sample_count, 
                                                      VkImageLayout initial_layout, VkImageLayout final_layout,
                                                      VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, 
                                                      VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, 
                                                      VkAttachmentDescriptionFlags flags = 0);
void render_pass_add_attachment(render_pass_attachments_t& attachments, VkFormat format, VkSampleCountFlagBits sample_count, 
                                                                        VkImageLayout initial_layout, VkImageLayout final_layout,
                                                                        VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, 
                                                                        VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, 
                                                                        VkAttachmentDescriptionFlags flags = 0);
void subpasses_add_attachment_ref(subpasses_t& subpasses, u32 subpass_index, SubpassAttachmentRefType type, u32 attachment, VkImageLayout layout);
void subpass_add_dependency(subpass_dependencies_t& subpass_dependencies, u32 src_subpass,
                                                                          u32 dst_subpass,
                                                                          VkPipelineStageFlags src_stage,
                                                                          VkPipelineStageFlags dst_stage,
                                                                          VkAccessFlags src_access,
                                                                          VkAccessFlags dst_access,
                                                                          VkDependencyFlags dependency_flags = 0);

// pipelines
pipeline_shader_stages_t pipeline_create_shader_stages(context_t& context, const char* vert_shader_file, const char* vert_shader_entry, const char* frag_shader_file, const char* frag_shader_entry);
pipeline_shader_stages_t pipeline_create_shader_stages(context_t& context, shader_info_t& vertex_shader_info, shader_info_t& fragment_shader_info);
pipeline_input_assembly_t pipeline_create_input_assembly(VkPrimitiveTopology topology, bool primitive_restart_enable = false);
pipeline_raster_state_t pipeline_create_raster_state(VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL, 
                                                     VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE, 
                                                     VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT, 
                                                     bool raster_discard_enable = false, 
                                                     bool depth_clamp_enable = false, 
                                                     bool depth_bias_enable = false,
                                                     f32 depth_bias_constant = 0.0f,
                                                     f32 depth_bias_clamp = 0.0f,
                                                     f32 depth_bias_slope = 0.0f,
                                                     f32 line_width = 1.0f);
void pipeline_create_viewport_state(pipeline_viewport_state_t& viewport_state, f32 x, f32 y, 
                                                                               f32 w, f32 h, 
                                                                               f32 min_depth, f32 max_depth, 
                                                                               i32 scissor_offset_x, i32 scissor_offset_y, 
                                                                               u32 scissor_extent_x, u32 scissor_extent_y);
pipeline_multisample_state_t pipeline_create_multisample_state(VkSampleCountFlagBits sample_count, bool sample_shading_enable = false, f32 min_sample_shading = 1.0f);
pipeline_depth_stencil_state_t pipeline_create_depth_stencil_state(bool depth_test_enable, bool depth_write_enable, VkCompareOp depth_compare_op, 
                                                                   bool depth_bounds_test_enable, f32 min_depth_bounds, f32 max_depth_bounds);
void pipeline_add_color_blend_attachments(pipeline_color_blend_state_t& color_blend_state, bool blend_enable, 
                                                                                           VkBlendFactor src_color_blend_factor, VkBlendFactor dst_color_blend_factor, VkBlendOp color_blend_op, 
                                                                                           VkBlendFactor src_alpha_blend_Factor, VkBlendFactor dst_alpha_blend_factor, VkBlendOp alpha_blend_op, 
                                                                                           VkColorComponentFlags color_write_mask);
void pipeline_create_color_blend_state(pipeline_color_blend_state_t& color_blend_state, bool logic_op_enable, VkLogicOp logic_op, f32 blend_constant_0, f32 blend_constant_1, f32 blend_constant_2, f32 blend_constant_3);
pipeline_layout_t pipeline_create_layout(context_t& context, mesh_instance_t& mesh_instance);
pipeline_t pipeline_create(context_t& context, pipeline_shader_stages_t& shader_stages, 
                                               pipeline_vertex_input_t& vertex_input, 
                                               pipeline_input_assembly_t& input_assembly,
                                               pipeline_raster_state_t& raster_state,
                                               pipeline_viewport_state_t& viewport_state,
                                               pipeline_multisample_state_t& multisample_state,
                                               pipeline_depth_stencil_state_t& depth_stencil_state,
                                               pipeline_color_blend_state_t& color_blend_state,
                                               pipeline_layout_t& pipeline_layout,
                                               render_pass_t& render_pass);
void pipeline_destroy_shader_stages(context_t& context, pipeline_shader_stages_t& shader_stages);
void pipeline_destroy(context_t& context, pipeline_t& pipeline);

// framebuffers
framebuffer_t framebuffer_create(context_t& context, render_pass_t& render_pass, const std::vector<VkImageView> attachments, u32 width, u32 height, u32 layers);
void framebuffer_destroy(context_t& context, framebuffer_t& framebuffer);

// synchronization
semaphore_t semaphore_create(context_t& context);
void semaphore_destroy(context_t& context, semaphore_t& semaphore);
fence_t fence_create(context_t& context);
void fence_reset(context_t& context, fence_t& fence);
void fence_wait(context_t& context, fence_t& fence, u64 timeout = UINT64_MAX);
void fence_destroy(context_t& context, fence_t& fence);

// TODO: move these somewhere else, these are renderer concepts, not vulkan concepts
// materials
material_t material_create(context_t& context, material_create_info_t& create_info);
void material_destroy(context_t& context, material_t& material);
material_resource_t material_load_sampled_texture_resource(context_t& context, const char* texture_filepath, u32 binding_index, u32 count, VkShaderStageFlagBits shader_stages);

// mesh instances
mesh_instance_t mesh_instance_create(context_t& context, mesh_id_t mesh_id, material_t& material);
void mesh_instance_destroy(context_t& context, mesh_instance_t& mesh_instance);
void mesh_instance_pipeline_create(context_t& context, mesh_instance_t& mesh_instance);
void mesh_instance_pipeline_refresh(context_t& context, mesh_instance_t& mesh_instance);

// frames
frame_t frame_create(context_t& context);
void frame_destroy(context_t& context, frame_t& frame);
void frame_update_render_data(context_t& context, frame_t& frame);
