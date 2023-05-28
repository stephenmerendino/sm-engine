#include "engine/core/debug.h"
#include "engine/core/file.h"
#include "engine/render/vulkan/vulkan_commands.h"
#include "engine/render/vulkan/vulkan_resource_manager.h"
#include "engine/render/vulkan/vulkan_resources.h"
#include "engine/render/vulkan/vulkan_formats.h"

#define STB_IMAGE_IMPLEMENTATION
#include "engine/thirdparty/stb/stb_image.h"

VkImageView image_view_create(device_t& device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips)
{
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = num_mips;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    VULKAN_ASSERT(vkCreateImageView(device.device_handle, &create_info, nullptr, &image_view));
    return image_view;
}

VkImageView image_view_create(context_t& context, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips)
{
    return image_view_create(context.device, image, format, aspect_flags, num_mips);
}

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
    VkCommandBuffer transition_command = command_begin_single_time(context);
    command_transition_image_layout(transition_command, texture.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture.num_mips);
    command_end_single_time(context, transition_command);

	// Copy pixel data from staging buffer to actual vulkan image
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

texture_t texture_create_color_target(context_t& context, VkFormat format, u32 width, u32 height, VkImageUsageFlags usage, VkSampleCountFlagBits num_samples)
{
    texture_t texture;
    texture.num_mips = 1;
    image_create(context, 
                 width, height, 
                 texture.num_mips, 
                 num_samples, 
                 format, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 usage, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 texture.handle, 
                 texture.memory);
    texture.image_view = image_view_create(context, texture.handle, format, VK_IMAGE_ASPECT_COLOR_BIT, texture.num_mips);
    return texture;
}

texture_t texture_create_depth_target(context_t& context, VkFormat format, u32 width, u32 height, VkImageUsageFlags usage, VkSampleCountFlagBits num_samples)
{
    texture_t texture;
    texture.num_mips = 1;
    image_create(context, 
                 width, height, 
                 texture.num_mips, 
                 num_samples, 
                 format, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 usage, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 texture.handle, 
                 texture.memory);
    texture.image_view = image_view_create(context, texture.handle, format, VK_IMAGE_ASPECT_DEPTH_BIT, texture.num_mips);
    return texture;
}

void texture_destroy(context_t& context, texture_t& texture)
{
    vkDestroyImageView(context.device.device_handle, texture.image_view, nullptr);
    vkDestroyImage(context.device.device_handle, texture.handle, nullptr);
    vkFreeMemory(context.device.device_handle, texture.memory, nullptr); 
}

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

void buffer_destroy(context_t& context, VkBuffer vk_buffer, VkDeviceMemory vk_memory)
{
    vkDestroyBuffer(context.device.device_handle, vk_buffer, nullptr);
    vkFreeMemory(context.device.device_handle, vk_memory, nullptr);
}

void buffer_destroy(context_t& context, buffer_t& buffer)
{
   buffer_destroy(context, buffer.handle, buffer.memory);
}

void buffer_update_data(context_t& context, buffer_t& buffer, void* data, VkDeviceSize gpu_memory_offset)
{
    void* mapped_gpu_mem;
    vkMapMemory(context.device.device_handle, buffer.memory, gpu_memory_offset, buffer.size, 0, &mapped_gpu_mem);
    memcpy(mapped_gpu_mem, data, buffer.size);
    vkUnmapMemory(context.device.device_handle, buffer.memory);
}

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

void sampler_destroy(context_t& context, sampler_t& sampler)
{
    vkDestroySampler(context.device.device_handle, sampler.handle, nullptr);
}

descriptor_set_layout_t descriptor_set_layout_create(context_t& context, descriptor_set_layout_bindings_t& bindings)
{
    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = (u32)bindings.bindings.size();
    layout_create_info.pBindings = bindings.bindings.data();

    descriptor_set_layout_t descriptor_set_layout;
    VULKAN_ASSERT(vkCreateDescriptorSetLayout(context.device.device_handle, &layout_create_info, nullptr, &descriptor_set_layout.handle));
    return descriptor_set_layout;
}

void descriptor_set_layout_destroy(context_t& context, descriptor_set_layout_t& layout)
{
    vkDestroyDescriptorSetLayout(context.device.device_handle, layout.handle, nullptr);
}

void descriptor_set_layout_add_binding(descriptor_set_layout_bindings_t& bindings, u32 binding_index, u32 descriptor_count, VkDescriptorType descriptor_type, VkShaderStageFlagBits shader_stages)
{
    VkDescriptorSetLayoutBinding new_binding = {};
    new_binding.binding = binding_index;
    new_binding.descriptorCount = descriptor_count;
    new_binding.descriptorType = descriptor_type;
    new_binding.stageFlags = shader_stages;
    new_binding.pImmutableSamplers = nullptr;
    bindings.bindings.push_back(new_binding);
}

descriptor_pool_t descriptor_pool_create(context_t& context, descriptor_pool_sizes_t& pool_sizes, u32 max_sets)
{
    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = (u32)pool_sizes.pool_sizes.size();
    create_info.pPoolSizes = pool_sizes.pool_sizes.data();
    create_info.maxSets = max_sets;

    descriptor_pool_t descriptor_pool;
    VULKAN_ASSERT(vkCreateDescriptorPool(context.device.device_handle, &create_info, nullptr, &descriptor_pool.descriptor_pool));
    descriptor_pool.max_num_sets = max_sets;
    return descriptor_pool;
}

void descriptor_pool_destroy(context_t& context, descriptor_pool_t& descriptor_pool)
{
    vkDestroyDescriptorPool(context.device.device_handle, descriptor_pool.descriptor_pool, nullptr);
}

void descriptor_pool_reset(context_t& context, descriptor_pool_t& descriptor_pool, VkDescriptorPoolResetFlags flags)
{
    vkResetDescriptorPool(context.device.device_handle, descriptor_pool.descriptor_pool, flags);
}

void descriptor_pool_add_size(descriptor_pool_sizes_t& pool_sizes, VkDescriptorType type, u32 count)
{
    VkDescriptorPoolSize pool_size = {}; 
    pool_size.type = type;
    pool_size.descriptorCount = count;
    pool_sizes.pool_sizes.push_back(pool_size);
}

std::vector<descriptor_set_t> descriptor_sets_allocate(context_t& context, descriptor_pool_t& descriptor_pool, std::vector<descriptor_set_layout_t>& layouts)
{
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool.descriptor_pool;
    alloc_info.descriptorSetCount = layouts.size();

    std::vector<VkDescriptorSetLayout> vk_layouts(layouts.size());
    for(i32 i = 0; i < vk_layouts.size(); i++)
    {
        vk_layouts[i] = layouts[i].handle;
    }
    alloc_info.pSetLayouts = vk_layouts.data();

    std::vector<VkDescriptorSet> vk_descriptor_sets(layouts.size());
    VULKAN_ASSERT(vkAllocateDescriptorSets(context.device.device_handle, &alloc_info, vk_descriptor_sets.data()));

    std::vector<descriptor_set_t> descriptor_sets(layouts.size());
    for(i32 i = 0; i < vk_layouts.size(); i++)
    {
        descriptor_sets[i].descriptor_set = vk_descriptor_sets[i];
    }

    return descriptor_sets;
}

descriptor_set_t descriptor_set_allocate(context_t& context, descriptor_pool_t& descriptor_pool, descriptor_set_layout_t& layout)
{
    std::vector<descriptor_set_layout_t> layouts(1, layout);
    std::vector<descriptor_set_t> ds = descriptor_sets_allocate(context, descriptor_pool, layouts);
    return ds[0];
}

void descriptor_sets_writes_add_uniform_buffer(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, buffer_t& buffer, u32 buffer_offset, u32 dst_binding, u32 dst_array_element, u32 descriptor_count)
{
    descriptor_set_write_info_t uniform_write_info = {};
    uniform_write_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_write_info.buffer_write_info.buffer = buffer.handle;
    uniform_write_info.buffer_write_info.offset = buffer_offset;
    uniform_write_info.buffer_write_info.range = buffer.size;
    descriptor_sets_writes.descriptor_sets_write_infos.push_back(uniform_write_info);

    VkWriteDescriptorSet uniform_write = {};
    uniform_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniform_write.dstBinding = dst_binding;
    uniform_write.dstSet = descriptor_set.descriptor_set;
    uniform_write.dstArrayElement = dst_array_element;
    uniform_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_write.descriptorCount = descriptor_count;
    descriptor_sets_writes.descriptor_sets_writes.push_back(uniform_write);
}

void descriptor_sets_writes_add_combined_image_sampler(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, texture_t& texture, sampler_t& sampler, VkImageLayout image_layout, u32 dst_binding, u32 dst_array_element, u32 descriptor_count)
{
    descriptor_set_write_info_t combined_sampler_write_info = {};
    combined_sampler_write_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    combined_sampler_write_info.image_write_info.imageView = texture.image_view;
    combined_sampler_write_info.image_write_info.sampler = sampler.handle;
    combined_sampler_write_info.image_write_info.imageLayout = image_layout;
    descriptor_sets_writes.descriptor_sets_write_infos.push_back(combined_sampler_write_info);

    VkWriteDescriptorSet sampler_write = {};
    sampler_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sampler_write.dstSet = descriptor_set.descriptor_set;
    sampler_write.dstBinding = dst_binding;
    sampler_write.dstArrayElement = dst_array_element;
    sampler_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_write.descriptorCount = descriptor_count;
    descriptor_sets_writes.descriptor_sets_writes.push_back(sampler_write);
}

void descriptor_sets_writes_add_sampler(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, sampler_t& sampler, u32 dst_binding, u32 dst_array_element, u32 descriptor_count)
{
    descriptor_set_write_info_t sampler_write_info = {};
    sampler_write_info.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler_write_info.image_write_info.sampler = sampler.handle;
    descriptor_sets_writes.descriptor_sets_write_infos.push_back(sampler_write_info);

    VkWriteDescriptorSet sampler_write = {};
    sampler_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sampler_write.dstSet = descriptor_set.descriptor_set;
    sampler_write.dstBinding = dst_binding;
    sampler_write.dstArrayElement = dst_array_element;
    sampler_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler_write.descriptorCount = descriptor_count;
    descriptor_sets_writes.descriptor_sets_writes.push_back(sampler_write);
}

void descriptor_sets_writes_add_sampled_image(descriptor_sets_writes_t& descriptor_sets_writes, descriptor_set_t& descriptor_set, texture_t& texture, VkImageLayout image_layout, u32 dst_binding, u32 dst_array_element, u32 descriptor_count)
{
    descriptor_set_write_info_t sampled_image_write_info = {};
    sampled_image_write_info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sampled_image_write_info.image_write_info.imageView = texture.image_view;
    sampled_image_write_info.image_write_info.imageLayout = image_layout;
    descriptor_sets_writes.descriptor_sets_write_infos.push_back(sampled_image_write_info);

    VkWriteDescriptorSet sampler_write = {};
    sampler_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sampler_write.dstSet = descriptor_set.descriptor_set;
    sampler_write.dstBinding = dst_binding;
    sampler_write.dstArrayElement = dst_array_element;
    sampler_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sampler_write.descriptorCount = descriptor_count;
    descriptor_sets_writes.descriptor_sets_writes.push_back(sampler_write);
}

void descriptor_sets_write(context_t& context, descriptor_sets_writes_t& descriptor_sets_writes)
{
    // hook up write infos to the writes
    for(i32 i = 0; i < descriptor_sets_writes.descriptor_sets_writes.size(); i++)
    {
        switch (descriptor_sets_writes.descriptor_sets_write_infos[i].type)
        {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             descriptor_sets_writes.descriptor_sets_writes[i].pBufferInfo = &descriptor_sets_writes.descriptor_sets_write_infos[i].buffer_write_info; break;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     descriptor_sets_writes.descriptor_sets_writes[i].pImageInfo = &descriptor_sets_writes.descriptor_sets_write_infos[i].image_write_info; break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:                    descriptor_sets_writes.descriptor_sets_writes[i].pImageInfo = &descriptor_sets_writes.descriptor_sets_write_infos[i].image_write_info; break;
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:              descriptor_sets_writes.descriptor_sets_writes[i].pImageInfo = &descriptor_sets_writes.descriptor_sets_write_infos[i].image_write_info; break;
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

    vkUpdateDescriptorSets(context.device.device_handle, (u32)descriptor_sets_writes.descriptor_sets_writes.size(), descriptor_sets_writes.descriptor_sets_writes.data(), 0, nullptr);
}
void buffer_update(context_t& context, buffer_t& buffer, command_pool_t& command_pool, void* data, VkDeviceSize gpu_memory_offset)
{
    if (buffer.type == BufferType::kUniformBuffer)
    {
        buffer_update_data(context, buffer, data, gpu_memory_offset);
    }
    else if (buffer.type == BufferType::kVertexBuffer || buffer.type == BufferType::kIndexBuffer)
    {
        buffer_t staging_buffer = buffer_create(context, BufferType::kStagingBuffer, buffer.size);

        buffer_update_data(context, staging_buffer, data, gpu_memory_offset);
        command_copy_buffer(context, staging_buffer, buffer, buffer.size);

        buffer_destroy(context, staging_buffer);
    }
}

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

void shader_module_destroy(context_t& context, VkShaderModule shader)
{
    vkDestroyShaderModule(context.device.device_handle, shader, nullptr);
}

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

void render_pass_destroy(context_t& context, render_pass_t& render_pass)
{
    vkDestroyRenderPass(context.device.device_handle, render_pass.handle, nullptr);
}

VkAttachmentDescription attachment_description_create(VkFormat format, VkSampleCountFlagBits sample_count, 
                                                      VkImageLayout initial_layout, VkImageLayout final_layout,
                                                      VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, 
                                                      VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, 
                                                      VkAttachmentDescriptionFlags flags)
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

void render_pass_add_attachment(render_pass_attachments_t& attachments, VkFormat format, VkSampleCountFlagBits sample_count, 
                                                                        VkImageLayout initial_layout, VkImageLayout final_layout,
                                                                        VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, 
                                                                        VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, 
                                                                        VkAttachmentDescriptionFlags flags)
{
        VkAttachmentDescription new_attachment = attachment_description_create(format, sample_count, 
                                                                               initial_layout, final_layout, 
                                                                               load_op, store_op, 
                                                                               stencil_load_op, stencil_store_op, 
                                                                               flags);
        attachments.attachments.push_back(new_attachment);
}

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

void subpass_add_dependency(subpass_dependencies_t& subpass_dependencies, u32 src_subpass,
                                                                          u32 dst_subpass,
                                                                          VkPipelineStageFlags src_stage,
                                                                          VkPipelineStageFlags dst_stage,
                                                                          VkAccessFlags src_access,
                                                                          VkAccessFlags dst_access,
                                                                          VkDependencyFlags dependency_flags)
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

pipeline_shader_stages_t pipeline_create_shader_stages(context_t& context, const char* vert_shader_file, const char* vert_shader_entry, const char* frag_shader_file, const char* frag_shader_entry)
{
    pipeline_shader_stages_t shader_stages; 

    VkShaderModule vert_shader = shader_module_create(context, vert_shader_file);
    VkShaderModule frag_shader = shader_module_create(context, frag_shader_file);

    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
    vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = vert_shader;
    vert_shader_stage_create_info.pName = vert_shader_entry;

    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = frag_shader;
    frag_shader_stage_create_info.pName = frag_shader_entry;

    shader_stages.shaders.push_back(vert_shader);
    shader_stages.shaders.push_back(frag_shader);
    shader_stages.shader_stage_infos.push_back(vert_shader_stage_create_info);
    shader_stages.shader_stage_infos.push_back(frag_shader_stage_create_info);

    return shader_stages;
}

pipeline_shader_stages_t pipeline_create_shader_stages(context_t& context, shader_info_t& vertex_shader_info, shader_info_t& fragment_shader_info)
{
    return pipeline_create_shader_stages(context, vertex_shader_info.shader_filepath, vertex_shader_info.shader_entry, fragment_shader_info.shader_filepath, fragment_shader_info.shader_entry); 
}

pipeline_input_assembly_t pipeline_create_input_assembly(VkPrimitiveTopology topology, bool primitive_restart_enable)
{
    pipeline_input_assembly_t input_assembly = {};
    input_assembly.input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.input_assembly_info.topology = topology;
    input_assembly.input_assembly_info.primitiveRestartEnable = primitive_restart_enable;
    return input_assembly;
}

pipeline_raster_state_t pipeline_create_raster_state(VkPolygonMode polygon_mode, 
                                                     VkFrontFace front_face, 
                                                     VkCullModeFlags cull_mode, 
                                                     bool raster_discard_enable, 
                                                     bool depth_clamp_enable, 
                                                     bool depth_bias_enable,
                                                     f32 depth_bias_constant,
                                                     f32 depth_bias_clamp,
                                                     f32 depth_bias_slope,
                                                     f32 line_width)
{
    pipeline_raster_state_t raster_state;
    raster_state.raster_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_state.raster_state.depthClampEnable = depth_clamp_enable;
    raster_state.raster_state.rasterizerDiscardEnable = raster_discard_enable;
    raster_state.raster_state.polygonMode = polygon_mode;
    raster_state.raster_state.lineWidth = line_width;
    raster_state.raster_state.cullMode = cull_mode;
    raster_state.raster_state.frontFace = front_face;
    raster_state.raster_state.depthBiasEnable = depth_bias_enable;
    raster_state.raster_state.depthBiasConstantFactor = depth_bias_constant;
    raster_state.raster_state.depthBiasClamp = depth_bias_clamp;
    raster_state.raster_state.depthBiasSlopeFactor = depth_bias_slope;
    return raster_state; 
}

void pipeline_create_viewport_state(pipeline_viewport_state_t& viewport_state, f32 x, f32 y, 
                                                                               f32 w, f32 h, 
                                                                               f32 min_depth, f32 max_depth, 
                                                                               i32 scissor_offset_x, i32 scissor_offset_y, 
                                                                               u32 scissor_extent_x, u32 scissor_extent_y)
{
    viewport_state.viewport.x = x;
    viewport_state.viewport.y = y;
    viewport_state.viewport.width = w;
    viewport_state.viewport.height = h;
    viewport_state.viewport.minDepth = min_depth;
    viewport_state.viewport.maxDepth = max_depth;
    viewport_state.scissor.offset = { scissor_offset_x, scissor_offset_y };
    viewport_state.scissor.extent = { scissor_extent_x, scissor_extent_y };

    viewport_state.viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewport_state.pViewports = &viewport_state.viewport;
    viewport_state.viewport_state.viewportCount = 1;
    viewport_state.viewport_state.pScissors = &viewport_state.scissor;
    viewport_state.viewport_state.scissorCount = 1;
}

pipeline_multisample_state_t pipeline_create_multisample_state(VkSampleCountFlagBits sample_count, bool sample_shading_enable, f32 min_sample_shading)
{
    pipeline_multisample_state_t multisample_state;
    multisample_state.multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.multisample_state.sampleShadingEnable = sample_shading_enable;
    multisample_state.multisample_state.rasterizationSamples = sample_count;
    multisample_state.multisample_state.minSampleShading = min_sample_shading;
    multisample_state.multisample_state.pSampleMask = nullptr;
    multisample_state.multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.multisample_state.alphaToOneEnable = VK_FALSE;
    return multisample_state;
}

pipeline_depth_stencil_state_t pipeline_create_depth_stencil_state(bool depth_test_enable, bool depth_write_enable, VkCompareOp depth_compare_op, 
                                                                   bool depth_bounds_test_enable, f32 min_depth_bounds, f32 max_depth_bounds)
{
    pipeline_depth_stencil_state_t depth_stencil_state;
    depth_stencil_state.depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depth_stencil_state.depthTestEnable = depth_test_enable;
    depth_stencil_state.depth_stencil_state.depthWriteEnable = depth_write_enable;
    depth_stencil_state.depth_stencil_state.depthCompareOp = depth_compare_op;
    depth_stencil_state.depth_stencil_state.depthBoundsTestEnable = depth_bounds_test_enable;
    depth_stencil_state.depth_stencil_state.minDepthBounds = min_depth_bounds;
    depth_stencil_state.depth_stencil_state.maxDepthBounds = max_depth_bounds;
    //TODO: Allow stencil to be passed in and set
    depth_stencil_state.depth_stencil_state.stencilTestEnable = VK_FALSE;
    depth_stencil_state.depth_stencil_state.front = {};
    depth_stencil_state.depth_stencil_state.back = {};
    return depth_stencil_state;
}

void pipeline_add_color_blend_attachments(pipeline_color_blend_state_t& color_blend_state, bool blend_enable, 
                                                                                           VkBlendFactor src_color_blend_factor, VkBlendFactor dst_color_blend_factor, VkBlendOp color_blend_op, 
                                                                                           VkBlendFactor src_alpha_blend_Factor, VkBlendFactor dst_alpha_blend_factor, VkBlendOp alpha_blend_op, 
                                                                                           VkColorComponentFlags color_write_mask)
{
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_state.color_blend_attachments.push_back(color_blend_attachment);
}

void pipeline_create_color_blend_state(pipeline_color_blend_state_t& color_blend_state, bool logic_op_enable, VkLogicOp logic_op, f32 blend_constant_0, f32 blend_constant_1, f32 blend_constant_2, f32 blend_constant_3)
{
    color_blend_state.color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.color_blend_state.logicOpEnable = logic_op_enable;
    color_blend_state.color_blend_state.logicOp = logic_op;
    color_blend_state.color_blend_state.blendConstants[0] = blend_constant_0;
    color_blend_state.color_blend_state.blendConstants[1] = blend_constant_1;
    color_blend_state.color_blend_state.blendConstants[2] = blend_constant_2;
    color_blend_state.color_blend_state.blendConstants[3] = blend_constant_3;
    color_blend_state.color_blend_state.attachmentCount = (u32)color_blend_state.color_blend_attachments.size();
    color_blend_state.color_blend_state.pAttachments = color_blend_state.color_blend_attachments.data();
}

pipeline_layout_t pipeline_create_layout(context_t& context, mesh_instance_t& mesh_instance)
{
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts = { 
                                                                  get_renderer_globals()->global_data_ds_layout.handle,
                                                                  get_renderer_globals()->frame_data_ds_layout.handle,
                                                                  get_renderer_globals()->material_data_ds_layout.handle,
                                                                  get_renderer_globals()->mesh_instance_render_data_ds_layout.handle 
                                                                };

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = (u32)descriptor_set_layouts.size();
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    layout_info.pushConstantRangeCount = 0;
    layout_info.pPushConstantRanges = nullptr;

    pipeline_layout_t pipeline_layout;
    VULKAN_ASSERT(vkCreatePipelineLayout(context.device.device_handle, &layout_info, nullptr, &pipeline_layout.pipeline_layout));
    return pipeline_layout;
}

pipeline_t pipeline_create(context_t& context, pipeline_shader_stages_t& shader_stages, 
                                               pipeline_vertex_input_t& vertex_input, 
                                               pipeline_input_assembly_t& input_assembly,
                                               pipeline_raster_state_t& raster_state,
                                               pipeline_viewport_state_t& viewport_state,
                                               pipeline_multisample_state_t& multisample_state,
                                               pipeline_depth_stencil_state_t& depth_stencil_state,
                                               pipeline_color_blend_state_t& color_blend_state,
                                               pipeline_layout_t& pipeline_layout,
                                               render_pass_t& render_pass)
{
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.flags = 0;
    pipeline_info.stageCount = (u32)shader_stages.shader_stage_infos.size();
    pipeline_info.pStages = shader_stages.shader_stage_infos.data();
    pipeline_info.pVertexInputState = &vertex_input.vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly.input_assembly_info;
    pipeline_info.pRasterizationState = &raster_state.raster_state;
    pipeline_info.pViewportState = &viewport_state.viewport_state;
    pipeline_info.pMultisampleState = &multisample_state.multisample_state;
    pipeline_info.pDepthStencilState = &depth_stencil_state.depth_stencil_state;
    pipeline_info.pColorBlendState = &color_blend_state.color_blend_state;
    pipeline_info.pDynamicState = nullptr; //#TODO Enable this pipeline state
    pipeline_info.layout = pipeline_layout.pipeline_layout;
    pipeline_info.renderPass = render_pass.handle;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    pipeline_t pipeline;
    pipeline.layout_handle = pipeline_layout.pipeline_layout;
    VULKAN_ASSERT(vkCreateGraphicsPipelines(context.device.device_handle, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline.pipeline_handle));
    return pipeline;
}

void pipeline_destroy_shader_stages(context_t& context, pipeline_shader_stages_t& shader_stages)
{
    for (VkShaderModule shader_module : shader_stages.shaders)
    {
        shader_module_destroy(context, shader_module);
    }
}

void pipeline_destroy(context_t& context, pipeline_t& pipeline)
{
	vkDestroyPipeline(context.device.device_handle, pipeline.pipeline_handle, nullptr);
    vkDestroyPipelineLayout(context.device.device_handle, pipeline.layout_handle, nullptr);
    pipeline.pipeline_handle = VK_NULL_HANDLE;
    pipeline.layout_handle = VK_NULL_HANDLE;
}

material_t material_create(context_t& context, material_create_info_t& create_info)
{
    material_t material;
    material.shaders = pipeline_create_shader_stages(context, create_info.vertex_shader_info, create_info.fragment_shader_info);
    material.resources = create_info.resources;

    descriptor_set_layout_bindings_t bindings;
    for(material_resource_t& resource : create_info.resources)
    {
        if(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE == resource.descriptor_info.type)
        {
            descriptor_set_layout_add_binding(bindings, resource.descriptor_info.binding_index, 
                                                        resource.descriptor_info.count, 
                                                        resource.descriptor_info.type, 
                                                        resource.descriptor_info.shader_stages);
        }
    }

    material.descriptor_set_layout = descriptor_set_layout_create(context, bindings);
    material.descriptor_set = descriptor_set_allocate(context, get_renderer_globals()->material_data_dp, material.descriptor_set_layout);

    descriptor_sets_writes_t descriptor_sets_writes;
    for(material_resource_t& resource : create_info.resources)
    {
        if(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE == resource.descriptor_info.type)
        {
            descriptor_sets_writes_add_sampled_image(descriptor_sets_writes, material.descriptor_set, 
                                                                             resource.descriptor_resource.texture, 
                                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                                                                             resource.descriptor_info.binding_index, 
                                                                             0, //TODO: need to support arrays in descriptors
                                                                             resource.descriptor_info.count);
        }
    }
    descriptor_sets_write(context, descriptor_sets_writes);

    return material;
}

void material_destroy(context_t& context, material_t& material)
{
    for(i32 i = 0; i < (i32)material.resources.size(); i++)
    {
        texture_destroy(context, material.resources[i].descriptor_resource.texture); 
    }
    descriptor_set_layout_destroy(context, material.descriptor_set_layout);
}

mesh_instance_t mesh_instance_create(context_t& context, mesh_id_t mesh_id, material_t& material)
{
    mesh_instance_t mesh_instance;
    mesh_instance.mesh_id = mesh_id;
    mesh_instance.transform.model = MAT44_IDENTITY;
    mesh_instance.material = material;
    mesh_instance_pipeline_create(context, mesh_instance);
    return mesh_instance;
}

void mesh_instance_destroy(context_t& context, mesh_instance_t& mesh_instance)
{
    pipeline_destroy(context, mesh_instance.pipeline);
    material_destroy(context, mesh_instance.material);
    mesh_release(mesh_instance.mesh_id);
}

void mesh_instance_pipeline_create(context_t& context, mesh_instance_t& mesh_instance)
{
    ASSERT(VK_NULL_HANDLE == mesh_instance.pipeline.pipeline_handle);

    pipeline_shader_stages_t shader_stages = mesh_instance.material.shaders;
    pipeline_vertex_input_t vertex_input = mesh_render_data_get_pipeline_vertex_input(mesh_instance.mesh_id);
    VkPrimitiveTopology mesh_topology = mesh_render_data_get_topology(mesh_instance.mesh_id);
    pipeline_input_assembly_t input_assembly = pipeline_create_input_assembly(mesh_topology);
    pipeline_raster_state_t raster_state = pipeline_create_raster_state();
    pipeline_viewport_state_t viewport_state;
    pipeline_create_viewport_state(viewport_state, 0.0f, 0.0f, 
                                                   context.swapchain.extent.width, context.swapchain.extent.height, 
                                                   0.0f, 1.0f, 
                                                   0, 0, 
                                                   context.swapchain.extent.width, context.swapchain.extent.width);
    pipeline_multisample_state_t multisample_state = pipeline_create_multisample_state(context.device.max_num_msaa_samples);
    pipeline_depth_stencil_state_t depth_stencil_state = pipeline_create_depth_stencil_state(true, true, VK_COMPARE_OP_LESS, false, 0.0f, 1.0f);
    pipeline_color_blend_state_t color_blend_state;
    pipeline_add_color_blend_attachments(color_blend_state, false, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 
                                                                   VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 
                                                                   VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    pipeline_create_color_blend_state(color_blend_state, false, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);
    pipeline_layout_t pipeline_layout = pipeline_create_layout(context, mesh_instance);
    mesh_instance.pipeline = pipeline_create(context, shader_stages, 
                                                      vertex_input, 
                                                      input_assembly, 
                                                      raster_state, 
                                                      viewport_state, 
                                                      multisample_state, 
                                                      depth_stencil_state, 
                                                      color_blend_state, 
                                                      pipeline_layout, 
                                                      get_renderer_globals()->main_draw_render_pass);

    // TODO: we are probably messing up the shader stored in the material by doing this
    // pipeline_destroy_shader_stages(context, shader_stages);
}

void mesh_instance_pipeline_refresh(context_t& context, mesh_instance_t& mesh_instance)
{
    if(VK_NULL_HANDLE != mesh_instance.pipeline.pipeline_handle)
    {
        pipeline_destroy(context, mesh_instance.pipeline);
    }
    mesh_instance_pipeline_create(context, mesh_instance);
}


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

void framebuffer_destroy(context_t& context, framebuffer_t& framebuffer)
{
    vkDestroyFramebuffer(context.device.device_handle, framebuffer.handle, nullptr);
}

frame_t frame_create(context_t& context)
{
    frame_t frame;
    frame.swapchain_image_index = -1;
    frame.swapchain_image_available_semaphore = semaphore_create(context);
    frame.render_finished_semaphore = semaphore_create(context);
    frame.frame_completed_fence = fence_create(context);
    frame.command_buffers = command_buffers_allocate(context, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    // render targets / framebuffer
    frame.main_draw_color_target = texture_create_color_target(context, context.swapchain.format, 
                                                               context.swapchain.extent.width, context.swapchain.extent.height, 
                                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                                                               context.device.max_num_msaa_samples);

    frame.main_draw_depth_target = texture_create_depth_target(context, context.device.depth_format, 
                                                               context.swapchain.extent.width, context.swapchain.extent.height, 
                                                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                                                               context.device.max_num_msaa_samples);

    frame.main_draw_resolve_target = texture_create_color_target(context, context.swapchain.format, 
                                                                 context.swapchain.extent.width, context.swapchain.extent.height, 
                                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                                                 VK_SAMPLE_COUNT_1_BIT);

    std::vector<VkImageView> attachments = { frame.main_draw_color_target.image_view, frame.main_draw_depth_target.image_view, frame.main_draw_resolve_target.image_view };
    frame.main_draw_framebuffer = framebuffer_create(context, get_renderer_globals()->main_draw_render_pass, attachments, context.swapchain.extent.width, context.swapchain.extent.height, 1);

    // instance render data descriptor pool
    const i32 INITIAL_MAX_SETS = 100;
    descriptor_pool_sizes_t pool_sizes;
    descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, INITIAL_MAX_SETS);
    descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, INITIAL_MAX_SETS);
    frame.mesh_instance_render_data_descriptor_pool = descriptor_pool_create(context, pool_sizes, INITIAL_MAX_SETS);

    return frame;
}

void frame_destroy(context_t& context, frame_t& frame)
{
	for (i32 i = 0; i < (i32)frame.mesh_instance_render_data.size(); i++)
	{
        mesh_instance_render_data_destroy(context, frame.mesh_instance_render_data[i]);
	}
    descriptor_pool_destroy(context, frame.mesh_instance_render_data_descriptor_pool);
    framebuffer_destroy(context, frame.main_draw_framebuffer);
    texture_destroy(context, frame.main_draw_color_target);
    texture_destroy(context, frame.main_draw_depth_target);
    texture_destroy(context, frame.main_draw_resolve_target);
    command_buffers_free(context, frame.command_buffers);
    semaphore_destroy(context, frame.swapchain_image_available_semaphore);
    semaphore_destroy(context, frame.render_finished_semaphore);
    fence_destroy(context, frame.frame_completed_fence);
}

semaphore_t semaphore_create(context_t& context)
{
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    semaphore_t semaphore;
    VULKAN_ASSERT(vkCreateSemaphore(context.device.device_handle, &semaphore_create_info, nullptr, &semaphore.handle));
    return semaphore;
}

void semaphore_destroy(context_t& context, semaphore_t& semaphore)
{
    vkDestroySemaphore(context.device.device_handle, semaphore.handle, nullptr);
}

fence_t fence_create(context_t& context)
{
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    fence_t fence;
    VULKAN_ASSERT(vkCreateFence(context.device.device_handle, &fence_create_info, nullptr, &fence.handle));
    return fence;
}

void fence_reset(context_t& context, fence_t& fence)
{
    vkResetFences(context.device.device_handle, 1, &fence.handle);
}

void fence_wait(context_t& context, fence_t& fence, u64 timeout)
{
    vkWaitForFences(context.device.device_handle, 1, &fence.handle, VK_TRUE, UINT64_MAX); 
}

void fence_destroy(context_t& context, fence_t& fence)
{
    vkDestroyFence(context.device.device_handle, fence.handle, nullptr);
}

