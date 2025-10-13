#include "sm/render/vulkan/vk_resources.h"
#include "sm/render/vulkan/vk_debug.h"
#include "sm/render/mesh_data.h"
#include "sm/config.h"
#include "sm/core/string.h"

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

void sm::gpu_mesh_init(render_context_t& context, sm::gpu_mesh_t& out_mesh, sm::mesh_data_t* mesh_data)
{
    // vertex buffer
    {
        size_t vertex_buffer_size = mesh_data_calc_vertex_buffer_size(mesh_data);
        buffer_init(context,
                    out_mesh.vertex_buffer, 
                    vertex_buffer_size, 
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        buffer_upload_data(context, out_mesh.vertex_buffer.buffer, mesh_data->vertices.data, vertex_buffer_size);
    }

    // index buffer
    {
        size_t index_buffer_size = mesh_data_calc_index_buffer_size(mesh_data);
        buffer_init(context, 
                    out_mesh.index_buffer, 
                    index_buffer_size, 
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        buffer_upload_data(context, out_mesh.index_buffer.buffer, mesh_data->indices.data, index_buffer_size);
    }

    out_mesh.num_indices = (u32)mesh_data->indices.cur_size;
}
