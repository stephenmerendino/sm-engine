#include "engine/render/vulkan/vulkan_commands.h"
#include "engine/render/mesh.h"
#include "engine/render/vulkan/vulkan_types.h"

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

void command_buffers_free(context_t& context, std::vector<VkCommandBuffer>& command_buffers)
{
	vkFreeCommandBuffers(context.device.device_handle, context.graphics_command_pool.handle, (u32)command_buffers.size(), command_buffers.data());
}

void command_buffer_begin(VkCommandBuffer command_buffer)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pInheritanceInfo = nullptr;
    begin_info.flags = 0;
    VULKAN_ASSERT(vkBeginCommandBuffer(command_buffer, &begin_info));
}

void command_buffer_end(VkCommandBuffer command_buffer)
{
    VULKAN_ASSERT(vkEndCommandBuffer(command_buffer));
}

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

void command_end_single_time(context_t& context, VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.device.graphics_queue);

    vkFreeCommandBuffers(context.device.device_handle, context.graphics_command_pool.handle, 1, &command_buffer);
}

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

void command_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, u32 num_mips)
{
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

    // setting up initial swapchain images
    if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        is_valid_transition = true; 

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    else if(old_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        is_valid_transition = true; 

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        is_valid_transition = true; 

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
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
}

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

void command_buffer_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clear_colors)
{
    VkRenderPassBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass;
    begin_info.framebuffer = framebuffer;
    begin_info.renderArea.offset = offset;
    begin_info.renderArea.extent = extent;
    begin_info.clearValueCount = static_cast<u32>(clear_colors.size());
    begin_info.pClearValues = clear_colors.data();

    vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void command_buffer_end_render_pass(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
}

void command_draw_mesh_instance(VkCommandBuffer command_buffer, mesh_instance_t& mesh_instance, const std::vector<VkDescriptorSet>& descriptor_sets)
{
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_instance.pipeline.pipeline_handle);

    VkBuffer vertex_buffers[] = { mesh_instance.vertex_buffer.handle };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, mesh_instance.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_instance.pipeline.layout_handle, 0, (u32)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

    vkCmdDrawIndexed(command_buffer, (u32)mesh_instance.mesh->m_indices.size(), 1, 0, 0, 0);
}

void command_copy_image(VkCommandBuffer command_buffer, VkImage src_image, VkImageLayout src_layout, VkImage dst_image, VkImageLayout dst_layout, u32 width, u32 height, u32 depth)
{
    VkImageCopy copy = {};
    copy.extent = { width, height, depth };
    copy.srcSubresource.mipLevel = 0;
    copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.srcSubresource.layerCount = 1;
    copy.srcSubresource.baseArrayLayer = 0;
    copy.dstSubresource.mipLevel = 0;
    copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.dstSubresource.layerCount = 1;
    copy.dstSubresource.baseArrayLayer = 0;
    vkCmdCopyImage(command_buffer, src_image, src_layout, dst_image, dst_layout, 1, &copy);
}
