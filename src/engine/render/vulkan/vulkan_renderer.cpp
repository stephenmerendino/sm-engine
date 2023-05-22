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
renderable_mesh_t renderable_mesh_create(context_t& context, mesh_t* mesh, descriptor_set_layout_t& descriptor_set_layout)
{
    renderable_mesh_t renderable_mesh;
    renderable_mesh.mesh = mesh;

    // set up vertex and index buffers
    renderable_mesh.vertex_buffer = buffer_create(context, BufferType::kVertexBuffer, mesh_calc_vertex_buffer_size(mesh));
    renderable_mesh.index_buffer = buffer_create(context, BufferType::kIndexBuffer, mesh_calc_index_buffer_size(mesh));
    buffer_update(context, renderable_mesh.vertex_buffer, context.graphics_command_pool, mesh->m_vertices.data());
    buffer_update(context, renderable_mesh.index_buffer, context.graphics_command_pool, mesh->m_indices.data());

    renderable_mesh.transform.model = MAT44_IDENTITY;
    renderable_mesh.descriptor_set_layout = descriptor_set_layout;
    return renderable_mesh;
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
void renderable_mesh_destroy(context_t& context, renderable_mesh_t& rm)
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

struct pipeline_shader_stages_t
{
    std::vector<VkShaderModule> shaders;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
};

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

struct pipeline_input_assembly_t
{
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
};

static
pipeline_input_assembly_t pipeline_create_input_assembly(VkPrimitiveTopology topology, bool primitive_restart_enable = false)
{
    pipeline_input_assembly_t input_assembly = {};
    input_assembly.input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.input_assembly_info.topology = topology;
    input_assembly.input_assembly_info.primitiveRestartEnable = primitive_restart_enable;
    return input_assembly;
}

struct pipeline_vertex_input_t
{
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    std::vector<VkVertexInputBindingDescription> input_binding_descs;
    std::vector<VkVertexInputAttributeDescription> input_attr_descs;
};

static
pipeline_vertex_input_t pipeline_create_vertex_input(renderable_mesh_t& renderable_mesh)
{
    pipeline_vertex_input_t vertex_input; 
    vertex_input.input_binding_descs = mesh_get_vertex_input_binding_descs(renderable_mesh.mesh);
    vertex_input.input_attr_descs = mesh_get_vertex_input_attr_descs(renderable_mesh.mesh);
    vertex_input.vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertex_input_info.vertexBindingDescriptionCount = (u32)vertex_input.input_binding_descs.size();
    vertex_input.vertex_input_info.pVertexBindingDescriptions = vertex_input.input_binding_descs.data();
    vertex_input.vertex_input_info.vertexAttributeDescriptionCount = (u32)(vertex_input.input_attr_descs.size());
    vertex_input.vertex_input_info.pVertexAttributeDescriptions = vertex_input.input_attr_descs.data();
    return vertex_input;
}

struct pipeline_raster_state_t
{
    VkPipelineRasterizationStateCreateInfo raster_state = {};
};

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

struct pipeline_viewport_state_t
{
    VkPipelineViewportStateCreateInfo viewport_state = {};
    VkViewport viewport;
    VkRect2D scissor;
};

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

struct pipeline_multisample_state_t
{
    VkPipelineMultisampleStateCreateInfo multisample_state = {};
};

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

struct pipeline_depth_stencil_state_t
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
};

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

struct pipeline_color_blend_state_t
{
    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
};

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

struct pipeline_layout_t
{
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE; 
};

pipeline_layout_t pipeline_create_layout(context_t& context, renderable_mesh_t& rm)
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static mesh_t* s_viking_room_mesh = nullptr;
static mesh_t* s_world_axes_mesh = nullptr;
static renderable_mesh_t s_viking_room_renderable_mesh;
static renderable_mesh_t s_world_axes_renderable_mesh;
static texture_t s_viking_room_texture;
static sampler_t s_viking_room_sampler;
static std::vector<buffer_t> s_uniform_buffers;

static render_pass_t s_render_pass;

static VkDescriptorPool s_descriptor_pool = VK_NULL_HANDLE;
std::vector<VkDescriptorSet> s_descriptor_sets;

static std::vector<VkFence> s_swapchain_images_in_flight;
std::vector<frame_t> s_in_flight_frames;

static bool s_debug_render = false;

static
void renderer_init_resources(context_t& context)
{
    render_pass_reset(s_render_pass);

    // uniform buffers
	s_uniform_buffers.resize(s_in_flight_frames.size());
	for (i32 i = 0; i < (i32)s_uniform_buffers.size(); i++)
	{
        s_uniform_buffers[i] = buffer_create(context, BufferType::kUniformBuffer, sizeof(mvp_buffer_t));
	}

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

        s_render_pass = render_pass_create(context, attachments, subpasses, subpass_dependencies);

        for(i32 i = 0; i < s_in_flight_frames.size(); i++)
        {
            frame_t& frame = s_in_flight_frames[i];
            std::vector<VkImageView> attachments { frame.main_draw_color_target.image_view, 
                                                   frame.main_draw_depth_target.image_view, 
                                                   frame.main_draw_resolve_target.image_view };

            frame.main_draw_framebuffer = framebuffer_create(context, s_render_pass, attachments, context.swapchain.extent.width, context.swapchain.extent.height, 1);
        }
    }

    // descriptor sets
    {
        struct desciptor_pool_t
        {
            VkDescriptorPool handle;
            std::vector<VkDescriptorPoolSize> pool_sizes;
        };

        std::vector<VkDescriptorPoolSize> pool_sizes;

        VkDescriptorPoolSize uniform_pool_size = {};
        uniform_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_pool_size.descriptorCount = s_in_flight_frames.size();
        pool_sizes.push_back(uniform_pool_size);

        VkDescriptorPoolSize sampler_pool_size = {};
        sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_pool_size.descriptorCount = s_in_flight_frames.size();
        pool_sizes.push_back(sampler_pool_size);

        VkDescriptorPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.poolSizeCount = (u32)pool_sizes.size();
        create_info.pPoolSizes = pool_sizes.data();
        create_info.maxSets = s_in_flight_frames.size();

        VULKAN_ASSERT(vkCreateDescriptorPool(context.device.device_handle, &create_info, nullptr, &s_descriptor_pool));

        // descriptor sets
        std::vector<VkDescriptorSetLayout> layouts(s_in_flight_frames.size(), s_viking_room_renderable_mesh.descriptor_set_layout.handle);

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = s_descriptor_pool;
        alloc_info.descriptorSetCount = s_in_flight_frames.size();
        alloc_info.pSetLayouts = layouts.data();

        s_descriptor_sets.resize(s_in_flight_frames.size());
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

        for(i32 i = 0; i < s_in_flight_frames.size(); i++)
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

            // finish binding the write infos based on type
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

            // do the writes
            vkUpdateDescriptorSets(context.device.device_handle, (u32)descriptor_set_writes.size(), descriptor_set_writes.data(), 0, nullptr);
        }
    }

    // pipelines
    {
        // Viking Room
        {
            if(VK_NULL_HANDLE != s_viking_room_renderable_mesh.pipeline.pipeline_handle)
            {
                pipeline_destroy(context, s_viking_room_renderable_mesh.pipeline);
            }

            pipeline_shader_stages_t shader_stages = pipeline_create_shader_stages(context, "shaders/tri-vert.spv", "main", "shaders/tri-frag.spv", "main");
            pipeline_vertex_input_t vertex_input = pipeline_create_vertex_input(s_viking_room_renderable_mesh);
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
            pipeline_layout_t pipeline_layout = pipeline_create_layout(context, s_viking_room_renderable_mesh);
            s_viking_room_renderable_mesh.pipeline = pipeline_create(context, shader_stages, 
                                                                              vertex_input, 
                                                                              input_assembly, 
                                                                              raster_state, 
                                                                              viewport_state, 
                                                                              multisample_state, 
                                                                              depth_stencil_state, 
                                                                              color_blend_state, 
                                                                              pipeline_layout, 
                                                                              s_render_pass);

            pipeline_destroy_shader_stages(context, shader_stages);
        }

        // World axes
        {
            if(VK_NULL_HANDLE != s_world_axes_renderable_mesh.pipeline.pipeline_handle)
            {
                pipeline_destroy(context, s_world_axes_renderable_mesh.pipeline);
            }

            pipeline_shader_stages_t shader_stages = pipeline_create_shader_stages(context, "shaders/simple-color-vert.spv", "main", "shaders/simple-color-frag.spv", "main");
            pipeline_vertex_input_t vertex_input = pipeline_create_vertex_input(s_world_axes_renderable_mesh);
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
            pipeline_layout_t pipeline_layout = pipeline_create_layout(context, s_world_axes_renderable_mesh);
            s_world_axes_renderable_mesh.pipeline = pipeline_create(context, shader_stages, 
                                                                              vertex_input, 
                                                                              input_assembly, 
                                                                              raster_state, 
                                                                              viewport_state, 
                                                                              multisample_state, 
                                                                              depth_stencil_state, 
                                                                              color_blend_state, 
                                                                              pipeline_layout, 
                                                                              s_render_pass);

            pipeline_destroy_shader_stages(context, shader_stages);
        }
    }
}

static
void swapchain_teardown(context_t& context)
{
    render_pass_destroy(context, s_render_pass);

	for (u32 i = 0; i < s_in_flight_frames.size(); i++)
	{
        buffer_destroy(context, s_uniform_buffers[i]);
	}

    descriptor_pool_destroy(context, s_descriptor_pool);
}

static
void swapchain_recreate(context_t& context)
{
    if(context.window->was_closed || context.window->width == 0 || context.window->height == 0)
    {
        return;
    }

    vkDeviceWaitIdle(context.device.device_handle);

    swapchain_teardown(context);
    context_refresh_swapchain(context);
    renderer_init_resources(context);
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

    return frame;
}

static
void frame_destroy(context_t& context, frame_t& frame)
{
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

    // manually init swapchain images to the correct starting layout
    VkCommandBuffer setup_swapchain_images_commands = command_begin_single_time(*s_context);
    for(i32 i = 0; i < s_context->swapchain.num_images; i++)
    {
        command_transition_image_layout(setup_swapchain_images_commands, s_context->swapchain.images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
    }
    command_end_single_time(*s_context, setup_swapchain_images_commands);

    {
        // set up descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        descriptor_set_layout_t viking_room_descriptor_set_layout;

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

        VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context->device.device_handle, &layout_create_info, nullptr, &viking_room_descriptor_set_layout.handle));

        // meshes
        s_viking_room_mesh = mesh_load_from_obj("models/viking_room.obj");
        s_viking_room_renderable_mesh = renderable_mesh_create(*s_context, s_viking_room_mesh, viking_room_descriptor_set_layout);

        // setup initial position
        translate(s_viking_room_renderable_mesh.transform.model, make_vec3(3.0f, 0.0f, 0.0f));
    }

    {
        // set up descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        descriptor_set_layout_t world_axes_descriptor_set_layout;

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

        VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context->device.device_handle, &layout_create_info, nullptr, &world_axes_descriptor_set_layout.handle));
        s_world_axes_mesh = mesh_load_axes();
        s_world_axes_renderable_mesh = renderable_mesh_create(*s_context, s_world_axes_mesh, world_axes_descriptor_set_layout);
    }

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
void frame_generate_command_buffers(context_t& context, frame_t& frame)
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

        command_buffer_begin_render_pass(command_buffer, s_render_pass.handle, frame.main_draw_framebuffer.handle, offset, context.swapchain.extent, clear_colors);
            {
                std::vector<VkDescriptorSet> draw_descriptor_sets = { s_descriptor_sets[s_cur_frame] };
                command_draw_renderable_mesh(command_buffer, s_viking_room_renderable_mesh, draw_descriptor_sets);
            }
             
            if(s_debug_render)
            {
                std::vector<VkDescriptorSet> draw_descriptor_sets = { s_descriptor_sets[s_cur_frame] };
                command_draw_renderable_mesh(command_buffer, s_world_axes_renderable_mesh, draw_descriptor_sets);
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
        s_debug_render = !s_debug_render; 
    }
}

void renderer_render_frame()
{
    frame_t& frame = s_in_flight_frames[s_cur_frame];

	// Wait for frame to finish in case its still in flight, this blocks on CPU
    fence_wait(*s_context, frame.frame_completed_fence);

	// Acquire an image from the swap chain
	VkResult res = frame_acquire_next_image(*s_context, frame); 
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
        swapchain_recreate(*s_context);
	    return;
	}
	ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);

	update_uniform_buffer(*s_context, *s_camera, s_cur_frame);

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

    frame_generate_command_buffers(*s_context, frame);
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
    vkDeviceWaitIdle(s_context->device.device_handle);

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
