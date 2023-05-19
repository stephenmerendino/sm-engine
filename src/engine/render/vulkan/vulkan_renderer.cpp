#include "engine/core/assert.h"
#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/core/file.h"
#include "engine/core/macros.h"
#include "engine/math/mat44.h"
#include "engine/render/Camera.h"
#include "engine/render/mesh.h"
#include "engine/render/vertex.h"
#include "engine/render/vulkan/vulkan_functions.h"
#include "engine/render/vulkan/vulkan_include.h"
#include "engine/render/vulkan/vulkan_renderer.h"
#include "engine/render/vulkan/vulkan_startup.h"
#include "engine/render/vulkan/vulkan_temp.h"
#include "engine/render/vulkan/vulkan_types.h"
#include "engine/render/window.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

#include <set>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "engine/thirdparty/stb/stb_image.h"

static
VkFormat format_find_supported(context_t& context, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(context.device.phys_device_handle, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	return VK_FORMAT_UNDEFINED;
}

static
VkFormat format_find_depth(context_t& context)
{
	VkFormat depth_format = format_find_supported(context, 
                                                  {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
												  VK_IMAGE_TILING_OPTIMAL, 
												  VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	ASSERT(depth_format != VK_FORMAT_UNDEFINED);
	return depth_format;
} 

static
bool has_stencil_component(VkFormat format)
{
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || 
           (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

static
u32 memory_find_type(context_t& context, u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.phys_device_handle, &mem_properties);

    u32 found_mem_type = UINT_MAX;
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            found_mem_type = i;
            break;
        }
    }

    ASSERT(found_mem_type != UINT_MAX);
    return found_mem_type;
}

static
void image_create(context_t& context, u32 width, u32 height, u32 num_mips, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags mem_props, VkImage& out_image, VkDeviceMemory& out_memory)
{
    // Create vulkan image object
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = num_mips;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = num_samples;
    image_create_info.flags = 0;

    VULKAN_ASSERT(vkCreateImage(context.device.device_handle, &image_create_info, nullptr, &out_image));

    // Setup backing memory of image
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(context.device.device_handle, out_image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_find_type(context, mem_requirements.memoryTypeBits, mem_props);

    VULKAN_ASSERT(vkAllocateMemory(context.device.device_handle, &alloc_info, nullptr, &out_memory));

    vkBindImageMemory(context.device.device_handle, out_image, out_memory, 0);
}

static
void buffer_create(context_t& context, VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags, VkBuffer& out_buffer, VkDeviceMemory& out_memory)
{
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage_flags;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VULKAN_ASSERT(vkCreateBuffer(context.device.device_handle, &create_info, nullptr, &out_buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(context.device.device_handle, out_buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_find_type(context, mem_requirements.memoryTypeBits, memory_property_flags);

    VULKAN_ASSERT(vkAllocateMemory(context.device.device_handle, &alloc_info, nullptr, &out_memory));

    vkBindBufferMemory(context.device.device_handle, out_buffer, out_memory, 0);
}

static 
buffer_t buffer_create(context_t& context, BufferType type, size_t size)
{
    buffer_t buffer;
    buffer.type = type;
    buffer.size = size;

    switch(buffer.type)
    {
        case BufferType::kVertexBuffer:	 buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.handle, buffer.memory); break;
        case BufferType::kIndexBuffer:	 buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.handle, buffer.memory); break;
        case BufferType::kUniformBuffer: buffer_create(context, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer.handle, buffer.memory); break;
        case BufferType::kStagingBuffer: buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer.handle, buffer.memory); break;
        case BufferType::kNumBufferTypes: break;
        case BufferType::kInvalidBuffer: break;
    }

    return buffer;
}

static
void buffer_destroy(context_t& context, VkBuffer vk_buffer, VkDeviceMemory vk_memory)
{
    vkDestroyBuffer(context.device.device_handle, vk_buffer, nullptr);
    vkFreeMemory(context.device.device_handle, vk_memory, nullptr);
}

static
void buffer_destroy(context_t& context, buffer_t& buffer)
{
   buffer_destroy(context, buffer.handle, buffer.memory);
}

static
void buffer_update_data(context_t& context, buffer_t& buffer, void* data, VkDeviceSize gpu_memory_offset = 0)
{
    void* mapped_gpu_mem;
    vkMapMemory(context.device.device_handle, buffer.memory, gpu_memory_offset, buffer.size, 0, &mapped_gpu_mem);
    memcpy(mapped_gpu_mem, data, buffer.size);
    vkUnmapMemory(context.device.device_handle, buffer.memory);
}

static
void queue_flush(VkQueue queue)
{
    vkQueueWaitIdle(queue);
}

static 
void device_flush(device_t& device)
{
    vkDeviceWaitIdle(device.device_handle);
}

static
VkCommandBuffer command_begin_single_time(context_t& context)
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = context.graphics_command_pool.handle;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(context.device.device_handle, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

static
void command_end_single_time(context_t& context, VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    queue_flush(context.device.graphics_queue);

    vkFreeCommandBuffers(context.device.device_handle, context.graphics_command_pool.handle, 1, &command_buffer);
}

static
std::vector<VkCommandBuffer> command_buffers_allocate(context_t& context, VkCommandBufferLevel level, u32 num_buffers)
{
    std::vector<VkCommandBuffer> buffers;
    buffers.resize(num_buffers);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = context.graphics_command_pool.handle;
    alloc_info.level = level;
    alloc_info.commandBufferCount = (u32)num_buffers;

    VULKAN_ASSERT(vkAllocateCommandBuffers(context.device.device_handle, &alloc_info, buffers.data()));
    return buffers;
}

static
void command_copy_buffer(context_t& context, buffer_t& src, buffer_t& dst, VkDeviceSize size)
{
    VkCommandBuffer copy_command_buffer = command_begin_single_time(context);

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(copy_command_buffer, src.handle, dst.handle, 1, &copy_region);

    command_end_single_time(context, copy_command_buffer);
}

static
void command_generate_mip_maps(context_t& context, VkImage image, VkFormat format, u32 width, u32 height, u32 num_mips)
{
    // Check if image format supports linear filtered blitting
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(context.device.phys_device_handle, format, &format_props);
    ASSERT(format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkCommandBuffer command_buffer = command_begin_single_time(context);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mip_width = (i32)width;
    i32 mip_height = (i32)height;

    for (u32 i = 1; i < num_mips; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mip_width, mip_height, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = i - 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        blit.dstSubresource.mipLevel = i;

        vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = num_mips - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    command_end_single_time(context, command_buffer);
}

static
void command_transition_image_layout(context_t& context, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, u32 num_mips)
{
    VkCommandBuffer command_buffer = command_begin_single_time(context);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = num_mips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    bool is_valid_transition = false;
    VkPipelineStageFlags src_stage = 0;
    VkPipelineStageFlags dst_stage = 0;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        is_valid_transition = true;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        is_valid_transition = true;

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    ASSERT(is_valid_transition);

    vkCmdPipelineBarrier(command_buffer, 
            src_stage, dst_stage, 
            0, 
            0, nullptr, 
            0, nullptr, 
            1, &barrier);

    command_end_single_time(context, command_buffer);
}

static
void command_copy_buffer_to_image(context_t& context, VkBuffer buffer, VkImage image, u32 width, u32 height)
{
    VkCommandBuffer command_buffer = command_begin_single_time(context);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    command_end_single_time(context, command_buffer);
}

static
void command_buffer_begin(VkCommandBuffer command_buffer)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pInheritanceInfo = nullptr;
    begin_info.flags = 0;
    VULKAN_ASSERT(vkBeginCommandBuffer(command_buffer, &begin_info));
}

static
void command_buffer_end(VkCommandBuffer command_buffer)
{
    VULKAN_ASSERT(vkEndCommandBuffer(command_buffer));
}

static
void command_buffer_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clear_colors)
{
    VkRenderPassBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass;
    begin_info.framebuffer = framebuffer;
    begin_info.renderArea.offset = offset;// { 0, 0 };
    begin_info.renderArea.extent = extent; // m_pSwapchain->GetExtent();
    begin_info.clearValueCount = static_cast<u32>(clear_colors.size());
    begin_info.pClearValues = clear_colors.data();

    vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

static
void command_buffer_end_render_pass(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
}

static
void draw_renderable_mesh(renderable_mesh_t& renderable_mesh, pipeline_t& pipeline, VkCommandBuffer& command_buffer, const std::vector<VkDescriptorSet>& descriptor_sets)
{
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_handle);

    VkBuffer vertex_buffers[] = { renderable_mesh.vertex_buffer.handle };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, renderable_mesh.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout_handle, 0, (u32)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

    vkCmdDrawIndexed(command_buffer, (u32)renderable_mesh.mesh->m_indices.size(), 1, 0, 0, 0);
}

static 
void buffer_update(context_t& context, buffer_t& buffer, command_pool_t& command_pool, void* data)
{
    if (buffer.type == BufferType::kUniformBuffer)
    {
        buffer_update_data(context, buffer, data);
    }
    else if (buffer.type == BufferType::kVertexBuffer || buffer.type == BufferType::kIndexBuffer)
    {
        buffer_t staging_buffer = buffer_create(context, BufferType::kStagingBuffer, buffer.size);

        buffer_update_data(context, staging_buffer, data);
        command_copy_buffer(context, staging_buffer, buffer, buffer.size);

        buffer_destroy(context, staging_buffer);
    }
}

std::vector<VkVertexInputBindingDescription> get_vertex_input_binding_descs(const vertex_pct_t& v)
{
    UNUSED(v);

    std::vector<VkVertexInputBindingDescription> vertex_input_binding_descs;
    vertex_input_binding_descs.resize(1);
    vertex_input_binding_descs[0].binding = 0;
    vertex_input_binding_descs[0].stride = sizeof(v);
    vertex_input_binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return vertex_input_binding_descs;
}

std::vector<VkVertexInputAttributeDescription> get_vertex_input_attr_descs(const vertex_pct_t& v)
{
    UNUSED(v);

	std::vector<VkVertexInputAttributeDescription> attr_descs;
	attr_descs.resize(3);

	//pos
	VkVertexInputAttributeDescription pos;
	pos.binding = 0;
	pos.location = 0;
	pos.offset = offsetof(vertex_pct_t, m_pos);
	pos.format = VK_FORMAT_R32G32B32_SFLOAT;

	// color
	VkVertexInputAttributeDescription color;
	color.binding = 0;
	color.location = 1;
	color.offset = offsetof(vertex_pct_t, m_color);
	color.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	// tex coord
	VkVertexInputAttributeDescription uv;
	uv.binding = 0;
	uv.location = 2;
	uv.offset = offsetof(vertex_pct_t, m_uv);
	uv.format = VK_FORMAT_R32G32_SFLOAT;

	attr_descs[0] = pos;
	attr_descs[1] = color;
	attr_descs[2] = uv;

	return attr_descs;
}

std::vector<VkVertexInputBindingDescription> mesh_get_vertex_input_binding_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_binding_descs(mesh->m_vertices[0]);
}

std::vector<VkVertexInputAttributeDescription> mesh_get_vertex_input_attr_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_attr_descs(mesh->m_vertices[0]);
}

static
renderable_mesh_t renderable_mesh_create(context_t& context, mesh_t* mesh)
{
    renderable_mesh_t renderable_mesh;
    renderable_mesh.mesh = mesh;
    renderable_mesh.vertex_buffer = buffer_create(context, BufferType::kVertexBuffer, mesh_calc_vertex_buffer_size(mesh));
    renderable_mesh.index_buffer = buffer_create(context, BufferType::kIndexBuffer, mesh_calc_index_buffer_size(mesh));
    buffer_update(context, renderable_mesh.vertex_buffer, context.graphics_command_pool, mesh->m_vertices.data());
    buffer_update(context, renderable_mesh.index_buffer, context.graphics_command_pool, mesh->m_indices.data());
    return renderable_mesh;
}

static
void renderable_mesh_destroy(context_t& context, renderable_mesh_t& rm)
{
    buffer_destroy(context, rm.index_buffer);
    buffer_destroy(context, rm.vertex_buffer);
}

static
texture_t texture_create_from_file(context_t& context, const char* filepath)
{
    texture_t texture;

    int tex_width;
	int tex_height;
	int tex_channels;

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	// load image pixels
	stbi_uc* pixels = stbi_load(filepath, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    // calc num mips
	texture.num_mips = (u32)(std::floor(std::log2(std::max(tex_width, tex_height))) + 1);

	// Copy image data to staging buffer
	VkDeviceSize image_size = tex_width * tex_height * 4; // times 4 because of STBI_rgb_alpha
    buffer_t staging_buffer = buffer_create(context, BufferType::kStagingBuffer, image_size);

	// Do the actual memcpy on cpu side
    buffer_update_data(context, staging_buffer, pixels);

	// Make sure to free the pixels data
	stbi_image_free(pixels);

	// Create the vulkan image and its memory
    image_create(context, tex_width, tex_height, texture.num_mips, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.handle, texture.memory);

	// Transition the image to transfer destination layout
    command_transition_image_layout(context, texture.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture.num_mips);

	// Copy pixel data from staging buffer to actual vulkan image
	//VulkanCommands::CopyBufferToImage(pGraphicsCommandPool, stagingBuffer, m_vkImage, static_cast<U32>(texWidth), static_cast<U32>(texHeight));
    command_copy_buffer_to_image(context, staging_buffer.handle, texture.handle, tex_width, tex_height);

	// Transition image to shader read layout so it can be used in fragment shader
    command_generate_mip_maps(context, texture.handle, format, tex_width, tex_height, texture.num_mips);

	// Set user friendly debug name
	if (is_debug())
	{
		VkDebugUtilsObjectNameInfoEXT debug_name_info = {};
		debug_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debug_name_info.objectType = VK_OBJECT_TYPE_IMAGE;
		debug_name_info.objectHandle = (u64)texture.handle;
		debug_name_info.pObjectName = filepath;
		debug_name_info.pNext = nullptr;

		vkSetDebugUtilsObjectNameEXT(context.device.device_handle, &debug_name_info);
	}

    buffer_destroy(context, staging_buffer);

	texture.image_view = image_view_create(context, texture.handle, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture.num_mips);

    return texture;
}

static
texture_t texture_create_color_target(context_t& context, VkFormat format, u32 width, u32 height)
{
    texture_t texture;
    texture.num_mips = 1;
    image_create(context, 
                 width, height, 
                 texture.num_mips, 
                 context.device.max_num_msaa_samples, 
                 format, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 texture.handle, 
                 texture.memory);
    texture.image_view = image_view_create(context, texture.handle, format, VK_IMAGE_ASPECT_COLOR_BIT, texture.num_mips);
    return texture;
}

static
texture_t texture_create_depth_target(context_t& context, VkFormat format, u32 width, u32 height)
{
    texture_t texture;
    texture.num_mips = 1;
    image_create(context, 
                 width, height, 
                 texture.num_mips, 
                 context.device.max_num_msaa_samples, 
                 format, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 texture.handle, 
                 texture.memory);
    texture.image_view = image_view_create(context, texture.handle, format, VK_IMAGE_ASPECT_DEPTH_BIT, texture.num_mips);
    return texture;
}

static
void texture_destroy(context_t& context, texture_t& texture)
{
    vkDestroyImageView(context.device.device_handle, texture.image_view, nullptr);
    vkDestroyImage(context.device.device_handle, texture.handle, nullptr);
    vkFreeMemory(context.device.device_handle, texture.memory, nullptr); 
}

static
VkShaderModule shader_module_create(context_t& context, const char* shader_filepath)
{
	std::vector<byte_t> raw_shader_code = read_binary_file(shader_filepath);

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = raw_shader_code.size();
	create_info.pCode = (const u32*)raw_shader_code.data();

	VkShaderModule shader_module = VK_NULL_HANDLE;
	VULKAN_ASSERT(vkCreateShaderModule(context.device.device_handle, &create_info, nullptr, &shader_module));

	return shader_module;
}

static
void shader_module_destroy(context_t& context, VkShaderModule shader)
{
    vkDestroyShaderModule(context.device.device_handle, shader, nullptr);
}

static
VkAttachmentDescription setup_attachment_desc(VkFormat format, VkSampleCountFlagBits sample_count, 
                                              VkImageLayout initial_layout, VkImageLayout final_layout,
                                              VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, 
                                              VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, 
                                              VkAttachmentDescriptionFlags flags = 0)
{
    VkAttachmentDescription attachment_desc = {};
    attachment_desc.format = format;
    attachment_desc.samples = sample_count;
    attachment_desc.loadOp = load_op;
    attachment_desc.storeOp = store_op;
    attachment_desc.stencilLoadOp = stencil_load_op;
    attachment_desc.stencilStoreOp = stencil_store_op;
    attachment_desc.initialLayout = initial_layout;
    attachment_desc.finalLayout = final_layout;
    attachment_desc.flags = flags;
    return attachment_desc;
}

static
void render_pass_add_attachment(render_pass_attachments_t& attachments, VkFormat format, VkSampleCountFlagBits sample_count, 
                                                                        VkImageLayout initial_layout, VkImageLayout final_layout,
                                                                        VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, 
                                                                        VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, 
                                                                        VkAttachmentDescriptionFlags flags = 0)
{
        VkAttachmentDescription new_attachment = setup_attachment_desc(format, sample_count, 
                                                                       initial_layout, final_layout, 
                                                                       load_op, store_op, 
                                                                       stencil_load_op, stencil_store_op, 
                                                                       flags);
        attachments.attachments.push_back(new_attachment);
}

static
void subpasses_add_attachment_ref(subpasses_t& subpasses, u32 subpass_index, SubpassAttachmentRefType type, u32 attachment, VkImageLayout layout)
{
    VkAttachmentReference attach_ref = {};
    attach_ref.attachment = attachment;
    attach_ref.layout = layout;

    if(subpass_index >= subpasses.subpasses.size())
    {
        subpasses.subpasses.resize(subpass_index + 1);
    }

    subpass_t& subpass = subpasses.subpasses[subpass_index];

    switch(type)
    {
        case SubpassAttachmentRefType::COLOR: 
            subpass.color_attach_refs.push_back(attach_ref);
            break;
        case SubpassAttachmentRefType::DEPTH: 
            ASSERT(false == subpass.has_depth_attach);
            subpass.depth_attach_ref = attach_ref;
            subpass.has_depth_attach = true;
            break;
        case SubpassAttachmentRefType::RESOLVE: 
            subpass.resolve_attach_refs.push_back(attach_ref);
            break;
    }
}

static
void subpass_add_dependency(subpass_dependencies_t& subpass_dependencies, u32 src_subpass,
                                                                          u32 dst_subpass,
                                                                          VkPipelineStageFlags src_stage,
                                                                          VkPipelineStageFlags dst_stage,
                                                                          VkAccessFlags src_access,
                                                                          VkAccessFlags dst_access,
                                                                          VkDependencyFlags dependency_flags = 0)
{
    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = src_subpass;
    subpass_dependency.dstSubpass = dst_subpass;
    subpass_dependency.srcStageMask = src_stage;
    subpass_dependency.dstStageMask = dst_stage;
    subpass_dependency.srcAccessMask = src_access;
    subpass_dependency.dstAccessMask = dst_access;
    subpass_dependency.dependencyFlags = dependency_flags;
    subpass_dependencies.dependencies.push_back(subpass_dependency);
}

static
render_pass_t render_pass_create(context_t& context, render_pass_attachments_t& attachments, subpasses_t& subpasses, subpass_dependencies_t& subpass_dependencies)
{
    render_pass_t render_pass;

    std::vector<VkSubpassDescription> subpass_descriptions;
    subpass_descriptions.resize(subpasses.subpasses.size());

    for(i32 i = 0; i < subpasses.subpasses.size(); i++)
    {
        subpass_t& subpass = subpasses.subpasses[i];

        VkSubpassDescription subpass_desc = {};
        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.colorAttachmentCount = (u32)subpass.color_attach_refs.size();
        subpass_desc.pColorAttachments = subpass.color_attach_refs.data();
        subpass_desc.pResolveAttachments = subpass.resolve_attach_refs.data();
        subpass_desc.pDepthStencilAttachment = subpass.has_depth_attach ? &subpass.depth_attach_ref : VK_NULL_HANDLE;

        subpass_descriptions[i] = subpass_desc;
    }

    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = (u32)attachments.attachments.size();
    create_info.pAttachments = attachments.attachments.data();
    create_info.subpassCount = (u32)subpass_descriptions.size();
    create_info.pSubpasses = subpass_descriptions.data();
    create_info.dependencyCount = (u32)subpass_dependencies.dependencies.size();
    create_info.pDependencies = subpass_dependencies.dependencies.data();

    VULKAN_ASSERT(vkCreateRenderPass(context.device.device_handle, &create_info, nullptr, &render_pass.handle));

    render_pass.attachments = attachments;
    render_pass.subpasses = subpasses;
    render_pass.subpass_dependencies = subpass_dependencies;

    return render_pass;
}

static
framebuffer_t framebuffer_create(context_t& context, render_pass_t& render_pass, const std::vector<VkImageView> attachments, u32 width, u32 height, u32 layers)
{
    VkFramebufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = render_pass.handle;
    create_info.pAttachments = attachments.data();
    create_info.attachmentCount = (u32)attachments.size();
    create_info.width = width;
    create_info.height = height;
    create_info.layers = layers;

    framebuffer_t framebuffer;
    VULKAN_ASSERT(vkCreateFramebuffer(context.device.device_handle, &create_info, nullptr, &framebuffer.handle));
    return framebuffer;
}

static 
void framebuffer_destroy(context_t& context, framebuffer_t& framebuffer)
{
    vkDestroyFramebuffer(context.device.device_handle, framebuffer.handle, nullptr);
}

static
sampler_t sampler_create(context_t& context, u32 max_mip_level)
{
    sampler_t sampler;
    sampler.max_mip_level = max_mip_level;

    // Probably should have a wrapper class around the physical device and store its properties so we can easily retrieve when needed without having to make a vulkan call everytime
    VkPhysicalDeviceProperties properties = context.device.phys_device_properties;

    VkSamplerCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = VK_FILTER_LINEAR;
    create_info.minFilter = VK_FILTER_LINEAR;
    create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.anisotropyEnable = VK_TRUE;
    create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.mipLodBias = 0.0f;
    create_info.minLod = 0.0f;
    create_info.maxLod = (f32)max_mip_level;

    VULKAN_ASSERT(vkCreateSampler(context.device.device_handle, &create_info, nullptr, &sampler.handle));
    return sampler;
}

static
void sampler_destroy(context_t& context, sampler_t& sampler)
{
    vkDestroySampler(context.device.device_handle, sampler.handle, nullptr);
}

static
void descriptor_set_layout_destroy(context_t& context, VkDescriptorSetLayout layout)
{
	vkDestroyDescriptorSetLayout(context.device.device_handle, layout, nullptr);
}

static
void descriptor_pool_destroy(context_t& context, VkDescriptorPool pool)
{
	vkDestroyDescriptorPool(context.device.device_handle, pool, nullptr);
}

static 
void render_pass_destroy(context_t& context, render_pass_t& render_pass)
{
    vkDestroyRenderPass(context.device.device_handle, render_pass.handle, nullptr);
}

static
void command_buffers_free(context_t& context, std::vector<VkCommandBuffer>& command_buffers)
{
	vkFreeCommandBuffers(context.device.device_handle, context.graphics_command_pool.handle, (u32)command_buffers.size(), command_buffers.data());
}

static
void pipeline_destroy(context_t& context, pipeline_t& pipeline)
{
	vkDestroyPipeline(context.device.device_handle, pipeline.pipeline_handle, nullptr);
	vkDestroyPipelineLayout(context.device.device_handle, pipeline.pipeline_layout_handle, nullptr);
}

static
void render_pass_reset(render_pass_t& render_pass)
{
    render_pass.handle = VK_NULL_HANDLE;
    render_pass.subpasses.subpasses.clear();
    render_pass.subpass_dependencies.dependencies.clear();
    render_pass.attachments.attachments.clear();
}

static
void pipeline_reset(pipeline_t& pipeline)
{
    pipeline.pipeline_handle = VK_NULL_HANDLE;
    pipeline.pipeline_layout_handle = VK_NULL_HANDLE;
    pipeline.color_blend_attachments.clear();
    pipeline.shaders.clear();
    pipeline.shader_stage_infos.clear();
}

static
semaphore_t semaphore_create(context_t& context)
{
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    semaphore_t semaphore;
    VULKAN_ASSERT(vkCreateSemaphore(context.device.device_handle, &semaphore_create_info, nullptr, &semaphore.handle));
    return semaphore;
}

static
fence_t fence_create(context_t& context)
{
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    fence_t fence;
    VULKAN_ASSERT(vkCreateFence(context.device.device_handle, &fence_create_info, nullptr, &fence.handle));
    return fence;
}

static
void fence_reset(context_t& context, fence_t& fence)
{
    vkResetFences(context.device.device_handle, 1, &fence.handle);
}

static
void fence_wait(context_t& context, fence_t& fence, u64 timeout = UINT64_MAX)
{
    vkWaitForFences(context.device.device_handle, 1, &fence.handle, VK_TRUE, UINT64_MAX); 
}

static 
void semaphore_destroy(context_t& context, semaphore_t& semaphore)
{
    vkDestroySemaphore(context.device.device_handle, semaphore.handle, nullptr);
}

static 
void fence_destroy(context_t& context, fence_t& fence)
{
    vkDestroyFence(context.device.device_handle, fence.handle, nullptr);
}

static mesh_t* s_viking_room_mesh = nullptr;
static mesh_t* s_world_axes_mesh = nullptr;
static renderable_mesh_t s_viking_room_renderable_mesh;
static renderable_mesh_t s_world_axes_renderable_mesh;

static texture_t s_viking_room_texture;
static sampler_t s_viking_room_sampler;
static texture_t s_color_target;
static texture_t s_depth_target;

static std::vector<buffer_t> s_uniform_buffers;

static render_pass_t s_render_pass;

static std::vector<framebuffer_t> s_framebuffers;
static pipeline_t s_viking_room_pipeline;
static pipeline_t s_world_axes_pipeline;

static VkDescriptorSetLayout s_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorPool s_descriptor_pool = VK_NULL_HANDLE;
std::vector<VkDescriptorSet> s_descriptor_sets;

static std::vector<VkCommandBuffer> s_command_buffers;

static std::vector<VkFence> s_swapchain_images_in_flight;

static
void renderer_init_resources(context_t& context)
{
    render_pass_reset(s_render_pass);

    VkFormat depth_format = format_find_depth(context);

    // render targets
    s_color_target = texture_create_color_target(context, context.swapchain.format, context.swapchain.extent.width, context.swapchain.extent.height);
    s_depth_target = texture_create_depth_target(context, depth_format, context.swapchain.extent.width, context.swapchain.extent.height);

    // uniform buffers
	s_uniform_buffers.resize(context.swapchain.num_images);
	for (i32 i = 0; i < context.swapchain.num_images; i++)
	{
        s_uniform_buffers[i] = buffer_create(context, BufferType::kUniformBuffer, sizeof(mvp_buffer_t));
	}

    // render pass
    {
        render_pass_attachments_t attachments;
        render_pass_add_attachment(attachments, context.swapchain.format, context.device.max_num_msaa_samples, 
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                                                 VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);
        render_pass_add_attachment(attachments, depth_format, context.device.max_num_msaa_samples, 
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
                                                 VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);
        render_pass_add_attachment(attachments, context.swapchain.format, VK_SAMPLE_COUNT_1_BIT, 
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);

        subpasses_t subpasses;
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::DEPTH, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::RESOLVE, 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        subpass_dependencies_t subpass_dependencies;
        subpass_add_dependency(subpass_dependencies, VK_SUBPASS_EXTERNAL, 
                                                     0, 
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                     0,
                                                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        s_render_pass = render_pass_create(context, attachments, subpasses, subpass_dependencies);
    }

    // framebuffers
    s_framebuffers.resize(context.swapchain.num_images);
    {
        for(i32 i = 0; i < context.swapchain.num_images; i++)
        {
            std::vector<VkImageView> attachments { s_color_target.image_view, 
                                                   s_depth_target.image_view, 
                                                   context.swapchain.image_views[i] };

            s_framebuffers[i] = framebuffer_create(context, s_render_pass, attachments, context.swapchain.extent.width, context.swapchain.extent.height, 1);
        }
    }
    
    // descriptor sets
    {
        struct descriptor_set_layout_t
        {
            VkDescriptorSetLayout layout;
            std::vector<VkDescriptorSetLayoutBinding> bindings;    
        };

        std::vector<VkDescriptorSetLayoutBinding> bindings;

        // layout
        VkDescriptorSetLayoutBinding uniform_binding = {};
        uniform_binding.binding = 0;
        uniform_binding.descriptorCount = 1;
        uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniform_binding.pImmutableSamplers = nullptr;
        bindings.push_back(uniform_binding);

        VkDescriptorSetLayoutBinding sampler_binding = {};
        sampler_binding.binding = 1;
        sampler_binding.descriptorCount = 1;
        sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        sampler_binding.pImmutableSamplers = nullptr;
        bindings.push_back(sampler_binding);

        VkDescriptorSetLayoutCreateInfo layout_create_info = {};
        layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_create_info.bindingCount = (u32)bindings.size();
        layout_create_info.pBindings = bindings.data();

        VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device.device_handle, &layout_create_info, nullptr, &s_descriptor_set_layout));

        // pool
        //
        struct desciptor_pool_t
        {
            VkDescriptorPool handle;
            std::vector<VkDescriptorPoolSize> pool_sizes;
        };

        std::vector<VkDescriptorPoolSize> pool_sizes;

        VkDescriptorPoolSize uniform_pool_size = {};
        uniform_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_pool_size.descriptorCount = context.swapchain.num_images;
        pool_sizes.push_back(uniform_pool_size);

        VkDescriptorPoolSize sampler_pool_size = {};
        sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_pool_size.descriptorCount = context.swapchain.num_images;
        pool_sizes.push_back(sampler_pool_size);

        VkDescriptorPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.poolSizeCount = (u32)pool_sizes.size();
        create_info.pPoolSizes = pool_sizes.data();
        create_info.maxSets = context.swapchain.num_images;

        VULKAN_ASSERT(vkCreateDescriptorPool(context.device.device_handle, &create_info, nullptr, &s_descriptor_pool));

        // descriptor sets
        std::vector<VkDescriptorSetLayout> layouts(context.swapchain.num_images, s_descriptor_set_layout);

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = s_descriptor_pool;
        alloc_info.descriptorSetCount = context.swapchain.num_images;
        alloc_info.pSetLayouts = layouts.data();

        s_descriptor_sets.resize(context.swapchain.num_images);
        VULKAN_ASSERT(vkAllocateDescriptorSets(context.device.device_handle, &alloc_info, s_descriptor_sets.data()));

        // descriptor set writes
        struct descriptor_set_write_info_t
        {
            VkDescriptorType type;
            union
            {
                VkDescriptorBufferInfo buffer_write_info;
                VkDescriptorImageInfo image_write_info;
            };
        };
        
        std::vector<descriptor_set_write_info_t> descriptor_set_write_infos;
        std::vector<VkWriteDescriptorSet> descriptor_set_writes;

        for(i32 i = 0; i < context.swapchain.num_images; i++)
        {
            descriptor_set_write_infos.clear();
            descriptor_set_writes.clear(); 

            // set up uniform write
            descriptor_set_write_info_t uniform_write_info = {};
            uniform_write_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniform_write_info.buffer_write_info.buffer = s_uniform_buffers[i].handle;
            uniform_write_info.buffer_write_info.offset = 0;
            uniform_write_info.buffer_write_info.range = s_uniform_buffers[i].size;
            descriptor_set_write_infos.push_back(uniform_write_info);

            VkWriteDescriptorSet uniform_write = {};
            uniform_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uniform_write.dstBinding = 0;
            uniform_write.dstArrayElement = 0;
            uniform_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniform_write.descriptorCount = 1;
            descriptor_set_writes.push_back(uniform_write);

            // set up sampler write
            descriptor_set_write_info_t sampler_write_info = {};
            sampler_write_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sampler_write_info.image_write_info.imageView = s_viking_room_texture.image_view;
            sampler_write_info.image_write_info.sampler = s_viking_room_sampler.handle;
            sampler_write_info.image_write_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptor_set_write_infos.push_back(sampler_write_info);

            VkWriteDescriptorSet sampler_write = {};
            sampler_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            sampler_write.dstBinding = 1;
            sampler_write.dstArrayElement = 0;
            sampler_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sampler_write.descriptorCount = 1;
            descriptor_set_writes.push_back(sampler_write);

            // do the writes
            for (u32 j = 0; j < descriptor_set_writes.size(); j++)
            {
                descriptor_set_writes[j].dstSet = s_descriptor_sets[i];
                switch (descriptor_set_writes[j].descriptorType)
                {
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: descriptor_set_writes[j].pBufferInfo = &descriptor_set_write_infos[j].buffer_write_info; break;
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: descriptor_set_writes[j].pImageInfo = &descriptor_set_write_infos[j].image_write_info; break;
                    case VK_DESCRIPTOR_TYPE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
                    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
                    case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE:
                    case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
                    case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
                    case VK_DESCRIPTOR_TYPE_MAX_ENUM:
                        ASSERT("Trying to set up a descriptor set write that isn't supported yet\n");
                        break;
                    }
            }

            vkUpdateDescriptorSets(context.device.device_handle, (u32)descriptor_set_writes.size(), descriptor_set_writes.data(), 0, nullptr);
        }
    }

    // pipelines
    {
        // Viking Room
        {
            pipeline_reset(s_viking_room_pipeline);

            // shaders
            VkShaderModule vert_shader = shader_module_create(context, "shaders/tri-vert.spv");
            VkShaderModule frag_shader = shader_module_create(context, "shaders/tri-frag.spv");

            VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
            vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vert_shader_stage_create_info.module = vert_shader;
            vert_shader_stage_create_info.pName = "main";

            VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
            frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_shader_stage_create_info.module = frag_shader;
            frag_shader_stage_create_info.pName = "main";

            s_viking_room_pipeline.shaders.push_back(vert_shader);
            s_viking_room_pipeline.shaders.push_back(frag_shader);
            s_viking_room_pipeline.shader_stage_infos.push_back(vert_shader_stage_create_info);
            s_viking_room_pipeline.shader_stage_infos.push_back(frag_shader_stage_create_info);

            // vertex input
            VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
            input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_info.primitiveRestartEnable = VK_FALSE;
            s_viking_room_pipeline.input_assembly_info = input_assembly_info;

            // viewport and scissor
            VkViewport viewport;
            VkRect2D scissor;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = context.swapchain.extent.width;
            viewport.height = context.swapchain.extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            scissor.offset = { 0, 0};
            scissor.extent = context.swapchain.extent;
            s_viking_room_pipeline.viewport = viewport;
            s_viking_room_pipeline.scissor = scissor;

            // rasterizer
            VkPipelineRasterizationStateCreateInfo raster_info = {};
            raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            raster_info.depthClampEnable = VK_FALSE;
            raster_info.rasterizerDiscardEnable = VK_FALSE;
            raster_info.polygonMode = VK_POLYGON_MODE_FILL;
            raster_info.lineWidth = 1.0f;
            raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
            raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            raster_info.depthBiasEnable = VK_FALSE;
            raster_info.depthBiasConstantFactor = 0.0f;
            raster_info.depthBiasClamp = 0.0f;
            raster_info.depthBiasSlopeFactor = 0.0f;
            s_viking_room_pipeline.raster_info = raster_info;

            // multisampling
            VkPipelineMultisampleStateCreateInfo multisample_info = {};
            multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_info.sampleShadingEnable = VK_FALSE;
            multisample_info.rasterizationSamples = context.device.max_num_msaa_samples;
            multisample_info.minSampleShading = 1.0f;
            multisample_info.pSampleMask = nullptr;
            multisample_info.alphaToCoverageEnable = VK_FALSE;
            multisample_info.alphaToOneEnable = VK_FALSE;
            s_viking_room_pipeline.multisample_info = multisample_info;

            // depth stencil
            VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
            depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_info.depthTestEnable = VK_TRUE;
            depth_stencil_info.depthWriteEnable = VK_TRUE;
            depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
            depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
            depth_stencil_info.minDepthBounds = 0.0f;
            depth_stencil_info.maxDepthBounds = 1.0f;
            depth_stencil_info.stencilTestEnable = VK_FALSE;
            depth_stencil_info.front = {};
            depth_stencil_info.back = {};
            s_viking_room_pipeline.depth_stencil_info = depth_stencil_info;
            
            // color blend attachments
            VkPipelineColorBlendAttachmentState color_blend_attachment = {};
            color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachment.blendEnable = VK_FALSE;
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            s_viking_room_pipeline.color_blend_attachments.push_back(color_blend_attachment);

            // color blend
            VkPipelineColorBlendStateCreateInfo color_blend_info = {};
            color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_info.logicOpEnable = VK_FALSE;
            color_blend_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_info.blendConstants[0] = 0.0f;
            color_blend_info.blendConstants[1] = 0.0f;
            color_blend_info.blendConstants[2] = 0.0f;
            color_blend_info.blendConstants[3] = 0.0f;
            color_blend_info.attachmentCount = (u32)s_viking_room_pipeline.color_blend_attachments.size();
            color_blend_info.pAttachments = s_viking_room_pipeline.color_blend_attachments.data();
            s_viking_room_pipeline.color_blend_info = color_blend_info;

            // pipeline layout
            std::vector<VkDescriptorSetLayout> descriptor_set_layouts = { s_descriptor_set_layout };
            VkPipelineLayoutCreateInfo layout_info = {};
            layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layout_info.setLayoutCount = (u32)descriptor_set_layouts.size();
            layout_info.pSetLayouts = descriptor_set_layouts.data();
            layout_info.pushConstantRangeCount = 0;
            layout_info.pPushConstantRanges = nullptr;
            s_viking_room_pipeline.pipeline_layout_info = layout_info;
            VULKAN_ASSERT(vkCreatePipelineLayout(context.device.device_handle, &s_viking_room_pipeline.pipeline_layout_info, nullptr, &s_viking_room_pipeline.pipeline_layout_handle));

            // vertex input
            std::vector<VkVertexInputBindingDescription> input_binding_descs = mesh_get_vertex_input_binding_descs(s_viking_room_renderable_mesh.mesh);
            std::vector<VkVertexInputAttributeDescription> input_attr_descs = mesh_get_vertex_input_attr_descs(s_viking_room_renderable_mesh.mesh);
            VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
            vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_info.vertexBindingDescriptionCount = (u32)input_binding_descs.size();
            vertex_input_info.pVertexBindingDescriptions = input_binding_descs.data();
            vertex_input_info.vertexAttributeDescriptionCount = (u32)(input_attr_descs.size());
            vertex_input_info.pVertexAttributeDescriptions = input_attr_descs.data();

            // viewport
            VkPipelineViewportStateCreateInfo viewport_state_info = {};
            viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_info.pViewports = &s_viking_room_pipeline.viewport;
            viewport_state_info.viewportCount = 1;
            viewport_state_info.pScissors = &s_viking_room_pipeline.scissor;
            viewport_state_info.scissorCount = 1;

            // pipeline
            VkGraphicsPipelineCreateInfo pipeline_info = {};
            pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_info.stageCount = (u32)s_viking_room_pipeline.shader_stage_infos.size();
            pipeline_info.pStages = s_viking_room_pipeline.shader_stage_infos.data();
            pipeline_info.pVertexInputState = &vertex_input_info;
            pipeline_info.pInputAssemblyState = &input_assembly_info;
            pipeline_info.pViewportState = &viewport_state_info;
            pipeline_info.pRasterizationState = &raster_info;
            pipeline_info.pMultisampleState = &multisample_info;
            pipeline_info.pDepthStencilState = &depth_stencil_info;
            pipeline_info.pColorBlendState = &color_blend_info;
            pipeline_info.pDynamicState = nullptr; //#TODO Enable this pipeline state
            pipeline_info.layout = s_viking_room_pipeline.pipeline_layout_handle;
            pipeline_info.renderPass = s_render_pass.handle;
            pipeline_info.subpass = 0;
            pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_info.basePipelineIndex = -1;

            VULKAN_ASSERT(vkCreateGraphicsPipelines(context.device.device_handle, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &s_viking_room_pipeline.pipeline_handle));

            // cleanup
            for (VkShaderModule shaderModule : s_viking_room_pipeline.shaders)
            {
                shader_module_destroy(context, shaderModule);
            }
        }

        // World axes
        {
            //pipelineMaker.Reset();
            pipeline_reset(s_world_axes_pipeline);

            // shaders
            VkShaderModule vert_shader = shader_module_create(context, "shaders/simple-color-vert.spv");
            VkShaderModule frag_shader = shader_module_create(context, "shaders/simple-color-frag.spv");

            VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
            vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vert_shader_stage_create_info.module = vert_shader;
            vert_shader_stage_create_info.pName = "main";

            VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
            frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_shader_stage_create_info.module = frag_shader;
            frag_shader_stage_create_info.pName = "main";

            s_world_axes_pipeline.shaders.push_back(vert_shader);
            s_world_axes_pipeline.shaders.push_back(frag_shader);
            s_world_axes_pipeline.shader_stage_infos.push_back(vert_shader_stage_create_info);
            s_world_axes_pipeline.shader_stage_infos.push_back(frag_shader_stage_create_info);

            // vertex input
            VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
            input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            input_assembly_info.primitiveRestartEnable = VK_FALSE;
            s_world_axes_pipeline.input_assembly_info = input_assembly_info;

            // viewport and scissor
            VkViewport viewport;
            VkRect2D scissor;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = context.swapchain.extent.width;
            viewport.height = context.swapchain.extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            scissor.offset = { 0, 0};
            scissor.extent = context.swapchain.extent;
            s_world_axes_pipeline.viewport = viewport;
            s_world_axes_pipeline.scissor = scissor;

            // rasterizer
            VkPipelineRasterizationStateCreateInfo raster_info = {};
            raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            raster_info.depthClampEnable = VK_FALSE;
            raster_info.rasterizerDiscardEnable = VK_FALSE;
            raster_info.polygonMode = VK_POLYGON_MODE_FILL;
            raster_info.lineWidth = 1.0f;
            raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
            raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            raster_info.depthBiasEnable = VK_FALSE;
            raster_info.depthBiasConstantFactor = 0.0f;
            raster_info.depthBiasClamp = 0.0f;
            raster_info.depthBiasSlopeFactor = 0.0f;
            s_world_axes_pipeline.raster_info = raster_info;

            // multisampling
            VkPipelineMultisampleStateCreateInfo multisample_info = {};
            multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_info.sampleShadingEnable = VK_FALSE;
            multisample_info.rasterizationSamples = context.device.max_num_msaa_samples;
            multisample_info.minSampleShading = 1.0f;
            multisample_info.pSampleMask = nullptr;
            multisample_info.alphaToCoverageEnable = VK_FALSE;
            multisample_info.alphaToOneEnable = VK_FALSE;
            s_world_axes_pipeline.multisample_info = multisample_info;

            // depth stencil
            VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
            depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_info.depthTestEnable = VK_TRUE;
            depth_stencil_info.depthWriteEnable = VK_TRUE;
            depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
            depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
            depth_stencil_info.minDepthBounds = 0.0f;
            depth_stencil_info.maxDepthBounds = 1.0f;
            depth_stencil_info.stencilTestEnable = VK_FALSE;
            depth_stencil_info.front = {};
            depth_stencil_info.back = {};
            s_world_axes_pipeline.depth_stencil_info = depth_stencil_info;

            // color blend attachments
            VkPipelineColorBlendAttachmentState color_blend_attachment = {};
            color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachment.blendEnable = VK_FALSE;
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            s_world_axes_pipeline.color_blend_attachments.push_back(color_blend_attachment);

            // color blend
            VkPipelineColorBlendStateCreateInfo color_blend_info = {};
            color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_info.logicOpEnable = VK_FALSE;
            color_blend_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_info.blendConstants[0] = 0.0f;
            color_blend_info.blendConstants[1] = 0.0f;
            color_blend_info.blendConstants[2] = 0.0f;
            color_blend_info.blendConstants[3] = 0.0f;
            color_blend_info.attachmentCount = (u32)s_world_axes_pipeline.color_blend_attachments.size();
            color_blend_info.pAttachments = s_world_axes_pipeline.color_blend_attachments.data();
            s_world_axes_pipeline.color_blend_info = color_blend_info;

            // pipeline layout
            std::vector<VkDescriptorSetLayout> descriptor_set_layouts = { s_descriptor_set_layout };
            VkPipelineLayoutCreateInfo layout_info = {};
            layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layout_info.setLayoutCount = (u32)descriptor_set_layouts.size();
            layout_info.pSetLayouts = descriptor_set_layouts.data();
            layout_info.pushConstantRangeCount = 0;
            layout_info.pPushConstantRanges = nullptr;
            s_world_axes_pipeline.pipeline_layout_info = layout_info;
            VULKAN_ASSERT(vkCreatePipelineLayout(context.device.device_handle, &s_world_axes_pipeline.pipeline_layout_info, nullptr, &s_world_axes_pipeline.pipeline_layout_handle));

            // vertex input
            std::vector<VkVertexInputBindingDescription> input_binding_descs = mesh_get_vertex_input_binding_descs(s_world_axes_renderable_mesh.mesh);
            std::vector<VkVertexInputAttributeDescription> input_attr_descs = mesh_get_vertex_input_attr_descs(s_world_axes_renderable_mesh.mesh);
            VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
            vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_info.vertexBindingDescriptionCount = (u32)input_binding_descs.size();
            vertex_input_info.pVertexBindingDescriptions = input_binding_descs.data();
            vertex_input_info.vertexAttributeDescriptionCount = (u32)(input_attr_descs.size());
            vertex_input_info.pVertexAttributeDescriptions = input_attr_descs.data();

            // viewport
            VkPipelineViewportStateCreateInfo viewport_state_info = {};
            viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_info.pViewports = &s_world_axes_pipeline.viewport;
            viewport_state_info.viewportCount = 1;
            viewport_state_info.pScissors = &s_world_axes_pipeline.scissor;
            viewport_state_info.scissorCount = 1;

            // pipeline
            VkGraphicsPipelineCreateInfo pipeline_info = {};
            pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_info.stageCount = (u32)s_world_axes_pipeline.shader_stage_infos.size();
            pipeline_info.pStages = s_world_axes_pipeline.shader_stage_infos.data();
            pipeline_info.pVertexInputState = &vertex_input_info;
            pipeline_info.pInputAssemblyState = &input_assembly_info;
            pipeline_info.pViewportState = &viewport_state_info;
            pipeline_info.pRasterizationState = &raster_info;
            pipeline_info.pMultisampleState = &multisample_info;
            pipeline_info.pDepthStencilState = &depth_stencil_info;
            pipeline_info.pColorBlendState = &color_blend_info;
            pipeline_info.pDynamicState = nullptr; //#TODO Enable this pipeline state
            pipeline_info.layout = s_world_axes_pipeline.pipeline_layout_handle;
            pipeline_info.renderPass = s_render_pass.handle;
            pipeline_info.subpass = 0;
            pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_info.basePipelineIndex = -1;

            VULKAN_ASSERT(vkCreateGraphicsPipelines(context.device.device_handle, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &s_world_axes_pipeline.pipeline_handle));

            // cleanup
            for (VkShaderModule shaderModule : s_world_axes_pipeline.shaders)
            {
                shader_module_destroy(context, shaderModule);
            }
        }
    }

    // command buffers
    {
        s_command_buffers = command_buffers_allocate(context, VK_COMMAND_BUFFER_LEVEL_PRIMARY, context.swapchain.num_images);

        for (i32 i = 0; i < (i32)s_command_buffers.size(); i++)
        {
            command_buffer_begin(s_command_buffers[i]);
                VkOffset2D offset{ 0, 0 };

                VkClearValue color_target_clear;
                color_target_clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

                VkClearValue depth_target_clear;
                depth_target_clear.depthStencil = {1.0f, 0};

                std::vector<VkClearValue> clear_colors;
                clear_colors.push_back(color_target_clear);
                clear_colors.push_back(depth_target_clear);

                command_buffer_begin_render_pass(s_command_buffers[i], s_render_pass.handle, s_framebuffers[i].handle, offset, context.swapchain.extent, clear_colors);
                    // Normal draw
                    {
                        std::vector<VkDescriptorSet> draw_descriptor_sets = { s_descriptor_sets[i] };
                        draw_renderable_mesh(s_viking_room_renderable_mesh, s_viking_room_pipeline, s_command_buffers[i], draw_descriptor_sets);
                    }
                     
                    // World Axes
                    {
                        std::vector<VkDescriptorSet> draw_descriptor_sets = { s_descriptor_sets[i] };
                        draw_renderable_mesh(s_world_axes_renderable_mesh, s_world_axes_pipeline, s_command_buffers[i], draw_descriptor_sets);
                    }
                command_buffer_end_render_pass(s_command_buffers[i]);
            command_buffer_end(s_command_buffers[i]);
        }
    }
}

static
void swapchain_teardown(context_t& context)
{
	for (framebuffer_t& fb : s_framebuffers)
	{
        framebuffer_destroy(context, fb);
    }

    command_buffers_free(context, s_command_buffers);

    pipeline_destroy(context, s_viking_room_pipeline);
    pipeline_destroy(context, s_world_axes_pipeline);

    render_pass_destroy(context, s_render_pass);

    texture_destroy(context, s_depth_target);
    texture_destroy(context, s_color_target);

	for (u32 i = 0; i < context.swapchain.num_images; i++)
	{
        buffer_destroy(context, s_uniform_buffers[i]);
	}

    descriptor_pool_destroy(context, s_descriptor_pool);
    descriptor_set_layout_destroy(context, s_descriptor_set_layout);
}

static
void swapchain_recreate(context_t& context)
{
    if(context.window->was_closed)
    {
        return;
    }

    device_flush(context.device); 

    swapchain_teardown(context);
    context_refresh_swapchain(context);
    renderer_init_resources(context);
}

static
void update_uniform_buffer(context_t& context, camera_t& camera, u32 current_image)
{
	mvp_buffer_t ubo;

	static f32 dt = 0.0f;
	dt += 0.016f;

	const f32 speed = 0.0f;

	mat44 rot = MAT44_IDENTITY;
    rotate_z_deg(rot, dt * speed);
	ubo.model = rot;

	ubo.view = camera_get_view_tranform(camera);

    f32 aspect = (f32)context.swapchain.extent.width / (f32)context.swapchain.extent.height;
	ubo.projection = create_perspective_projection(45.0, 0.01f, 110.0f, aspect);

    buffer_update_data(context, s_uniform_buffers[current_image], &ubo);
}

static u32 s_cur_frame = 0;
static context_t* s_context;
static camera_t* s_camera = nullptr;

frame_t frame_create(context_t& context)
{
    frame_t frame;
    frame.swap_chain_image_index = -1;
    frame.image_available_semaphore = semaphore_create(context);
    frame.render_finished_semaphore = semaphore_create(context);
    frame.frame_completed_fence = fence_create(context);
    return frame;
}

void frame_destroy(context_t& context, frame_t& frame)
{
    semaphore_destroy(context, frame.image_available_semaphore);
    semaphore_destroy(context, frame.render_finished_semaphore);
    fence_destroy(context, frame.frame_completed_fence);
}

std::vector<frame_t> s_in_flight_frames;

void renderer_init(window_t* app_window)
{
    ASSERT(nullptr != app_window);

    s_context = context_create(app_window);

    // frames
    s_in_flight_frames.resize(MAX_NUM_FRAMES_IN_FLIGHT);
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
    {
        s_in_flight_frames[i] = frame_create(*s_context);
    }

    // meshes
    s_viking_room_mesh = mesh_load_from_obj("models/viking_room.obj");
    s_viking_room_renderable_mesh = renderable_mesh_create(*s_context, s_viking_room_mesh);

    s_world_axes_mesh = mesh_load_axes();
    s_world_axes_renderable_mesh = renderable_mesh_create(*s_context, s_world_axes_mesh);

    // texture and sampler
    s_viking_room_texture = texture_create_from_file(*s_context, "textures/viking_room.png");
    s_viking_room_sampler = sampler_create(*s_context, s_viking_room_texture.num_mips);

    renderer_init_resources(*s_context);
}

void renderer_set_main_camera(camera_t* camera)
{
    ASSERT(nullptr != camera); 
    s_camera = camera;
}

static
VkResult frame_acquire_next_image(context_t& context, frame_t& frame)
{
	// Acquire an image from the swap chain
	VkResult res = vkAcquireNextImageKHR(context.device.device_handle, 
                                         context.swapchain.handle, 
                                         UINT64_MAX, 
                                         frame.image_available_semaphore.handle, 
                                         VK_NULL_HANDLE, 
                                         &frame.swap_chain_image_index);
    return res; 
}

static
VkResult frame_present(context_t& context, frame_t& frame)
{
	VkSemaphore signal_semaphores[] = { frame.render_finished_semaphore.handle };

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;
	VkSwapchainKHR swapchains[] = { context.swapchain.handle };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &frame.swap_chain_image_index;
	present_info.pResults = nullptr;

	VkResult res = vkQueuePresentKHR(context.device.graphics_queue, &present_info);
    return res;
}

void renderer_render_frame()
{
    frame_t& frame = s_in_flight_frames[s_cur_frame];

	// Wait for previous frame to finish, this blocks on CPU
    fence_wait(*s_context, frame.frame_completed_fence);

	// Acquire an image from the swap chain
	VkResult res = frame_acquire_next_image(*s_context, frame); 
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
        swapchain_recreate(*s_context);
	    return;
	}
	ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);

	update_uniform_buffer(*s_context, *s_camera, frame.swap_chain_image_index);

	if (VK_NULL_HANDLE != s_context->swapchain.image_in_flight_fences[frame.swap_chain_image_index].handle)
	{
        fence_wait(*s_context, s_context->swapchain.image_in_flight_fences[frame.swap_chain_image_index]);
	}

	s_context->swapchain.image_in_flight_fences[frame.swap_chain_image_index] = frame.frame_completed_fence;

	// Submit command buffer
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { frame.image_available_semaphore.handle };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &s_command_buffers[frame.swap_chain_image_index];

	VkSemaphore signal_semaphores[] = { frame.render_finished_semaphore.handle };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

    fence_reset(*s_context, frame.frame_completed_fence);
	VULKAN_ASSERT(vkQueueSubmit(s_context->device.graphics_queue, 1, &submit_info, frame.frame_completed_fence.handle));

	// Present to screen
    VkResult present_res = frame_present(*s_context, frame);
	if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR || s_context->window->was_resized)
	{
        swapchain_recreate(*s_context);
	}
	else
	{
		VULKAN_ASSERT(res);
	}

	s_cur_frame = (s_cur_frame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
}

void renderer_deinit()
{
    device_flush(s_context->device);

    swapchain_teardown(*s_context);

    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
        frame_destroy(*s_context, s_in_flight_frames[i]);
	}

    sampler_destroy(*s_context, s_viking_room_sampler);
    texture_destroy(*s_context, s_viking_room_texture);

    renderable_mesh_destroy(*s_context, s_world_axes_renderable_mesh);
    renderable_mesh_destroy(*s_context, s_viking_room_renderable_mesh);
    delete s_world_axes_mesh;
    delete s_viking_room_mesh;

    context_destroy(s_context);
}
