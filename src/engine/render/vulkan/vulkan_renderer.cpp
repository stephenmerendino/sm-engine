#include "engine/core/assert.h"
#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/core/file.h"
#include "engine/core/macros.h"
#include "engine/input/input.h"
#include "engine/math/mat44.h"
#include "engine/render/Camera.h"
#include "engine/render/mesh.h"
#include "engine/render/vertex.h"
#include "engine/render/vulkan/vulkan_commands.h"
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
bool format_has_stencil_component(VkFormat format)
{
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
           (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

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

static
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

static
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

static
std::vector<VkVertexInputBindingDescription> mesh_get_vertex_input_binding_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_binding_descs(mesh->m_vertices[0]);
}

static
std::vector<VkVertexInputAttributeDescription> mesh_get_vertex_input_attr_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_attr_descs(mesh->m_vertices[0]);
}

static
mesh_instance_t mesh_instance_create(context_t& context, mesh_t* mesh, descriptor_set_layout_t& descriptor_set_layout)
{
    mesh_instance_t mesh_instance;
    mesh_instance.mesh = mesh;

    // set up vertex and index buffers
    mesh_instance.vertex_buffer = buffer_create(context, BufferType::kVertexBuffer, mesh_calc_vertex_buffer_size(mesh));
    mesh_instance.index_buffer = buffer_create(context, BufferType::kIndexBuffer, mesh_calc_index_buffer_size(mesh));
    buffer_update(context, mesh_instance.vertex_buffer, context.graphics_command_pool, mesh->m_vertices.data());
    buffer_update(context, mesh_instance.index_buffer, context.graphics_command_pool, mesh->m_indices.data());

    mesh_instance.transform.model = MAT44_IDENTITY;
    mesh_instance.descriptor_set_layout = descriptor_set_layout;
    return mesh_instance;
}

static
void descriptor_set_layout_destroy(context_t& context, descriptor_set_layout_t& layout)
{
    vkDestroyDescriptorSetLayout(context.device.device_handle, layout.handle, nullptr);
}

static
void pipeline_destroy(context_t& context, pipeline_t& pipeline)
{
	vkDestroyPipeline(context.device.device_handle, pipeline.pipeline_handle, nullptr);
    vkDestroyPipelineLayout(context.device.device_handle, pipeline.layout_handle, nullptr);
}

static
void mesh_instance_destroy(context_t& context, mesh_instance_t& rm)
{
    pipeline_destroy(context, rm.pipeline);
    descriptor_set_layout_destroy(context, rm.descriptor_set_layout);
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

static
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

static
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
void render_pass_destroy(context_t& context, render_pass_t& render_pass)
{
    vkDestroyRenderPass(context.device.device_handle, render_pass.handle, nullptr);
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

static
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

static
void pipeline_destroy_shader_stages(context_t& context, pipeline_shader_stages_t& shader_stages)
{
    // cleanup
    for (VkShaderModule shader_module : shader_stages.shaders)
    {
        shader_module_destroy(context, shader_module);
    }
}

static
pipeline_input_assembly_t pipeline_create_input_assembly(VkPrimitiveTopology topology, bool primitive_restart_enable = false)
{
    pipeline_input_assembly_t input_assembly = {};
    input_assembly.input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.input_assembly_info.topology = topology;
    input_assembly.input_assembly_info.primitiveRestartEnable = primitive_restart_enable;
    return input_assembly;
}

static
pipeline_vertex_input_t pipeline_create_vertex_input(mesh_instance_t& mesh_instance)
{
    pipeline_vertex_input_t vertex_input; 
    vertex_input.input_binding_descs = mesh_get_vertex_input_binding_descs(mesh_instance.mesh);
    vertex_input.input_attr_descs = mesh_get_vertex_input_attr_descs(mesh_instance.mesh);
    vertex_input.vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertex_input_info.vertexBindingDescriptionCount = (u32)vertex_input.input_binding_descs.size();
    vertex_input.vertex_input_info.pVertexBindingDescriptions = vertex_input.input_binding_descs.data();
    vertex_input.vertex_input_info.vertexAttributeDescriptionCount = (u32)(vertex_input.input_attr_descs.size());
    vertex_input.vertex_input_info.pVertexAttributeDescriptions = vertex_input.input_attr_descs.data();
    return vertex_input;
}

static
pipeline_raster_state_t pipeline_create_raster_state(VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL, 
                                                     VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE, 
                                                     VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT, 
                                                     bool raster_discard_enable = false, 
                                                     bool depth_clamp_enable = false, 
                                                     bool depth_bias_enable = false,
                                                     f32 depth_bias_constant = 0.0f,
                                                     f32 depth_bias_clamp = 0.0f,
                                                     f32 depth_bias_slope = 0.0f,
                                                     f32 line_width = 1.0f)
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

static
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

static
pipeline_multisample_state_t pipeline_create_multisample_state(VkSampleCountFlagBits sample_count, bool sample_shading_enable = false, f32 min_sample_shading = 1.0f)
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

static
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

static
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

static
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

pipeline_layout_t pipeline_create_layout(context_t& context, mesh_instance_t& rm)
{
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts = { rm.descriptor_set_layout.handle };
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

static
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

static
void descriptor_pool_add_size(descriptor_pool_sizes_t& pool_sizes, VkDescriptorType type, u32 count)
{
    VkDescriptorPoolSize pool_size = {}; 
    pool_size.type = type;
    pool_size.descriptorCount = count;
    pool_sizes.pool_sizes.push_back(pool_size);
}

static
descriptor_pool_t descriptor_pool_create(context_t& context, descriptor_pool_sizes_t& pool_sizes, u32 max_sets)
{
    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = (u32)pool_sizes.pool_sizes.size();
    create_info.pPoolSizes = pool_sizes.pool_sizes.data();
    create_info.maxSets = max_sets;

    descriptor_pool_t descriptor_pool;
    VULKAN_ASSERT(vkCreateDescriptorPool(context.device.device_handle, &create_info, nullptr, &descriptor_pool.descriptor_pool));
    return descriptor_pool;
}

static
void descriptor_pool_destroy(context_t& context, descriptor_pool_t& descriptor_pool)
{
    vkDestroyDescriptorPool(context.device.device_handle, descriptor_pool.descriptor_pool, nullptr);
}

static
void descriptor_pool_reset(context_t& context, descriptor_pool_t& descriptor_pool, VkDescriptorPoolResetFlags flags = 0)
{
    vkResetDescriptorPool(context.device.device_handle, descriptor_pool.descriptor_pool, flags);
}

static
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

struct descriptor_set_layout_bindings_t
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

static
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

static
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

static
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

static
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

static
void descriptor_sets_writes_reset(descriptor_sets_writes_t& descriptor_sets_writes)
{
    descriptor_sets_writes.descriptor_sets_write_infos.clear();
    descriptor_sets_writes.descriptor_sets_writes.clear();
}

static
void descriptor_sets_write(context_t& context, descriptor_sets_writes_t& descriptor_sets_writes)
{
    // hook up write infos to the writes
    for(i32 i = 0; i < descriptor_sets_writes.descriptor_sets_writes.size(); i++)
    {
        switch (descriptor_sets_writes.descriptor_sets_write_infos[i].type)
        {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             descriptor_sets_writes.descriptor_sets_writes[i].pBufferInfo = &descriptor_sets_writes.descriptor_sets_write_infos[i].buffer_write_info; break;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     descriptor_sets_writes.descriptor_sets_writes[i].pImageInfo = &descriptor_sets_writes.descriptor_sets_write_infos[i].image_write_info; break;
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

    vkUpdateDescriptorSets(context.device.device_handle, (u32)descriptor_sets_writes.descriptor_sets_writes.size(), descriptor_sets_writes.descriptor_sets_writes.data(), 0, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static mesh_t* s_viking_room_mesh = nullptr;
static mesh_t* s_world_axes_mesh = nullptr;
static mesh_instance_t s_viking_room_mesh_instance;
static mesh_instance_t s_world_axes_mesh_instance;
static texture_t s_viking_room_texture;
static sampler_t s_viking_room_sampler;

struct renderer_globals_t
{
    std::vector<frame_t> in_flight_frames;
    std::vector<VkFence> swapchain_images_in_flight;
    u32 cur_frame = 0;

    //render_pass_t depth_only_render_pass;
    render_pass_t main_draw_render_pass;
    //render_pass_t post_process_render_pass;
    
    camera_t* main_camera = nullptr;

    bool debug_render = false;
};

static
void renderer_init_resources(context_t& context, renderer_globals_t& globals)
{
    render_pass_reset(globals.main_draw_render_pass);

    VkFormat depth_format = format_find_depth(context);

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
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);

        subpasses_t subpasses;
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::DEPTH, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::RESOLVE, 2, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        subpass_dependencies_t subpass_dependencies;
        subpass_add_dependency(subpass_dependencies, VK_SUBPASS_EXTERNAL, 
                                                     0, 
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                     0,
                                                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        globals.main_draw_render_pass = render_pass_create(context, attachments, subpasses, subpass_dependencies);

        for(i32 i = 0; i < globals.in_flight_frames.size(); i++)
        {
            frame_t& frame = globals.in_flight_frames[i];
            std::vector<VkImageView> attachments { frame.main_draw_color_target.image_view, 
                                                   frame.main_draw_depth_target.image_view, 
                                                   frame.main_draw_resolve_target.image_view };

            frame.main_draw_framebuffer = framebuffer_create(context, globals.main_draw_render_pass, attachments, context.swapchain.extent.width, context.swapchain.extent.height, 1);
        }
    }

    // pipelines
    {
        // Viking Room
        {
            if(VK_NULL_HANDLE != s_viking_room_mesh_instance.pipeline.pipeline_handle)
            {
                pipeline_destroy(context, s_viking_room_mesh_instance.pipeline);
            }

            pipeline_shader_stages_t shader_stages = pipeline_create_shader_stages(context, "shaders/tri-vert.spv", "main", "shaders/tri-frag.spv", "main");
            pipeline_vertex_input_t vertex_input = pipeline_create_vertex_input(s_viking_room_mesh_instance);
            pipeline_input_assembly_t input_assembly = pipeline_create_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
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
            pipeline_layout_t pipeline_layout = pipeline_create_layout(context, s_viking_room_mesh_instance);
            s_viking_room_mesh_instance.pipeline = pipeline_create(context, shader_stages, 
                                                                              vertex_input, 
                                                                              input_assembly, 
                                                                              raster_state, 
                                                                              viewport_state, 
                                                                              multisample_state, 
                                                                              depth_stencil_state, 
                                                                              color_blend_state, 
                                                                              pipeline_layout, 
                                                                              globals.main_draw_render_pass);

            pipeline_destroy_shader_stages(context, shader_stages);
        }

        // World axes
        {
            if(VK_NULL_HANDLE != s_world_axes_mesh_instance.pipeline.pipeline_handle)
            {
                pipeline_destroy(context, s_world_axes_mesh_instance.pipeline);
            }

            pipeline_shader_stages_t shader_stages = pipeline_create_shader_stages(context, "shaders/simple-color-vert.spv", "main", "shaders/simple-color-frag.spv", "main");
            pipeline_vertex_input_t vertex_input = pipeline_create_vertex_input(s_world_axes_mesh_instance);
            pipeline_input_assembly_t input_assembly = pipeline_create_input_assembly(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
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
            pipeline_layout_t pipeline_layout = pipeline_create_layout(context, s_world_axes_mesh_instance);
            s_world_axes_mesh_instance.pipeline = pipeline_create(context, shader_stages, 
                                                                              vertex_input, 
                                                                              input_assembly, 
                                                                              raster_state, 
                                                                              viewport_state, 
                                                                              multisample_state, 
                                                                              depth_stencil_state, 
                                                                              color_blend_state, 
                                                                              pipeline_layout, 
                                                                              globals.main_draw_render_pass);

            pipeline_destroy_shader_stages(context, shader_stages);
        }
    }
}

static
void swapchain_teardown(context_t& context, renderer_globals_t& globals)
{
    render_pass_destroy(context, globals.main_draw_render_pass);
}

static
void swapchain_recreate(context_t& context, renderer_globals_t& globals)
{
    if(context.window->was_closed || context.window->width == 0 || context.window->height == 0)
    {
        return;
    }

    vkDeviceWaitIdle(context.device.device_handle);

    swapchain_teardown(context, globals);
    context_refresh_swapchain(context);
    renderer_init_resources(context, globals);
}

static
frame_t frame_create(context_t& context)
{
    frame_t frame;
    frame.swapchain_image_index = -1;
    frame.swapchain_image_available_semaphore = semaphore_create(context);
    frame.render_finished_semaphore = semaphore_create(context);
    frame.frame_completed_fence = fence_create(context);
    frame.command_buffers = command_buffers_allocate(context, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    VkFormat depth_format = format_find_depth(context);

    // render targets / framebuffer
    frame.main_draw_color_target = texture_create_color_target(context, context.swapchain.format, 
                                                               context.swapchain.extent.width, context.swapchain.extent.height, 
                                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                                                               context.device.max_num_msaa_samples);

    frame.main_draw_depth_target = texture_create_depth_target(context, depth_format, 
                                                               context.swapchain.extent.width, context.swapchain.extent.height, 
                                                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                                                               context.device.max_num_msaa_samples);

    frame.main_draw_resolve_target = texture_create_color_target(context, context.swapchain.format, 
                                                                 context.swapchain.extent.width, context.swapchain.extent.height, 
                                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                                                 VK_SAMPLE_COUNT_1_BIT);

    descriptor_pool_sizes_t pool_sizes;
    descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4);
    descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4);
    frame.descriptor_pool = descriptor_pool_create(context, pool_sizes, 4);

    buffer_t viking_room_shader_inputs_buffer = buffer_create(context, BufferType::kUniformBuffer, sizeof(object_shader_inputs_t));
    buffer_t world_axes_shader_inputs_buffer = buffer_create(context, BufferType::kUniformBuffer, sizeof(object_shader_inputs_t));
    frame.uniform_buffers.push_back(viking_room_shader_inputs_buffer);
    frame.uniform_buffers.push_back(world_axes_shader_inputs_buffer);

    return frame;
}

static
void frame_destroy(context_t& context, frame_t& frame)
{
	for (i32 i = 0; i < (i32)frame.uniform_buffers.size(); i++)
	{
        buffer_destroy(context, frame.uniform_buffers[i]);
	}
    descriptor_pool_destroy(context, frame.descriptor_pool);
    framebuffer_destroy(context, frame.main_draw_framebuffer);
    texture_destroy(context, frame.main_draw_color_target);
    texture_destroy(context, frame.main_draw_depth_target);
    texture_destroy(context, frame.main_draw_resolve_target);
    command_buffers_free(context, frame.command_buffers);
    semaphore_destroy(context, frame.swapchain_image_available_semaphore);
    semaphore_destroy(context, frame.render_finished_semaphore);
    fence_destroy(context, frame.frame_completed_fence);
}

static
void update_uniform_buffers(context_t& context, camera_t& camera, frame_t& frame)
{
	mat44 view = camera_get_view_tranform(camera);
    f32 aspect = (f32)context.swapchain.extent.width / (f32)context.swapchain.extent.height;
	mat44 projection = create_perspective_projection(45.0, 0.01f, 110.0f, aspect);

    {
        s_viking_room_mesh_instance.uniform_buffer = frame.uniform_buffers[0];

        object_shader_inputs_t shader_inputs;
        shader_inputs.mvp = s_viking_room_mesh_instance.transform.model * view * projection;
        buffer_update_data(context, s_viking_room_mesh_instance.uniform_buffer, &shader_inputs);
    }

    {
        s_world_axes_mesh_instance.uniform_buffer = frame.uniform_buffers[1];

        object_shader_inputs_t shader_inputs;
        shader_inputs.mvp = MAT44_IDENTITY * view * projection;
        buffer_update_data(context, s_world_axes_mesh_instance.uniform_buffer, &shader_inputs);
    }
}

static context_t* s_context;
static renderer_globals_t* s_globals = nullptr;

void renderer_init(window_t* app_window)
{
    ASSERT(nullptr != app_window);

    s_context = context_create(app_window);

    s_globals = new renderer_globals_t;
    s_globals->debug_render = is_debug();

    // frames
    s_globals->in_flight_frames.resize(MAX_NUM_FRAMES_IN_FLIGHT);
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
    {
        s_globals->in_flight_frames[i] = frame_create(*s_context);
    }

    // viking room
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        descriptor_set_layout_add_binding(bindings, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        descriptor_set_layout_t descriptor_set_layout = descriptor_set_layout_create(*s_context, bindings);

        // meshes
        s_viking_room_mesh = mesh_load_from_obj("models/viking_room.obj");
        s_viking_room_mesh_instance = mesh_instance_create(*s_context, s_viking_room_mesh, descriptor_set_layout);

        // setup initial position
        translate(s_viking_room_mesh_instance.transform.model, make_vec3(-1.0f, -1.0f, 0.0f));

        // texture and sampler
        s_viking_room_texture = texture_create_from_file(*s_context, "textures/viking_room.png");
        s_viking_room_sampler = sampler_create(*s_context, s_viking_room_texture.num_mips);
    }

    // world axes
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        descriptor_set_layout_add_binding(bindings, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        descriptor_set_layout_t descriptor_set_layout = descriptor_set_layout_create(*s_context, bindings);

        s_world_axes_mesh = mesh_load_axes();
        s_world_axes_mesh_instance = mesh_instance_create(*s_context, s_world_axes_mesh, descriptor_set_layout);
    }

    renderer_init_resources(*s_context, *s_globals);
}

void renderer_set_main_camera(camera_t* camera)
{
    ASSERT(nullptr != camera); 
    s_globals->main_camera = camera;
}

static
VkResult frame_acquire_next_image(context_t& context, frame_t& frame)
{
	// Acquire an image from the swap chain
	VkResult res = vkAcquireNextImageKHR(context.device.device_handle, 
                                         context.swapchain.handle, 
                                         UINT64_MAX, 
                                         frame.swapchain_image_available_semaphore.handle, 
                                         VK_NULL_HANDLE, 
                                         &frame.swapchain_image_index);
    return res; 
}

static
VkResult frame_present(context_t& context, frame_t& frame)
{
	VkSemaphore wait_semaphores[] = { frame.render_finished_semaphore.handle };

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = wait_semaphores;
	VkSwapchainKHR swapchains[] = { context.swapchain.handle };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &frame.swapchain_image_index;
	present_info.pResults = nullptr;

	VkResult res = vkQueuePresentKHR(context.device.graphics_queue, &present_info);
    return res;
}

static
void frame_generate_command_buffers(context_t& context, renderer_globals_t& globals, frame_t& frame)
{
    VkCommandBuffer command_buffer = frame.command_buffers[0];
    vkResetCommandBuffer(command_buffer, 0);

    command_buffer_begin(command_buffer);
    {
        VkOffset2D offset{ 0, 0 };

        VkClearValue color_target_clear;
        color_target_clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkClearValue depth_target_clear;
        depth_target_clear.depthStencil = {1.0f, 0};

        std::vector<VkClearValue> clear_colors;
        clear_colors.push_back(color_target_clear);
        clear_colors.push_back(depth_target_clear);

        command_buffer_begin_render_pass(command_buffer, globals.main_draw_render_pass.handle, frame.main_draw_framebuffer.handle, offset, context.swapchain.extent, clear_colors);
            {
                std::vector<VkDescriptorSet> draw_descriptor_sets = { s_viking_room_mesh_instance.descriptor_sets[0].descriptor_set };
                command_draw_mesh_instance(command_buffer, s_viking_room_mesh_instance, draw_descriptor_sets);
            }
             
            if(globals.debug_render)
            {
                std::vector<VkDescriptorSet> draw_descriptor_sets = { s_world_axes_mesh_instance.descriptor_sets[0].descriptor_set };
                command_draw_mesh_instance(command_buffer, s_world_axes_mesh_instance, draw_descriptor_sets);
            }
        command_buffer_end_render_pass(command_buffer);

        // copy from render target to swap chain for presentation
        command_transition_image_layout(command_buffer, context.swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        command_copy_image(command_buffer, frame.main_draw_resolve_target.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                                           context.swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                                           context.swapchain.extent.width, context.swapchain.extent.height);
        command_transition_image_layout(command_buffer, context.swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
    }
    command_buffer_end(command_buffer);
}

void renderer_update()
{
    if(input_was_key_pressed(KeyCode::KEY_F1))
    {
        s_globals->debug_render = !s_globals->debug_render; 
    }
}

static
void update_descriptors(context_t& context, frame_t& frame)
{
    descriptor_pool_reset(context, frame.descriptor_pool);

    // descriptor sets allocate
    std::vector<descriptor_set_layout_t> viking_room_layouts(1, s_viking_room_mesh_instance.descriptor_set_layout);
    s_viking_room_mesh_instance.descriptor_sets = descriptor_sets_allocate(*s_context, frame.descriptor_pool, viking_room_layouts);

    std::vector<descriptor_set_layout_t> world_axes_layouts(1, s_world_axes_mesh_instance.descriptor_set_layout);
    s_world_axes_mesh_instance.descriptor_sets = descriptor_sets_allocate(*s_context, frame.descriptor_pool, world_axes_layouts);

    // descriptor sets writes
    descriptor_sets_writes_t descriptor_sets_writes;

    descriptor_sets_writes_reset(descriptor_sets_writes);
    descriptor_sets_writes_add_uniform_buffer(descriptor_sets_writes, s_viking_room_mesh_instance.descriptor_sets[0], s_viking_room_mesh_instance.uniform_buffer, 0, 0, 0, 1);
    descriptor_sets_writes_add_combined_image_sampler(descriptor_sets_writes, s_viking_room_mesh_instance.descriptor_sets[0], s_viking_room_texture, s_viking_room_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0, 1);
    descriptor_sets_writes_add_uniform_buffer(descriptor_sets_writes, s_world_axes_mesh_instance.descriptor_sets[0], s_world_axes_mesh_instance.uniform_buffer, 0, 0, 0, 1);
    descriptor_sets_writes_add_combined_image_sampler(descriptor_sets_writes, s_world_axes_mesh_instance.descriptor_sets[0], s_viking_room_texture, s_viking_room_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0, 1);
    descriptor_sets_write(context, descriptor_sets_writes);

}

void renderer_render_frame()
{
    frame_t& frame = s_globals->in_flight_frames[s_globals->cur_frame];

	// Wait for frame to finish in case its still in flight, this blocks on CPU
    fence_wait(*s_context, frame.frame_completed_fence);

	// Acquire an image from the swap chain
	VkResult res = frame_acquire_next_image(*s_context, frame); 
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
        swapchain_recreate(*s_context, *s_globals);
	    return;
	}
	ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);

	update_uniform_buffers(*s_context, *s_globals->main_camera, frame);
    update_descriptors(*s_context, frame);

	if (VK_NULL_HANDLE != s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index].handle)
	{
        fence_wait(*s_context, s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index]);
	}

	s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index] = frame.frame_completed_fence;

	// Submit command buffer
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { frame.swapchain_image_available_semaphore.handle };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

    frame_generate_command_buffers(*s_context, *s_globals, frame);
	submit_info.commandBufferCount = frame.command_buffers.size();
	submit_info.pCommandBuffers = frame.command_buffers.data();

	VkSemaphore signal_semaphores[] = { frame.render_finished_semaphore.handle };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

    fence_reset(*s_context, frame.frame_completed_fence);
	VULKAN_ASSERT(vkQueueSubmit(s_context->device.graphics_queue, 1, &submit_info, frame.frame_completed_fence.handle));

	// Present to screen
    VkResult present_res = frame_present(*s_context, frame);
	if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR || s_context->window->was_resized)
	{
        swapchain_recreate(*s_context, *s_globals);
	}
	else
	{
		VULKAN_ASSERT(res);
	}

    s_globals->cur_frame = (s_globals->cur_frame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
}

void renderer_deinit()
{
    vkDeviceWaitIdle(s_context->device.device_handle);

    swapchain_teardown(*s_context, *s_globals);

    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
        frame_destroy(*s_context, s_globals->in_flight_frames[i]);
	}

    sampler_destroy(*s_context, s_viking_room_sampler);
    texture_destroy(*s_context, s_viking_room_texture);

    mesh_instance_destroy(*s_context, s_world_axes_mesh_instance);
    mesh_instance_destroy(*s_context, s_viking_room_mesh_instance);
    delete s_world_axes_mesh;
    delete s_viking_room_mesh;

    context_destroy(s_context);
}
