#include "sm/render/vulkan/vk_resources.h"
#include "sm/render/vulkan/vk_debug.h"
#include "sm/render/mesh_data.h"
#include "sm/config.h"
#include "sm/core/string.h"
#include "sm/render/shader_compiler.h"

#pragma warning(push)
#pragma warning(disable:4244)
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"
#pragma warning(pop)

using namespace sm;

u32 sm::calculate_num_mips(u32 width, u32 height, u32 depth)
{
    u32 max_dimension = max(width, max(height, depth));
    return (u32)(std::floor(std::log2(max_dimension)) + 1);
}

u32 sm::calculate_num_mips(VkExtent3D size)
{
    return calculate_num_mips(size.width, size.height, size.depth);
}

void sm::texture_init(render_context_t& context, 
                      texture_t& out_texture, 
                      VkFormat format, 
                      VkExtent3D size, 
                      VkImageUsageFlags usage_flags, 
                      VkImageAspectFlags image_aspect, 
                      VkSampleCountFlagBits sample_count, 
                      bool with_mips_enabled)
{
    out_texture.num_mips = with_mips_enabled ? calculate_num_mips(size) : 1;

    // VkImage
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent = size;
    image_create_info.mipLevels = out_texture.num_mips;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage_flags;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = sample_count;
    image_create_info.flags = 0;
    SM_VULKAN_ASSERT(vkCreateImage(context.device, &image_create_info, nullptr, &out_texture.image));

    // VkDeviceMemory
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(context.device, out_texture.image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_supported_memory_type_index(context, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    SM_VULKAN_ASSERT(vkAllocateMemory(context.device, &alloc_info, nullptr, &out_texture.memory));

    vkBindImageMemory(context.device, out_texture.image, out_texture.memory, 0);

    // VkImageView
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = out_texture.image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.subresourceRange.aspectMask = image_aspect;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = out_texture.num_mips;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    SM_VULKAN_ASSERT(vkCreateImageView(context.device, &image_view_create_info, nullptr, &out_texture.image_view));
}

void sm::texture_init_from_file(render_context_t& context, texture_t& out_texture, const char* filename, bool generate_mips)
{
    arena_t* stack_arena = nullptr;
    arena_stack_init(stack_arena, 256);

    // load image pixels
    sm::string_t full_filepath = string_init(stack_arena);
    string_append(full_filepath, TEXTURES_PATH);
    string_append(full_filepath, filename);

    int tex_width;
    int tex_height;
    int tex_channels;
    stbi_uc* pixels = stbi_load(full_filepath.c_str.data, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    // calc num mips
    u32 num_mips = generate_mips ? calculate_num_mips(tex_width, tex_height, 1) : 1;

    // calc memory needed
    size_t bytes_per_pixel = 4; // 4 because of STBI_rgb_alpha
    VkDeviceSize image_size = tex_width * tex_height * bytes_per_pixel; 

    // image
    {
        VkImageCreateInfo image_create_info{};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = nullptr;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        image_create_info.extent.width = tex_width;
        image_create_info.extent.height = tex_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = num_mips;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        u32 queue_family_indices[] = {
            (u32)context.queue_indices.graphics
        };
        image_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
        image_create_info.pQueueFamilyIndices = queue_family_indices;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        SM_VULKAN_ASSERT(vkCreateImage(context.device, &image_create_info, nullptr, &out_texture.image));

        set_debug_name(context, VK_OBJECT_TYPE_IMAGE, (u64)out_texture.image, filename);
    }

    // image memory
    {
        VkMemoryRequirements image_mem_requirements;
        vkGetImageMemoryRequirements(context.device, out_texture.image, &image_mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = image_mem_requirements.size;
        alloc_info.memoryTypeIndex = find_supported_memory_type_index(context, image_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        SM_VULKAN_ASSERT(vkAllocateMemory(context.device, &alloc_info, nullptr, &out_texture.memory));

        SM_VULKAN_ASSERT(vkBindImageMemory(context.device, out_texture.image, out_texture.memory, 0));
    }

    // allocate command buffer
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = context.graphics_command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    SM_VULKAN_ASSERT(vkAllocateCommandBuffers(context.device, &command_buffer_alloc_info, &command_buffer));

    // begin command buffer
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    SM_VULKAN_ASSERT(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    // staging buffer upload + copy
    {
        VkBuffer staging_buffer = VK_NULL_HANDLE;
        VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

        VkBufferCreateInfo staging_buffer_create_info{};
        staging_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_buffer_create_info.pNext = nullptr;
        staging_buffer_create_info.flags = 0;
        staging_buffer_create_info.size = image_size;
        staging_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        u32 queue_family_indices[] = {
            (u32)context.queue_indices.graphics
        };
        staging_buffer_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
        staging_buffer_create_info.pQueueFamilyIndices = queue_family_indices;
        SM_VULKAN_ASSERT(vkCreateBuffer(context.device, &staging_buffer_create_info, nullptr, &staging_buffer));

        VkMemoryRequirements staging_buffer_mem_requirements;
        vkGetBufferMemoryRequirements(context.device, staging_buffer, &staging_buffer_mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = staging_buffer_mem_requirements.size;
        alloc_info.memoryTypeIndex = find_supported_memory_type_index(context, staging_buffer_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        SM_VULKAN_ASSERT(vkAllocateMemory(context.device, &alloc_info, nullptr, &staging_buffer_memory));

        SM_VULKAN_ASSERT(vkBindBufferMemory(context.device, staging_buffer, staging_buffer_memory, 0));

        void* mapped_memory = nullptr;
        SM_VULKAN_ASSERT(vkMapMemory(context.device, staging_buffer_memory, 0, image_size, 0, &mapped_memory));
        memcpy(mapped_memory, pixels, image_size);
        vkUnmapMemory(context.device, staging_buffer_memory);

        // transition the image to transfer dst
        VkImageMemoryBarrier image_memory_barrier{};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = out_texture.image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = num_mips;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr, 
                             1, &image_memory_barrier);

        // do a buffer copy from staging buffer to image
        VkImageSubresourceLayers subresource_layers{};
        subresource_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_layers.mipLevel = 0;
        subresource_layers.baseArrayLayer = 0;
        subresource_layers.layerCount = 1;

        VkExtent3D image_extent{};
        image_extent.depth = 1;
        image_extent.height = tex_height;
        image_extent.width = tex_width;

        VkOffset3D image_offset{};
        image_offset.x = 0;
        image_offset.y = 0;
        image_offset.z = 0;

        VkBufferImageCopy buffer_to_image_copy{};
        buffer_to_image_copy.bufferOffset = 0;
        buffer_to_image_copy.bufferRowLength = tex_width;
        buffer_to_image_copy.bufferImageHeight = tex_height;
        buffer_to_image_copy.imageSubresource = subresource_layers;
        buffer_to_image_copy.imageOffset = image_offset;
        buffer_to_image_copy.imageExtent = image_extent;

        VkBufferImageCopy copy_regions[] = {
            buffer_to_image_copy
        };
        vkCmdCopyBufferToImage(command_buffer, staging_buffer, out_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ARRAY_LEN(copy_regions), copy_regions);
    }

    // generate mip maps for image and setup final image layout
    {
        for(u32 cur_mip_level = 0; cur_mip_level < num_mips; ++cur_mip_level)
        {
            u32 mip_level_read = cur_mip_level;
            u32 mip_level_write = cur_mip_level + 1;

            // transition the mip_level_read to transfer src
            {
                VkImageMemoryBarrier image_mip_read_memory_barrier{};
                image_mip_read_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                image_mip_read_memory_barrier.pNext = nullptr;
                image_mip_read_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                image_mip_read_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                image_mip_read_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_mip_read_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                image_mip_read_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                image_mip_read_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                image_mip_read_memory_barrier.image = out_texture.image;
                image_mip_read_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_mip_read_memory_barrier.subresourceRange.baseMipLevel = mip_level_read;
                image_mip_read_memory_barrier.subresourceRange.levelCount = 1;
                image_mip_read_memory_barrier.subresourceRange.baseArrayLayer = 0;
                image_mip_read_memory_barrier.subresourceRange.layerCount = 1;

                VkImageMemoryBarrier image_mip_barriers[] = {
                    image_mip_read_memory_barrier,
                };
                vkCmdPipelineBarrier(command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    ARRAY_LEN(image_mip_barriers), image_mip_barriers);
            }

            // do actual copy from mip_level_read to mip_level_write
            if(mip_level_write < num_mips)
            {
                VkImageBlit blit_region{};
                blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit_region.srcSubresource.mipLevel = mip_level_read;
                blit_region.srcSubresource.baseArrayLayer = 0;
                blit_region.srcSubresource.layerCount = 1;
                blit_region.srcOffsets[0].x = 0;
                blit_region.srcOffsets[0].y = 0;
                blit_region.srcOffsets[0].z = 0;
                blit_region.srcOffsets[1].x = max(tex_width >> mip_level_read, 1);
                blit_region.srcOffsets[1].y = max(tex_height >> mip_level_read, 1);
                blit_region.srcOffsets[1].z = 1;
                blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit_region.dstSubresource.mipLevel = mip_level_write;
                blit_region.dstSubresource.baseArrayLayer = 0;
                blit_region.dstSubresource.layerCount = 1;
                blit_region.dstOffsets[0].x = 0;
                blit_region.dstOffsets[0].y = 0;
                blit_region.dstOffsets[0].z = 0;
                blit_region.dstOffsets[1].x = max(tex_width >> mip_level_write, 1);
                blit_region.dstOffsets[1].y = max(tex_height >> mip_level_write, 1);
                blit_region.dstOffsets[1].z = 1;

                vkCmdBlitImage(command_buffer,
                               out_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               out_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit_region,
                               VK_FILTER_LINEAR);

            }
        }
                        
        // transition all mip levels to color attachment
        {
            VkImageMemoryBarrier image_to_color_attachment_memory_barrier{};
            image_to_color_attachment_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_to_color_attachment_memory_barrier.pNext = nullptr;
            image_to_color_attachment_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_to_color_attachment_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_to_color_attachment_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_to_color_attachment_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_to_color_attachment_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_to_color_attachment_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_to_color_attachment_memory_barrier.image = out_texture.image;
            image_to_color_attachment_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_to_color_attachment_memory_barrier.subresourceRange.baseMipLevel = 0;
            image_to_color_attachment_memory_barrier.subresourceRange.levelCount = num_mips;
            image_to_color_attachment_memory_barrier.subresourceRange.baseArrayLayer = 0;
            image_to_color_attachment_memory_barrier.subresourceRange.layerCount = 1;

            VkImageMemoryBarrier image_mip_barriers[] = {
                image_to_color_attachment_memory_barrier,
            };
            vkCmdPipelineBarrier(command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                ARRAY_LEN(image_mip_barriers), image_mip_barriers);
        }
    }

    // end and submit command buffer
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo command_submit_info{};
    command_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    command_submit_info.pNext = nullptr;
    command_submit_info.waitSemaphoreCount = 0;
    command_submit_info.pWaitSemaphores = nullptr;
    command_submit_info.pWaitDstStageMask = nullptr;
    VkCommandBuffer command_buffers[] = {
        command_buffer
    };
    command_submit_info.commandBufferCount = ARRAY_LEN(command_buffers);
    command_submit_info.pCommandBuffers = command_buffers;
    command_submit_info.signalSemaphoreCount = 0;
    command_submit_info.pSignalSemaphores = nullptr;

    SM_VULKAN_ASSERT(vkQueueSubmit(context.graphics_queue, 1, &command_submit_info, VK_NULL_HANDLE));

    // image view
    {
        VkComponentMapping components{};
        components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VkImageSubresourceRange subresource_range{};
        subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = num_mips;
        subresource_range.baseArrayLayer = 0;
        subresource_range.layerCount = 1;

        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = nullptr;
        image_view_create_info.flags = 0;
        image_view_create_info.image = out_texture.image;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        image_view_create_info.components = components;
        image_view_create_info.subresourceRange = subresource_range;
        SM_VULKAN_ASSERT(vkCreateImageView(context.device, &image_view_create_info, nullptr, &out_texture.image_view));
    }
}

void sm::texture_release(render_context_t& context, texture_t& texture)
{
    vkDestroyImageView(context.device, texture.image_view, nullptr);
    vkDestroyImage(context.device, texture.image, nullptr);
    vkFreeMemory(context.device, texture.memory, nullptr);
}

void sm::buffer_init(render_context_t& context, 
                     buffer_t& out_buffer,
                     size_t size,
                     VkBufferUsageFlags usage_flags,
                     VkMemoryPropertyFlags memory_flags)
{
    // buffer
    u32 queue_families[] = {
        (u32)context.queue_indices.graphics
    };

    VkBufferCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = ARRAY_LEN(queue_families),
        .pQueueFamilyIndices = queue_families
    };
    SM_VULKAN_ASSERT(vkCreateBuffer(context.device, &create_info, nullptr, &out_buffer.buffer));

    // memory
    VkMemoryRequirements mem_requirements{};
    vkGetBufferMemoryRequirements(context.device, out_buffer.buffer, &mem_requirements);

    u32 memory_type_index = find_supported_memory_type_index(context, mem_requirements.memoryTypeBits, memory_flags);

    VkMemoryAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = memory_type_index
    };
    SM_VULKAN_ASSERT(vkAllocateMemory(context.device, &alloc_info, nullptr, &out_buffer.memory));

    // bind together
    SM_VULKAN_ASSERT(vkBindBufferMemory(context.device, out_buffer.buffer, out_buffer.memory, 0));
}

void sm::buffer_upload_data(render_context_t& context, VkBuffer dst_buffer, void* src_data, size_t src_data_size)
{
    VkBuffer staging_buffer = VK_NULL_HANDLE;

    VkBufferCreateInfo staging_buffer_create_info{};
    staging_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buffer_create_info.pNext = nullptr;
    staging_buffer_create_info.flags = 0;
    staging_buffer_create_info.size = src_data_size;
    staging_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    u32 queue_families[] = {
        (u32)context.queue_indices.graphics
    };
    staging_buffer_create_info.pQueueFamilyIndices = queue_families;
    staging_buffer_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_families);

    SM_VULKAN_ASSERT(vkCreateBuffer(context.device, &staging_buffer_create_info, nullptr, &staging_buffer));

    VkMemoryRequirements staging_buffer_mem_requirements{};
    vkGetBufferMemoryRequirements(context.device, staging_buffer, &staging_buffer_mem_requirements);

    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    // allocate staging memory
    VkMemoryAllocateInfo staging_alloc_info{};
    staging_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_alloc_info.pNext = nullptr;
    staging_alloc_info.allocationSize = staging_buffer_mem_requirements.size;
    staging_alloc_info.memoryTypeIndex = find_supported_memory_type_index(context, staging_buffer_mem_requirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    SM_VULKAN_ASSERT(vkAllocateMemory(context.device, &staging_alloc_info, nullptr, &staging_buffer_memory));
    SM_VULKAN_ASSERT(vkBindBufferMemory(context.device, staging_buffer, staging_buffer_memory, 0));

    // map staging memory and memcpy data into it
    void* gpu_staging_memory = nullptr;
    vkMapMemory(context.device, staging_buffer_memory, 0, staging_buffer_mem_requirements.size, 0, &gpu_staging_memory);
    memcpy(gpu_staging_memory, src_data, src_data_size);
    vkUnmapMemory(context.device, staging_buffer_memory);

    // transfer data to actual buffer
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = context.graphics_command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer buffer_copy_command_buffer = VK_NULL_HANDLE;
    SM_VULKAN_ASSERT(vkAllocateCommandBuffers(context.device, &command_buffer_alloc_info, &buffer_copy_command_buffer));

    {
        VkCommandBufferBeginInfo buffer_copy_command_buffer_begin_info{};
        buffer_copy_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        buffer_copy_command_buffer_begin_info.pNext = nullptr;
        buffer_copy_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        buffer_copy_command_buffer_begin_info.pInheritanceInfo = nullptr;
        SM_VULKAN_ASSERT(vkBeginCommandBuffer(buffer_copy_command_buffer, &buffer_copy_command_buffer_begin_info));

        SCOPED_QUEUE_DEBUG_LABEL(context.graphics_queue, "Staging Buffer Upload", color_gen_random());

        VkBufferCopy buffer_copy{};
        buffer_copy.srcOffset = 0;
        buffer_copy.dstOffset = 0;
        buffer_copy.size = src_data_size;

        VkBufferCopy copy_regions[] = {
            buffer_copy
        };
        vkCmdCopyBuffer(buffer_copy_command_buffer, staging_buffer, dst_buffer, ARRAY_LEN(copy_regions), copy_regions);

        vkEndCommandBuffer(buffer_copy_command_buffer);
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    VkCommandBuffer commands_to_submit[] = {
        buffer_copy_command_buffer
    };
    submit_info.commandBufferCount = ARRAY_LEN(commands_to_submit);
    submit_info.pCommandBuffers = commands_to_submit;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    VkSubmitInfo submit_infos[] = {
        submit_info
    };
    SM_VULKAN_ASSERT(vkQueueSubmit(context.graphics_queue, ARRAY_LEN(submit_infos), submit_infos, nullptr));
    vkQueueWaitIdle(context.graphics_queue);

    vkDestroyBuffer(context.device, staging_buffer, nullptr);
    vkFreeCommandBuffers(context.device, context.graphics_command_pool, ARRAY_LEN(commands_to_submit), commands_to_submit);
}

void sm::buffer_release(render_context_t& context, buffer_t& buffer)
{
    vkFreeMemory(context.device, buffer.memory, nullptr);
    vkDestroyBuffer(context.device, buffer.buffer, nullptr);
}

void sm::gpu_mesh_init(render_context_t& context, const sm::cpu_mesh_t& mesh_data, sm::gpu_mesh_t& out_mesh)
{
    // vertex buffer
    {
        size_t vertex_buffer_size = mesh_data_calc_vertex_buffer_size(mesh_data);
        buffer_init(context,
                    out_mesh.vertex_buffer, 
                    vertex_buffer_size, 
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        buffer_upload_data(context, out_mesh.vertex_buffer.buffer, mesh_data.vertices.data, vertex_buffer_size);
    }

    // index buffer
    {
        size_t index_buffer_size = mesh_data_calc_index_buffer_size(mesh_data);
        buffer_init(context, 
                    out_mesh.index_buffer, 
                    index_buffer_size, 
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        buffer_upload_data(context, out_mesh.index_buffer.buffer, mesh_data.indices.data, index_buffer_size);
    }

    out_mesh.num_indices = (u32)mesh_data.indices.cur_size;
}

static VkPipelineVertexInputStateCreateInfo s_default_vertex_input_state;

static VkVertexInputBindingDescription s_default_vertex_input_binding_description{
    .binding = 0,
    .stride = sizeof(vertex_t),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
};
static VkVertexInputBindingDescription s_default_vertex_input_binding_descriptions[] = { s_default_vertex_input_binding_description };

static VkVertexInputAttributeDescription s_vertex_pos_attribute_description{
    .location = 0,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(vertex_t, pos)
};
static VkVertexInputAttributeDescription s_vertex_uv_attribute_description{
    .location = 1,
    .binding = 0,
    .format = VK_FORMAT_R32G32_SFLOAT,
    .offset = offsetof(vertex_t, uv)
};
static VkVertexInputAttributeDescription s_vertex_color_attribute_description{
    .location = 2,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(vertex_t, color)
};
static VkVertexInputAttributeDescription s_vertex_normal_attribute_description{
    .location = 3,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(vertex_t, normal)
};
static VkVertexInputAttributeDescription s_vertex_input_attributes_descriptions[] = {
    s_vertex_pos_attribute_description,
    s_vertex_uv_attribute_description,
    s_vertex_color_attribute_description,
    s_vertex_normal_attribute_description
};

// input assembly
static VkPipelineInputAssemblyStateCreateInfo s_default_triangle_input_assembly{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
};

// tessellation state
static VkPipelineTessellationStateCreateInfo s_default_no_tesselation_state{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .patchControlPoints = 0
};

static VkPipelineViewportStateCreateInfo s_default_main_window_viewport_state;
static VkViewport s_main_window_viewport;
static VkViewport s_main_viewports[] = { s_main_window_viewport };
static VkRect2D s_default_scissor;
static VkRect2D s_default_scissors[] = { s_default_scissor };

// rasterization state
static VkPipelineRasterizationStateCreateInfo s_default_rasterization_state{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
    .lineWidth = 1.0f
};

// multisample state
static VkPipelineMultisampleStateCreateInfo s_default_multisample_state{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 0.0f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
};

// depth stencil state
static VkPipelineDepthStencilStateCreateInfo s_default_depth_stencil_state{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .front = {
        .failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_NEVER,
        .compareMask = 0,
        .writeMask = 0,
        .reference = 0,
    },
    .back = {
        .failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_NEVER,
        .compareMask = 0,
        .writeMask = 0,
        .reference = 0,
    },
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
};

// dynamic state
static VkPipelineDynamicStateCreateInfo s_default_dynamic_state{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .dynamicStateCount = 0,
    .pDynamicStates = nullptr
};

static arena_t* s_materials_arena = nullptr;

VkDescriptorPool sm::g_global_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorPool sm::g_frame_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorPool sm::g_material_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorPool sm::g_imgui_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorSetLayout sm::g_empty_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout sm::g_global_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout sm::g_frame_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout sm::g_material_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout sm::g_mesh_instance_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout sm::g_post_process_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout sm::g_infinite_grid_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSet sm::g_empty_descriptor_set = VK_NULL_HANDLE;
VkDescriptorSet sm::g_global_descriptor_set = VK_NULL_HANDLE;
VkSampler sm::g_linear_sampler = VK_NULL_HANDLE;

gpu_mesh_t sm::g_viking_room_mesh;
texture_t sm::g_viking_room_diffuse_texture;
VkPipelineLayout sm::g_infinite_grid_forward_pass_pipeline_layout = VK_NULL_HANDLE;
VkPipelineLayout sm::g_post_process_pipeline_layout = VK_NULL_HANDLE;
VkPipeline sm::g_post_process_compute_pipeline = VK_NULL_HANDLE;

material_t* sm::g_viking_room_material = nullptr;
material_t* sm::g_debug_draw_material = nullptr;
material_t* sm::g_debug_draw_material_wireframe = nullptr;

static void pipelines_init(render_context_t& context)
{
    arena_t* temp_shader_arena = arena_init(KiB(50));

    // viking room pipeline
    {
        // shaders
        shader_t* vertex_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::VERTEX, "simple-material.vs.hlsl", "main", &vertex_shader));

        shader_t* pixel_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::PIXEL, "simple-material.ps.hlsl", "main", &pixel_shader));

        VkShaderModuleCreateInfo vertex_shader_module_create_info{};
        vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertex_shader_module_create_info.pNext = nullptr;
        vertex_shader_module_create_info.flags = 0;
        vertex_shader_module_create_info.codeSize = vertex_shader->bytecode.cur_size;
        vertex_shader_module_create_info.pCode = (u32*)vertex_shader->bytecode.data;

        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(context.device, &vertex_shader_module_create_info, nullptr, &vertex_shader_module));

        VkShaderModuleCreateInfo pixel_shader_module_create_info{};
        pixel_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        pixel_shader_module_create_info.pNext = nullptr;
        pixel_shader_module_create_info.flags = 0;
        pixel_shader_module_create_info.codeSize = pixel_shader->bytecode.cur_size;
        pixel_shader_module_create_info.pCode = (u32*)pixel_shader->bytecode.data;

        VkShaderModule pixel_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(context.device, &pixel_shader_module_create_info, nullptr, &pixel_shader_module));

        VkPipelineShaderStageCreateInfo vertex_stage{};
        vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_stage.pNext = nullptr;
        vertex_stage.flags = 0;
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.module = vertex_shader_module;
        vertex_stage.pName = vertex_shader->entry_name;
        vertex_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo pixel_stage{};
        pixel_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pixel_stage.pNext = nullptr;
        pixel_stage.flags = 0;
        pixel_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pixel_stage.module = pixel_shader_module;
        pixel_stage.pName = pixel_shader->entry_name;
        pixel_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo shader_stages[] = {
            vertex_stage,
            pixel_stage
        };

        // multisample state
        VkPipelineMultisampleStateCreateInfo multisample_state = s_default_multisample_state;
        multisample_state.rasterizationSamples = context.max_msaa_samples;

        // color blend state
        VkPipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.pNext = nullptr;
        color_blend_state.flags = 0;
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.logicOp = VK_LOGIC_OP_CLEAR;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
        color_blend_attachment_state.blendEnable = VK_FALSE;
        color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                      VK_COLOR_COMPONENT_G_BIT |
                                                      VK_COLOR_COMPONENT_B_BIT |
                                                      VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment_states[] = {
            color_blend_attachment_state
        };

        color_blend_state.attachmentCount = ARRAY_LEN(color_blend_attachment_states);
        color_blend_state.pAttachments = color_blend_attachment_states;

        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        VkFormat color_formats[] = {
            context.default_color_format
        };

        VkPipelineRenderingCreateInfo pipeline_rendering_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.pNext = nullptr,
            .viewMask = 0,
			.colorAttachmentCount = ARRAY_LEN(color_formats),
			.pColorAttachmentFormats = color_formats,
			.depthAttachmentFormat = context.default_depth_format,
			.stencilAttachmentFormat = VK_FORMAT_UNDEFINED 
        };

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = &pipeline_rendering_create_info;
        pipeline_create_info.flags = 0;
        pipeline_create_info.stageCount = ARRAY_LEN(shader_stages);
        pipeline_create_info.pStages = shader_stages;
        pipeline_create_info.pVertexInputState = &s_default_vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &s_default_triangle_input_assembly;
        pipeline_create_info.pTessellationState = &s_default_no_tesselation_state;
        pipeline_create_info.pViewportState = &s_default_main_window_viewport_state;
        pipeline_create_info.pRasterizationState = &s_default_rasterization_state;
        pipeline_create_info.pMultisampleState = &multisample_state;
        pipeline_create_info.pDepthStencilState = &s_default_depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pDynamicState = &s_default_dynamic_state;
        pipeline_create_info.layout = g_viking_room_material->pipeline_layouts[(u32)render_pass_t::FORWARD_PASS];
        pipeline_create_info.renderPass = VK_NULL_HANDLE;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        VkGraphicsPipelineCreateInfo pipeline_create_infos[] = {
            pipeline_create_info
        };
        SM_VULKAN_ASSERT(
            vkCreateGraphicsPipelines(context.device, 
                                      VK_NULL_HANDLE, 
                                      ARRAY_LEN(pipeline_create_infos), 
                                      pipeline_create_infos, 
                                      nullptr, 
                                      &g_viking_room_material->pipelines[(u32)render_pass_t::FORWARD_PASS])
        );

        vkDestroyShaderModule(context.device, vertex_shader_module, nullptr);
        vkDestroyShaderModule(context.device, pixel_shader_module, nullptr);
    }

    // debug draw pipeline
    {
        // shaders
        shader_t* vertex_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::VERTEX, "debug-draw.vs.hlsl", "main", &vertex_shader));

        shader_t* pixel_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::PIXEL, "debug-draw.ps.hlsl", "main", &pixel_shader));

        VkShaderModuleCreateInfo vertex_shader_module_create_info{};
        vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertex_shader_module_create_info.pNext = nullptr;
        vertex_shader_module_create_info.flags = 0;
        vertex_shader_module_create_info.codeSize = vertex_shader->bytecode.cur_size;
        vertex_shader_module_create_info.pCode = (u32*)vertex_shader->bytecode.data;

        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(context.device, &vertex_shader_module_create_info, nullptr, &vertex_shader_module));

        VkShaderModuleCreateInfo pixel_shader_module_create_info{};
        pixel_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        pixel_shader_module_create_info.pNext = nullptr;
        pixel_shader_module_create_info.flags = 0;
        pixel_shader_module_create_info.codeSize = pixel_shader->bytecode.cur_size;
        pixel_shader_module_create_info.pCode = (u32*)pixel_shader->bytecode.data;

        VkShaderModule pixel_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(context.device, &pixel_shader_module_create_info, nullptr, &pixel_shader_module));

        VkPipelineShaderStageCreateInfo vertex_stage{};
        vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_stage.pNext = nullptr;
        vertex_stage.flags = 0;
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.module = vertex_shader_module;
        vertex_stage.pName = vertex_shader->entry_name;
        vertex_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo pixel_stage{};
        pixel_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pixel_stage.pNext = nullptr;
        pixel_stage.flags = 0;
        pixel_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pixel_stage.module = pixel_shader_module;
        pixel_stage.pName = pixel_shader->entry_name;
        pixel_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo shader_stages[] = {
            vertex_stage,
            pixel_stage
        };

        // color blend state
        VkPipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.pNext = nullptr;
        color_blend_state.flags = 0;
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.logicOp = VK_LOGIC_OP_CLEAR;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
        color_blend_attachment_state.blendEnable = VK_TRUE;
        color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment_states[] = {
            color_blend_attachment_state
        };

        color_blend_state.attachmentCount = ARRAY_LEN(color_blend_attachment_states);
        color_blend_state.pAttachments = color_blend_attachment_states;

        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        VkFormat color_formats[] = {
            context.default_color_format,
        };

        VkFormat depth_format = context.default_depth_format;

        VkPipelineRenderingCreateInfo pipeline_rendering_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = ARRAY_LEN(color_formats),
            .pColorAttachmentFormats = color_formats,
            .depthAttachmentFormat = depth_format,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
        };

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = &pipeline_rendering_create_info;
        pipeline_create_info.flags = 0;
        pipeline_create_info.stageCount = ARRAY_LEN(shader_stages);
        pipeline_create_info.pStages = shader_stages;
        pipeline_create_info.pVertexInputState = &s_default_vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &s_default_triangle_input_assembly;
        pipeline_create_info.pTessellationState = &s_default_no_tesselation_state;
        pipeline_create_info.pViewportState = &s_default_main_window_viewport_state;
        pipeline_create_info.pRasterizationState = &s_default_rasterization_state;
        pipeline_create_info.pMultisampleState = &s_default_multisample_state;
        pipeline_create_info.pDepthStencilState = &s_default_depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pDynamicState = &s_default_dynamic_state;
        pipeline_create_info.layout = g_debug_draw_material->pipeline_layouts[(u32)render_pass_t::DEBUG_PASS];
        pipeline_create_info.renderPass = VK_NULL_HANDLE;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        VkGraphicsPipelineCreateInfo pipeline_create_info_wireframe{};
        pipeline_create_info_wireframe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info_wireframe.pNext = &pipeline_rendering_create_info;
        pipeline_create_info_wireframe.flags = 0;
        pipeline_create_info_wireframe.stageCount = ARRAY_LEN(shader_stages);
        pipeline_create_info_wireframe.pStages = shader_stages;
        pipeline_create_info_wireframe.pVertexInputState = &s_default_vertex_input_state;
        pipeline_create_info_wireframe.pInputAssemblyState = &s_default_triangle_input_assembly;
        pipeline_create_info_wireframe.pTessellationState = &s_default_no_tesselation_state;
        pipeline_create_info_wireframe.pViewportState = &s_default_main_window_viewport_state;

        VkPipelineRasterizationStateCreateInfo wireframe_info = s_default_rasterization_state;
        wireframe_info.polygonMode = VK_POLYGON_MODE_LINE;
        pipeline_create_info_wireframe.pRasterizationState = &wireframe_info;

        pipeline_create_info_wireframe.pMultisampleState = &s_default_multisample_state;
        pipeline_create_info_wireframe.pDepthStencilState = &s_default_depth_stencil_state;
        pipeline_create_info_wireframe.pColorBlendState = &color_blend_state;
        pipeline_create_info_wireframe.pDynamicState = &s_default_dynamic_state;
        pipeline_create_info_wireframe.layout = g_debug_draw_material->pipeline_layouts[(u32)render_pass_t::DEBUG_PASS];
        pipeline_create_info_wireframe.renderPass = VK_NULL_HANDLE;
        pipeline_create_info_wireframe.subpass = 0;
        pipeline_create_info_wireframe.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info_wireframe.basePipelineIndex = 0;

        VkGraphicsPipelineCreateInfo pipeline_create_infos[] = {
            pipeline_create_info,
            pipeline_create_info_wireframe
        };

        VkPipeline debug_draw_pipeline = VK_NULL_HANDLE;
        VkPipeline debug_draw_pipeline_wireframe = VK_NULL_HANDLE;
        VkPipeline pipelines[] = {
            debug_draw_pipeline,
            debug_draw_pipeline_wireframe
        };
        SM_VULKAN_ASSERT(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, ARRAY_LEN(pipeline_create_infos), pipeline_create_infos, nullptr, pipelines));

        vkDestroyShaderModule(context.device, vertex_shader_module, nullptr);
        vkDestroyShaderModule(context.device, pixel_shader_module, nullptr);

        g_debug_draw_material->pipelines[(u32)render_pass_t::DEBUG_PASS] = pipelines[0];
        g_debug_draw_material_wireframe->pipelines[(u32)render_pass_t::DEBUG_PASS] = pipelines[1];
        g_debug_draw_material->descriptor_sets[(u32)render_pass_t::DEBUG_PASS] = g_empty_descriptor_set;
        g_debug_draw_material_wireframe->descriptor_sets[(u32)render_pass_t::DEBUG_PASS] = g_empty_descriptor_set;
    }

    arena_destroy(temp_shader_arena);
}

void sm::material_init(render_context_t& context)
{
	// descriptor pools
	{
		// global descriptor pool
		{
            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1 } 
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = 2;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(context.device, &create_info, nullptr, &g_global_descriptor_pool));
		}

		// frame descriptor pool
		{
            // frame render data, infinite grid, post processing
            u32 num_uniform_buffers_per_frame = 2;
            u32 num_storage_images_per_frame = 2; // post processing input + output
            u32 num_sets_per_frame = 3;

            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_NUM_FRAMES_IN_FLIGHT * num_uniform_buffers_per_frame },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_NUM_FRAMES_IN_FLIGHT * num_storage_images_per_frame } 
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = MAX_NUM_FRAMES_IN_FLIGHT * num_sets_per_frame;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(context.device, &create_info, nullptr, &g_frame_descriptor_pool));
		}

		// material descriptor pool
		{
            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 20 }
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = 20;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(context.device, &create_info, nullptr, &g_material_descriptor_pool));
		}

		// imgui descriptor pool
		{
            const i32 IMGUI_MAX_SETS = 1000;

            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, IMGUI_MAX_SETS }
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = IMGUI_MAX_SETS;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(context.device, &create_info, nullptr, &g_imgui_descriptor_pool));
		}
	}

	// descriptor set layouts
	{
        {
            VkDescriptorSetLayoutCreateInfo create_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = 0,
				.pBindings = nullptr,
            };
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &g_empty_descriptor_set_layout));
        }

		// global descriptor set layout
		{
			VkDescriptorSetLayoutBinding sampler_binding{};
			sampler_binding.binding = 0;
			sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			sampler_binding.descriptorCount = 1;
			sampler_binding.stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				sampler_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &g_global_descriptor_set_layout));
		}

		// frame descriptor set layout
		{
			VkDescriptorSetLayoutBinding uniform_binding{};
			uniform_binding.binding = 0;
			uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniform_binding.descriptorCount = 1;
			uniform_binding.stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				uniform_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &g_frame_descriptor_set_layout));
        }

        // infinite grid descriptor set layout
        {
            VkDescriptorSetLayoutBinding uniform_binding{};
            uniform_binding.binding = 0;
            uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniform_binding.descriptorCount = 1;
            uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
                uniform_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &g_infinite_grid_descriptor_set_layout));
        }

        // post processing descriptor set layout
        {
            VkDescriptorSetLayoutBinding src_image_binding{};
            src_image_binding.binding = 0;
            src_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            src_image_binding.descriptorCount = 1;
            src_image_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding dst_image_binding{};
            dst_image_binding.binding = 1;
            dst_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            dst_image_binding.descriptorCount = 1;
            dst_image_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding params_buffer_binding{};
            params_buffer_binding.binding = 2;
            params_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            params_buffer_binding.descriptorCount = 1;
            params_buffer_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
                src_image_binding,
                dst_image_binding,
                params_buffer_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &g_post_process_descriptor_set_layout));
        }

        // materials descriptor set layout
        {
            VkDescriptorSetLayoutBinding diffuse_texture_binding{};
            diffuse_texture_binding.binding = 0;
            diffuse_texture_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            diffuse_texture_binding.descriptorCount = 1;
            diffuse_texture_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
                diffuse_texture_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &g_material_descriptor_set_layout));
        }

        // mesh instance descriptor set layout
        {
            VkDescriptorSetLayoutBinding uniform_binding{};
            uniform_binding.binding = 0;
            uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniform_binding.descriptorCount = 1;
            uniform_binding.stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
                uniform_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &g_mesh_instance_descriptor_set_layout));
        }
    }

    // descriptor sets
    {
        // empty descriptor set
        {
            VkDescriptorSetLayout descriptor_set_layouts[] = {
               g_empty_descriptor_set_layout 
            };

            VkDescriptorSetAllocateInfo alloc_info{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = g_global_descriptor_pool,
                .descriptorSetCount = ARRAY_LEN(descriptor_set_layouts),
                .pSetLayouts = descriptor_set_layouts
            };

            SM_VULKAN_ASSERT(vkAllocateDescriptorSets(context.device, &alloc_info, &g_empty_descriptor_set));
        }

        // global descriptor set
        {
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = g_global_descriptor_pool;

            VkDescriptorSetLayout descriptor_set_layouts[] = {
               g_global_descriptor_set_layout 
            };
            alloc_info.descriptorSetCount = ARRAY_LEN(descriptor_set_layouts);
            alloc_info.pSetLayouts = descriptor_set_layouts;

            SM_VULKAN_ASSERT(vkAllocateDescriptorSets(context.device, &alloc_info, &g_global_descriptor_set));
        }

        // sampler
        {
            VkSamplerCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            create_info.flags = 0;
            create_info.magFilter = VK_FILTER_LINEAR;
            create_info.minFilter = VK_FILTER_LINEAR;
            create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            create_info.mipLodBias = 0.0f;
            create_info.anisotropyEnable = VK_FALSE;
            create_info.maxAnisotropy = 0.0f;
            create_info.compareEnable = VK_FALSE;
            create_info.compareOp = VK_COMPARE_OP_NEVER;
            create_info.minLod = 0.0f;
            create_info.maxLod = VK_LOD_CLAMP_NONE;
            create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            create_info.unnormalizedCoordinates = VK_FALSE;
            SM_VULKAN_ASSERT(vkCreateSampler(context.device, &create_info, nullptr, &g_linear_sampler));
        }

        // write sampler to global descriptor set
        {
            VkWriteDescriptorSet sampler_write{};
            sampler_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            sampler_write.dstSet = g_global_descriptor_set;
            sampler_write.dstBinding = 0;
            sampler_write.dstArrayElement = 0;
            sampler_write.descriptorCount = 1;
            sampler_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

            VkDescriptorImageInfo image_info{};
            image_info.sampler = g_linear_sampler;
            image_info.imageView = VK_NULL_HANDLE;
            image_info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            
            sampler_write.pImageInfo = &image_info;

            VkWriteDescriptorSet descriptor_set_writes[] = {
                sampler_write
            };
            
            vkUpdateDescriptorSets(context.device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);
        }
    }

    // vertex input
    s_default_vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    s_default_vertex_input_state.pNext = nullptr;
    s_default_vertex_input_state.flags = 0;
    s_default_vertex_input_state.vertexBindingDescriptionCount = ARRAY_LEN(s_default_vertex_input_binding_descriptions);
    s_default_vertex_input_state.pVertexBindingDescriptions = s_default_vertex_input_binding_descriptions;
    s_default_vertex_input_state.vertexAttributeDescriptionCount = ARRAY_LEN(s_vertex_input_attributes_descriptions);
    s_default_vertex_input_state.pVertexAttributeDescriptions = s_vertex_input_attributes_descriptions;

    // viewport state
    s_main_window_viewport.x = 0;
    s_main_window_viewport.y = 0;
    s_main_window_viewport.width = (float)context.swapchain.extent.width;
    s_main_window_viewport.height = (float)context.swapchain.extent.height;
    s_main_window_viewport.minDepth = 0.0f;
    s_main_window_viewport.maxDepth = 1.0f;
    s_main_viewports[0] = { s_main_window_viewport };
    s_default_scissor.offset.x = 0;
    s_default_scissor.offset.y = 0;
    s_default_scissor.extent.width = context.swapchain.extent.width;
    s_default_scissor.extent.height = context.swapchain.extent.height;
    s_default_scissors[0] = { s_default_scissor };
    s_default_main_window_viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    s_default_main_window_viewport_state.pNext = nullptr;
    s_default_main_window_viewport_state.flags = 0;
    s_default_main_window_viewport_state.viewportCount = ARRAY_LEN(s_main_viewports);
    s_default_main_window_viewport_state.pViewports = s_main_viewports;
    s_default_main_window_viewport_state.scissorCount = ARRAY_LEN(s_default_scissors);
    s_default_main_window_viewport_state.pScissors = s_default_scissors;

    s_materials_arena = arena_init(MiB(5));
    g_viking_room_material = arena_alloc_struct_zero(s_materials_arena, material_t);
    g_debug_draw_material = arena_alloc_struct_zero(s_materials_arena, material_t);
    g_debug_draw_material_wireframe = arena_alloc_struct_zero(s_materials_arena, material_t);

    // pipeline layouts
    {
        // viking room pipeline layout
        {
            VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                g_global_descriptor_set_layout,
                g_frame_descriptor_set_layout,
                g_material_descriptor_set_layout,
                g_mesh_instance_descriptor_set_layout
            };

            VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                .pSetLayouts = pipeline_descriptor_set_layouts,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr
            };
            SM_VULKAN_ASSERT(vkCreatePipelineLayout(context.device, &pipeline_layout_create_info, nullptr, &g_viking_room_material->pipeline_layouts[(u32)render_pass_t::FORWARD_PASS]));
        }

        // debug draw pipeline layout
        {
            VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                g_global_descriptor_set_layout,
                g_frame_descriptor_set_layout,
                g_empty_descriptor_set_layout,
                g_mesh_instance_descriptor_set_layout
            };

            VkPushConstantRange push_constant_range{
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(debug_draw_push_constants_t)
            };

            VkPushConstantRange push_constants[] = {
                push_constant_range
            };

            VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                .pSetLayouts = pipeline_descriptor_set_layouts,
                .pushConstantRangeCount = ARRAY_LEN(push_constants),
                .pPushConstantRanges = push_constants 
            };
            SM_VULKAN_ASSERT(vkCreatePipelineLayout(context.device, &pipeline_layout_create_info, nullptr, &g_debug_draw_material->pipeline_layouts[(u32)render_pass_t::DEBUG_PASS]));

            // copy over pipeline layout to the wireframe material
            g_debug_draw_material_wireframe->pipeline_layouts[(u32)render_pass_t::DEBUG_PASS] = g_debug_draw_material->pipeline_layouts[(u32)render_pass_t::DEBUG_PASS];
        }

        // infinite grid pipeline layout
        {
            VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                g_infinite_grid_descriptor_set_layout
            };

            VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                .pSetLayouts = pipeline_descriptor_set_layouts,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr
            };
            SM_VULKAN_ASSERT(vkCreatePipelineLayout(context.device, &pipeline_layout_create_info, nullptr, &g_infinite_grid_forward_pass_pipeline_layout));
        }

        // post processing pipeline layout
        {
            VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                g_post_process_descriptor_set_layout,
                g_frame_descriptor_set_layout
            };

            VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                .pSetLayouts = pipeline_descriptor_set_layouts,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr
            };
            SM_VULKAN_ASSERT(vkCreatePipelineLayout(context.device, &pipeline_layout_create_info, nullptr, &g_post_process_pipeline_layout));
        }
    }

    // post processing
    {
        arena_t* temp_shader_arena = arena_init(KiB(50));
        shader_t* compute_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::COMPUTE, "post-processing.cs.hlsl", "main", &compute_shader));

        VkShaderModuleCreateInfo shader_module_create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = compute_shader->bytecode.cur_size,
            .pCode = (u32*)compute_shader->bytecode.data
        };

        VkShaderModule shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(context.device, &shader_module_create_info, nullptr, &shader_module));

        VkPipelineShaderStageCreateInfo post_process_shader_stage_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module,
            .pName = compute_shader->entry_name,
            .pSpecializationInfo = nullptr
        };

        VkComputePipelineCreateInfo post_process_create_info{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = post_process_shader_stage_create_info,
            .layout = g_post_process_pipeline_layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0 
        };
        VkComputePipelineCreateInfo pipeline_create_infos[] = {
            post_process_create_info
        };
        SM_VULKAN_ASSERT(vkCreateComputePipelines(context.device, VK_NULL_HANDLE, ARRAY_LEN(pipeline_create_infos), pipeline_create_infos, nullptr, &g_post_process_compute_pipeline));

        vkDestroyShaderModule(context.device, shader_module, nullptr);
        arena_destroy(temp_shader_arena);
    }

    // viking room
    {
        cpu_mesh_t* viking_room_obj = mesh_data_init_from_obj(s_materials_arena, "viking_room.obj");
        gpu_mesh_init(context, *viking_room_obj, g_viking_room_mesh);

        // viking room diffuse texture
        {
            texture_init_from_file(context, g_viking_room_diffuse_texture, "viking-room.png", true);
        }

        // viking room descriptor set
        {
            // alloc
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.pNext = nullptr;
            alloc_info.descriptorPool = g_material_descriptor_pool;
            alloc_info.descriptorSetCount = 1;
            VkDescriptorSetLayout descriptor_set_layouts[] = {
                g_material_descriptor_set_layout
            };
            alloc_info.pSetLayouts = descriptor_set_layouts;
            SM_VULKAN_ASSERT(vkAllocateDescriptorSets(context.device, &alloc_info, &g_viking_room_material->descriptor_sets[(u32)render_pass_t::FORWARD_PASS]));

            // update
            VkDescriptorImageInfo descriptor_set_write_image_info{};
            descriptor_set_write_image_info.sampler = VK_NULL_HANDLE;
            descriptor_set_write_image_info.imageView = g_viking_room_diffuse_texture.image_view;
            descriptor_set_write_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet descriptor_set_write{};
            descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_set_write.pNext = nullptr;
            descriptor_set_write.dstSet = g_viking_room_material->descriptor_sets[(u32)render_pass_t::FORWARD_PASS];
            descriptor_set_write.dstBinding = 0;
            descriptor_set_write.dstArrayElement = 0;
            descriptor_set_write.descriptorCount = 1;
            descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptor_set_write.pImageInfo = &descriptor_set_write_image_info;
            descriptor_set_write.pBufferInfo = nullptr;
            descriptor_set_write.pTexelBufferView = nullptr;

            VkWriteDescriptorSet ds_writes[] = {
                descriptor_set_write
            };

            vkUpdateDescriptorSets(context.device, ARRAY_LEN(ds_writes), ds_writes, 0, nullptr);
        }
    }

    pipelines_init(context);
}

void sm::pipelines_recreate(render_context_t& context)
{
	vkDestroyPipeline(context.device, g_viking_room_material->pipelines[(u32)render_pass_t::FORWARD_PASS], nullptr);
	vkDestroyPipeline(context.device, g_debug_draw_material->pipelines[(u32)render_pass_t::DEBUG_PASS], nullptr);
	vkDestroyPipeline(context.device, g_debug_draw_material_wireframe->pipelines[(u32)render_pass_t::DEBUG_PASS], nullptr);
    pipelines_init(context);
}
