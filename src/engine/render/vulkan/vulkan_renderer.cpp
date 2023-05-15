#include "engine/core/assert.h"
#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/core/macros.h"
#include "engine/math/mat44.h"
#include "engine/render/mesh.h"
#include "engine/render/vulkan/vulkan_functions.h"
#include "engine/render/vulkan/vulkan_include.h"
#include "engine/render/vulkan/vulkan_renderer.h"
#include "engine/render/window.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

#include <set>
#include <string>

struct instance_t
{
    VkInstance m_vk_handle = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
};

struct surface_t
{
    VkSurfaceKHR m_vk_handle = VK_NULL_HANDLE;
};

struct queue_family_indices_t
{
    static const i32 INVALID = -1;

    i32 m_graphics_family = INVALID;
    i32 m_present_family = INVALID;
};

struct device_t
{
    VkPhysicalDevice m_phys_device_vk_handle = VK_NULL_HANDLE;
    VkDevice m_device_vk_handle = VK_NULL_HANDLE;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;
    VkSampleCountFlagBits m_max_num_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    queue_family_indices_t m_queue_families;
    VkPhysicalDeviceProperties m_phys_device_properties;
};

struct swapchain_details_t 
{
    VkSurfaceCapabilitiesKHR m_capabilities;
    std::vector<VkSurfaceFormatKHR> m_formats;
    std::vector<VkPresentModeKHR> m_present_modes;
};

struct swapchain_t
{
    VkSwapchainKHR m_vk_handle = VK_NULL_HANDLE; 
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent = { 0, 0 };
    u32 m_num_images = 0;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
};

struct command_pool_t
{
    VkCommandPool m_vk_handle = VK_NULL_HANDLE;
    VkQueueFlags m_queue_families;
    VkCommandPoolCreateFlags m_create_flags;
};

enum class BufferType : u8
{
    kVertexBuffer,
    kIndexBuffer,
    kUniformBuffer,
    kStagingBuffer,
    kNumBufferTypes,
    kInvalidBuffer = 0xFF,
};

struct buffer_t
{
    VkBuffer m_vk_handle = VK_NULL_HANDLE;   
    VkDeviceMemory m_vk_memory = VK_NULL_HANDLE;
    size_t m_size;
    BufferType m_type;
};

struct renderable_mesh_t
{
    mesh_t* m_mesh = nullptr;
    buffer_t m_vertex_buffer;
    buffer_t m_index_buffer;
};

struct texture_t
{
    VkImage m_vk_handle = VK_NULL_HANDLE;
    VkDeviceMemory m_vk_memory = VK_NULL_HANDLE;
    VkImageView m_image_view;
    u32 m_num_mips;
};

struct sampler_t
{
    VkSampler m_vk_handle = VK_NULL_HANDLE; 
    u32 m_max_mip_level = 1;
};

struct mvp_buffer_t
{
	mat44 m_model;
	mat44 m_view;
	mat44 m_projection;
};

// vulkan infrastructure
static window_t* s_window = nullptr;
static camera_t* s_camera = nullptr;
static instance_t s_instance;
static surface_t s_surface;
static device_t s_device;
static swapchain_t s_swapchain;
static command_pool_t s_graphics_command_pool;

//  these should be collected into a container at some point
static mesh_t* s_viking_room_mesh = nullptr;
static mesh_t* s_world_axes_mesh = nullptr;
static renderable_mesh_t s_viking_room_renderable_mesh;
static renderable_mesh_t s_world_axes_renderable_mesh;

static texture_t s_viking_room_texture;
static texture_t s_color_target;
static texture_t s_depth_target;

static sampler_t s_sampler;

static std::vector<buffer_t> s_uniform_buffers;

static 
void print_instance_info()
{
    if (!is_debug())
    {
        return;
    }

    // print supported extensions
    {
        u32 num_extensions;
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);

        std::vector<VkExtensionProperties> instance_extensions(num_extensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, instance_extensions.data());

        debug_printf("Supported Instance Extensions\n");
        for (const VkExtensionProperties& p : instance_extensions)
        {
            debug_printf("  Name: %s | Spec Version: %i\n", p.extensionName, p.specVersion);
        }
    }

    // print supported validation layers
    {
        u32 num_layers;
        vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

        std::vector<VkLayerProperties> instance_layers(num_layers);
        vkEnumerateInstanceLayerProperties(&num_layers, instance_layers.data());

        debug_printf("Supported Instance Validation Layers\n");
        for (const VkLayerProperties& l : instance_layers)
        {
            debug_printf("  Name: %s | Desc: %s | Spec Version: %i\n", l.layerName, l.description, l.specVersion);
        }
    }
}

static 
bool check_validation_layer_support()
{
    u32 num_layers;
    vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

    std::vector<VkLayerProperties> instance_layers(num_layers);
    vkEnumerateInstanceLayerProperties(&num_layers, instance_layers.data());

    for (const char* layer_name : VALIDATION_LAYERS)
    {
        bool layer_found = false;

        for (const VkLayerProperties& l : instance_layers)
        {
            if (strcmp(layer_name, l.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            return false;
        }
    }

    return true;
}

static
VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_cb(VkDebugUtilsMessageSeverityFlagBitsEXT      msg_severity,
        VkDebugUtilsMessageTypeFlagsEXT             msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* p_cb_data,
        void*                                       p_user_data)
{
    UNUSED(msg_type);
    UNUSED(p_user_data);

    // filter out verbose and info messages unless we explicitly want them
    if (!VULKAN_VERBOSE)
    {
        if (msg_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ||
                msg_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            return VK_FALSE;
        }
    }

    debug_printf("[Vk");

    switch (msg_type)
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:		debug_printf("-General");		break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:	debug_printf("-Performance");	break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:	debug_printf("-Validation");	break;
    }

    switch (msg_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	debug_printf("-Verbose]");	break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		debug_printf("-Info]");		break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	debug_printf("-Warning]");	break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		debug_printf("-Error]");	break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
                                                                break;
    }

    {
        debug_printf(" %s\n", p_cb_data->pMessage);
    }

    // returning false means we don't abort the Vulkan call that triggered the debug callback
    return VK_FALSE;
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
VkDebugUtilsMessengerCreateInfoEXT setup_debug_messenger_create_info(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
    VkDebugUtilsMessengerCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    create_info.pfnUserCallback = callback;
    create_info.pUserData = nullptr;

    return create_info;
}

static
instance_t instance_create()
{
    load_vulkan_global_funcs();

    print_instance_info();

    // app info
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Workbench";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    // create info
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    // extensions
    create_info.ppEnabledExtensionNames = INSTANCE_EXTENSIONS.data();
    create_info.enabledExtensionCount = (u32)INSTANCE_EXTENSIONS.size();

    // validation layers
    if (ENABLE_VALIDATION_LAYERS)
    {
        ASSERT(check_validation_layer_support());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        create_info.enabledLayerCount = (u32)VALIDATION_LAYERS.size();

        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = setup_debug_messenger_create_info(vulkan_debug_cb);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_create_info;

        if (ENABLE_VALIDATION_BEST_PRACTICES)
        {
            VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
            VkValidationFeaturesEXT features = {};
            features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            features.enabledValidationFeatureCount = 1;
            features.pEnabledValidationFeatures = enables;
            debug_messenger_create_info.pNext = &features;
        }
    }

    instance_t vulkan_instance = {};
    VULKAN_ASSERT(vkCreateInstance(&create_info, nullptr, &vulkan_instance.m_vk_handle));

    load_vulkan_instance_funcs(vulkan_instance.m_vk_handle);

    // debug messenger
    if (ENABLE_VALIDATION_LAYERS)
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = setup_debug_messenger_create_info(vulkan_debug_cb);
        VULKAN_ASSERT(vkCreateDebugUtilsMessengerEXT(vulkan_instance.m_vk_handle, &debug_messenger_create_info, nullptr, &vulkan_instance.m_debug_messenger));
    }

    return vulkan_instance;
}

static
surface_t surface_create(instance_t& instance, window_t& window)
{
    VkWin32SurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.hwnd = window.m_handle;
    create_info.hinstance = GetModuleHandle(nullptr);

    surface_t surface;
    VULKAN_ASSERT(vkCreateWin32SurfaceKHR(instance.m_vk_handle, &create_info, nullptr, &surface.m_vk_handle));
    return surface;
}

static 
bool check_physical_device_extension_support(VkPhysicalDevice device)
{
    u32 num_extensions = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

    std::vector<VkExtensionProperties> extensions(num_extensions);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, extensions.data());

    std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
    for (const VkExtensionProperties& extension : extensions)
    {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

static 
swapchain_details_t query_swapchain_support(VkPhysicalDevice device, surface_t& surface)
{
    swapchain_details_t details;

    // surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface.m_vk_handle, &details.m_capabilities);

    // surface formats
    u32 num_surface_formats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface.m_vk_handle, &num_surface_formats, nullptr);

    if (num_surface_formats != 0)
    {
        details.m_formats.resize(num_surface_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface.m_vk_handle, &num_surface_formats, details.m_formats.data());
    }

    // present modes
    u32 num_present_modes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.m_vk_handle, &num_present_modes, nullptr);

    if (num_present_modes != 0)
    {
        details.m_present_modes.resize(num_present_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.m_vk_handle, &num_present_modes, details.m_present_modes.data());
    }

    return details;
}

static 
swapchain_details_t query_swapchain_support(device_t& device, surface_t& surface)
{
    return query_swapchain_support(device.m_phys_device_vk_handle, surface);
}

static
VkSurfaceFormatKHR swapchain_choose_surface_format(std::vector<VkSurfaceFormatKHR> formats)
{
    for (const VkSurfaceFormatKHR& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return formats[0];
}

static
VkPresentModeKHR swapchain_choose_present_mode(std::vector<VkPresentModeKHR> present_modes)
{
    for (const VkPresentModeKHR& mode : present_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static
VkExtent2D swapchain_choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, u32 window_width, u32 window_height)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    VkExtent2D actual_extent = { window_width, window_height };

    actual_extent.width = clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

static
bool has_stencil_component(VkFormat format)
{
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || 
        (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

static
bool queue_family_indices_has_required(queue_family_indices_t& indices)
{
    return indices.m_graphics_family != queue_family_indices_t::INVALID && 
        indices.m_present_family != queue_family_indices_t::INVALID;
}

static 
queue_family_indices_t queue_family_find(VkPhysicalDevice device, surface_t& surface)
{
    queue_family_indices_t indices;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_family_props(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_props.data());

    for (int i = 0; i < (int)queue_family_props.size(); i++)
    {
        const VkQueueFamilyProperties& props = queue_family_props[i];
        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.m_graphics_family = i;
        }

        // query for presentation support on this queue
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface.m_vk_handle, &present_support);

        if (present_support)
        {
            indices.m_present_family = i;
        }

        // has required families
        if(queue_family_indices_has_required(indices))
        {
            break;
        }
    }

    return indices;
}

static 
bool is_physical_device_suitable(VkPhysicalDevice device, surface_t& surface)
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        return false;
    }

    if (features.samplerAnisotropy == VK_FALSE)
    {
        return false;
    }

    bool supports_extensions = check_physical_device_extension_support(device);
    if (!supports_extensions)
    {
        return false;
    }

    swapchain_details_t swapchain_details = query_swapchain_support(device, surface);
    if (swapchain_details.m_formats.empty() || swapchain_details.m_present_modes.empty())
    {
        return false;
    }

    queue_family_indices_t queue_indices = queue_family_find(device, surface);
    if (!queue_family_indices_has_required(queue_indices))
    {
        return false;
    }

    return true;
}

static
VkSampleCountFlagBits get_max_msaa_sample_count(VkPhysicalDeviceProperties props)
{
    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}

static
device_t device_create(instance_t& instance, surface_t& surface)
{
    // physical device
    VkPhysicalDevice selected_physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties selected_physical_device_props = {};
    queue_family_indices_t queue_families = {};
    VkSampleCountFlagBits max_num_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    {
        u32 device_count = 0;
        vkEnumeratePhysicalDevices(s_instance.m_vk_handle, &device_count, nullptr);
        ASSERT(device_count != 0);

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(s_instance.m_vk_handle, &device_count, devices.data());

        if (is_debug())
        {
            debug_printf("Physical Devices:\n");

            for (const VkPhysicalDevice& device : devices)
            {
                VkPhysicalDeviceProperties deviceProps;
                vkGetPhysicalDeviceProperties(device, &deviceProps);

                //VkPhysicalDeviceFeatures deviceFeatures;
                //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

                debug_printf("%s\n", deviceProps.deviceName);
            }
        }

        for (const VkPhysicalDevice& device : devices)
        {
            if (is_physical_device_suitable(device, surface))
            {
                selected_physical_device = device;
                vkGetPhysicalDeviceProperties(selected_physical_device, &selected_physical_device_props);
                queue_families = queue_family_find(selected_physical_device, surface);
                max_num_msaa_samples = get_max_msaa_sample_count(selected_physical_device_props);
                break;
            }
        }

        ASSERT(VK_NULL_HANDLE != selected_physical_device);
    }

    // logical device
    VkDevice logical_device = VK_NULL_HANDLE;
    {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<i32> unique_queue_families = { queue_families.m_graphics_family, queue_families.m_present_family };

        f32 queue_priority = 1.0f;
        for (i32 queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info{};

            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;

            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.queueCreateInfoCount = (u32)queue_create_infos.size();
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = (u32)DEVICE_EXTENSIONS.size();
        create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

        // validation layers are not used on devices anymore, but this is for older vulkan systems that still used it
        if (ENABLE_VALIDATION_LAYERS)
        {
            create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
            create_info.enabledLayerCount = (u32)VALIDATION_LAYERS.size();
        }

        VULKAN_ASSERT(vkCreateDevice(selected_physical_device, &create_info, nullptr, &logical_device));

        load_vulkan_device_funcs(logical_device);
    }

    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    {
        vkGetDeviceQueue(logical_device, queue_families.m_graphics_family, 0, &graphics_queue);
        vkGetDeviceQueue(logical_device, queue_families.m_present_family, 0, &present_queue);
    }

    device_t device = {};
    device.m_phys_device_vk_handle = selected_physical_device;
    device.m_device_vk_handle = logical_device;
    device.m_graphics_queue = graphics_queue;
    device.m_present_queue = present_queue;
    device.m_max_num_msaa_samples = max_num_msaa_samples;
    device.m_queue_families = queue_families;
    device.m_phys_device_properties = selected_physical_device_props;
    return device;
}

static
u32 memory_find_type(device_t& device, u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(device.m_phys_device_vk_handle, &mem_properties);

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
void image_create(device_t& device, u32 width, u32 height, u32 num_mips, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags mem_props, VkImage& out_image, VkDeviceMemory& out_memory)
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

    VULKAN_ASSERT(vkCreateImage(device.m_device_vk_handle, &image_create_info, nullptr, &out_image));

    // Setup backing memory of image
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device.m_device_vk_handle, out_image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_find_type(device, mem_requirements.memoryTypeBits, mem_props);

    VULKAN_ASSERT(vkAllocateMemory(device.m_device_vk_handle, &alloc_info, nullptr, &out_memory));

    vkBindImageMemory(device.m_device_vk_handle, out_image, out_memory, 0);
}

static
VkImageView image_view_create(device_t& device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 num_mips)
{
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = num_mips;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VULKAN_ASSERT(vkCreateImageView(device.m_device_vk_handle, &create_info, nullptr, &imageView));
    return imageView;
}

static
swapchain_t swapchain_create(device_t& device, surface_t& surface, window_t& window)
{
    swapchain_details_t swapchain_details = query_swapchain_support(device, surface);

    VkSurfaceFormatKHR surface_format   = swapchain_choose_surface_format(swapchain_details.m_formats);
    VkPresentModeKHR present_mode       = swapchain_choose_present_mode(swapchain_details.m_present_modes);
    VkExtent2D image_extent             = swapchain_choose_extent(swapchain_details.m_capabilities, window.m_width, window.m_height);

    u32 image_count = swapchain_details.m_capabilities.minImageCount + 1; // one extra image to prevent waiting on driver
    if (swapchain_details.m_capabilities.maxImageCount > 0)
    {
        image_count = min(image_count, swapchain_details.m_capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.surface = surface.m_vk_handle;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.presentMode = present_mode;
    create_info.imageExtent = image_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    queue_family_indices_t indices = device.m_queue_families;
    u32 queue_family_indices[] = { (u32)indices.m_graphics_family, (u32)indices.m_present_family };

    if (indices.m_graphics_family != indices.m_present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swapchain_details.m_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    swapchain_t swapchain = {};
    VULKAN_ASSERT(vkCreateSwapchainKHR(device.m_device_vk_handle, &create_info, nullptr, &swapchain.m_vk_handle));

    vkGetSwapchainImagesKHR(device.m_device_vk_handle, swapchain.m_vk_handle, &swapchain.m_num_images, nullptr);

    swapchain.m_images.resize(swapchain.m_num_images);
    vkGetSwapchainImagesKHR(device.m_device_vk_handle, swapchain.m_vk_handle, &swapchain.m_num_images, swapchain.m_images.data());

    swapchain.m_format = surface_format.format;
    swapchain.m_extent = image_extent;

    // create swapchain image views
    swapchain.m_image_views.resize(swapchain.m_num_images);
    for (size_t i = 0; i < swapchain.m_num_images; i++)
    {
        swapchain.m_image_views[i] = image_view_create(device, swapchain.m_images[i], swapchain.m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    return swapchain;
}

static
command_pool_t command_pool_create(device_t& device, VkQueueFlags queue_families, VkCommandPoolCreateFlags create_flags)
{
    queue_family_indices_t queue_family_indices = device.m_queue_families;

    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = create_flags;

    if (queue_families & VK_QUEUE_GRAPHICS_BIT)
    {
        create_info.queueFamilyIndex = queue_family_indices.m_graphics_family;
    }

    command_pool_t command_pool;
    VULKAN_ASSERT(vkCreateCommandPool(device.m_device_vk_handle, &create_info, nullptr, &command_pool.m_vk_handle));
    return command_pool;
}

static
void buffer_create(device_t& device, VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags, VkBuffer& out_buffer, VkDeviceMemory& out_memory)
{
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage_flags;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VULKAN_ASSERT(vkCreateBuffer(device.m_device_vk_handle, &create_info, nullptr, &out_buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device.m_device_vk_handle, out_buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_find_type(device, mem_requirements.memoryTypeBits, memory_property_flags);

    VULKAN_ASSERT(vkAllocateMemory(device.m_device_vk_handle, &alloc_info, nullptr, &out_memory));

    vkBindBufferMemory(device.m_device_vk_handle, out_buffer, out_memory, 0);
}

static 
buffer_t buffer_create(device_t& device, BufferType type, size_t size)
{
    buffer_t buffer;
    buffer.m_type = type;
    buffer.m_size = size;

    switch(buffer.m_type)
    {
        case BufferType::kVertexBuffer:	 buffer_create(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.m_vk_handle, buffer.m_vk_memory); break;
        case BufferType::kIndexBuffer:	 buffer_create(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.m_vk_handle, buffer.m_vk_memory); break;
        case BufferType::kUniformBuffer: buffer_create(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer.m_vk_handle, buffer.m_vk_memory); break;
        case BufferType::kStagingBuffer: buffer_create(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer.m_vk_handle, buffer.m_vk_memory); break;
        case BufferType::kNumBufferTypes: break;
        case BufferType::kInvalidBuffer: break;
    }

    return buffer;
}

static
void buffer_destroy(device_t& device, VkBuffer vk_buffer, VkDeviceMemory vk_memory)
{
    vkDestroyBuffer(device.m_device_vk_handle, vk_buffer, nullptr);
    vkFreeMemory(device.m_device_vk_handle, vk_memory, nullptr);
}

static
void buffer_destroy(device_t& device, buffer_t& buffer)
{
   buffer_destroy(device, buffer.m_vk_handle, buffer.m_vk_memory);
}

static
void buffer_update_data(device_t& device, buffer_t& buffer, void* data, VkDeviceSize gpu_memory_offset = 0)
{
    void* mapped_gpu_mem;
    vkMapMemory(device.m_device_vk_handle, buffer.m_vk_memory, gpu_memory_offset, buffer.m_size, 0, &mapped_gpu_mem);
    memcpy(mapped_gpu_mem, data, buffer.m_size);
    vkUnmapMemory(device.m_device_vk_handle, buffer.m_vk_memory);
}

static
void queue_flush(VkQueue queue)
{
    vkQueueWaitIdle(queue);
}

static
VkCommandBuffer command_begin_single_time(device_t& device, command_pool_t& pool)
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = pool.m_vk_handle;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device.m_device_vk_handle, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

static
void command_end_single_time(device_t& device, command_pool_t& pool, VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(device.m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    queue_flush(device.m_graphics_queue);

    vkFreeCommandBuffers(device.m_device_vk_handle, pool.m_vk_handle, 1, &command_buffer);
}

static
std::vector<VkCommandBuffer> command_allocate_buffers(device_t& device, command_pool_t& command_pool, VkCommandBufferLevel level, u32 num_buffers)
{
    std::vector<VkCommandBuffer> buffers;
    buffers.resize(num_buffers);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool.m_vk_handle;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (u32)num_buffers;

    VULKAN_ASSERT(vkAllocateCommandBuffers(device.m_device_vk_handle, &alloc_info, buffers.data()));
    return buffers;
}

static
void command_copy_buffer(device_t& device, command_pool_t& command_pool, buffer_t& src, buffer_t& dst, VkDeviceSize size)
{
    VkCommandBuffer copy_command_buffer = command_begin_single_time(device, command_pool);

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(copy_command_buffer, src.m_vk_handle, dst.m_vk_handle, 1, &copy_region);

    command_end_single_time(device, command_pool, copy_command_buffer);
}

static
void command_generate_mip_maps(device_t& device, command_pool_t& command_pool, VkImage image, VkFormat format, u32 width, u32 height, u32 num_mips)
{
    // Check if image format supports linear filtered blitting
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(device.m_phys_device_vk_handle, format, &format_props);
    ASSERT(format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkCommandBuffer command_buffer = command_begin_single_time(device, command_pool);

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

    command_end_single_time(device, command_pool, command_buffer);
}

static
void command_transition_image_layout(device_t& device, command_pool_t& command_pool, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, u32 num_mips)
{
    VkCommandBuffer command_buffer = command_begin_single_time(device, command_pool);

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

    command_end_single_time(device, command_pool, command_buffer);
}

static
void command_copy_buffer_to_image(device_t& device, command_pool_t& command_pool, VkBuffer buffer, VkImage image, u32 width, u32 height)
{
    VkCommandBuffer command_buffer = command_begin_single_time(device, command_pool);

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

    command_end_single_time(device, command_pool, command_buffer);
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
void command_buffer_begin_render_pass(device_t& device, VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, const std::vector<VkClearValue>& clear_colors)
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
void draw_renderable_mesh(renderable_mesh_t& renderable_mesh, VkCommandBuffer command_buffer, VkPipeline pipeline, const std::vector<VkDescriptorSet>& descriptorSets)
{
    //vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->GetPipelineHandle());

    //VkBuffer vertexBuffers[] = { pMesh->GetVertexBuffer() };
    //VkDeviceSize offsets[] = { 0 };
    //vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    //vkCmdBindIndexBuffer(commandBuffer, pMesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
    //vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->GetPipelineLayoutHandle(), 0, (U32)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

    //vkCmdDrawIndexed(commandBuffer, pMesh->GetNumIndices(), 1, 0, 0, 0);
}

static 
void buffer_update(device_t& device, buffer_t& buffer, command_pool_t& command_pool, void* data)
{
    if (buffer.m_type == BufferType::kUniformBuffer)
    {
        buffer_update_data(device, buffer, data);
    }
    else if (buffer.m_type == BufferType::kVertexBuffer || buffer.m_type == BufferType::kIndexBuffer)
    {
        buffer_t staging_buffer = buffer_create(device, BufferType::kStagingBuffer, buffer.m_size);

        buffer_update_data(device, staging_buffer, data);
        command_copy_buffer(device, command_pool, staging_buffer, buffer, buffer.m_size);

        buffer_destroy(device, staging_buffer);
    }
}

static
renderable_mesh_t renderable_mesh_create(device_t& device, command_pool_t& graphics_command_pool, mesh_t* mesh)
{
    renderable_mesh_t renderable_mesh;
    renderable_mesh.m_mesh = mesh;
    renderable_mesh.m_vertex_buffer = buffer_create(device, BufferType::kVertexBuffer, mesh_calc_vertex_buffer_size(mesh));
    renderable_mesh.m_index_buffer = buffer_create(device, BufferType::kIndexBuffer, mesh_calc_index_buffer_size(mesh));
    buffer_update(device, renderable_mesh.m_vertex_buffer, graphics_command_pool, mesh->m_vertices.data());
    buffer_update(device, renderable_mesh.m_index_buffer, graphics_command_pool, mesh->m_indices.data());
    return renderable_mesh;
}

static
void renderable_mesh_destroy(device_t& device, renderable_mesh_t& rm)
{
    buffer_destroy(device, rm.m_index_buffer);
    buffer_destroy(device, rm.m_vertex_buffer);
}

static
texture_t texture_create_from_file(device_t& device, command_pool_t graphics_command_pool, const char* filepath)
{
    texture_t texture;
    return texture;
}

static
texture_t texture_create_color_target(device_t& device, VkFormat format, u32 width, u32 height)
{
    texture_t texture;
    texture.m_num_mips = 1;
    image_create(device, width, height, texture.m_num_mips, device.m_max_num_msaa_samples, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.m_vk_handle, texture.m_vk_memory);
    image_view_create(device, texture.m_vk_handle, format, VK_IMAGE_ASPECT_COLOR_BIT, texture.m_num_mips);
    return texture;
}

static
texture_t texture_create_depth_target(device_t& device, VkFormat format, u32 width, u32 height)
{
    texture_t texture;
    texture.m_num_mips = 1;
    image_create(device, width, height, texture.m_num_mips, device.m_max_num_msaa_samples, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.m_vk_handle, texture.m_vk_memory);
    image_view_create(device, texture.m_vk_handle, format, VK_IMAGE_ASPECT_DEPTH_BIT, texture.m_num_mips);
    return texture;
}

static
void texture_destroy(device_t& device, texture_t& texture)
{
    vkDestroyImageView(device.m_device_vk_handle, texture.m_image_view, nullptr);
    vkDestroyImage(device.m_device_vk_handle, texture.m_vk_handle, nullptr);
    vkFreeMemory(device.m_device_vk_handle, texture.m_vk_memory, nullptr); 
}

static
sampler_t sampler_create(device_t& device, u32 max_mip_level)
{
    sampler_t sampler;
    sampler.m_max_mip_level = max_mip_level;

    // Probably should have a wrapper class around the physical device and store its properties so we can easily retrieve when needed without having to make a vulkan call everytime
    VkPhysicalDeviceProperties properties = device.m_phys_device_properties;

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

    VULKAN_ASSERT(vkCreateSampler(device.m_device_vk_handle, &create_info, nullptr, &sampler.m_vk_handle));
    return sampler;
}

static
void sampler_destroy(device_t& device, sampler_t& sampler)
{
    vkDestroySampler(device.m_device_vk_handle, sampler.m_vk_handle, nullptr);
}

static
void command_pool_destroy(device_t& device, command_pool_t& command_pool)
{
    vkDestroyCommandPool(device.m_device_vk_handle, command_pool.m_vk_handle, nullptr);
}

static
void swapchain_destroy(device_t& device, swapchain_t& swapchain)
{
    vkDestroySwapchainKHR(device.m_device_vk_handle, swapchain.m_vk_handle, nullptr);
}

static
void device_destroy(device_t& device)
{
    vkDestroyDevice(device.m_device_vk_handle, nullptr);
}

static
void surface_destroy(instance_t& instance, surface_t& surface)
{
    vkDestroySurfaceKHR(instance.m_vk_handle, surface.m_vk_handle, nullptr);
}

static
void instance_destroy(instance_t& instance)
{
    vkDestroyInstance(instance.m_vk_handle, nullptr);
}

//void VulkanRenderer::UpdateUniformBuffer(U32 currentImage)
//{
//	MVPUniformBuffer ubo;
//
//	static F32 dt = 0.0f;
//	dt += 0.016f;
//
//	const F32 speed = 0.0f;
//
//	Mat44 rot = Mat44::IDENTITY;
//	rot.RotateZDeg(dt * speed);
//	ubo.m_model = rot;
//
//	ubo.m_view = m_pCurrentCamera->GetViewTransform();
//
//	ubo.m_projection = CreatePerspectiveProjection(45.0f, 0.01f, 100.0f, m_pSwapchain->GetAspectRatio());
//
//	m_pUniformBuffers[currentImage]->Update(m_pGraphicsCommandPool, &ubo);
//}

struct subpass_t
{
	std::vector<VkAttachmentReference> m_color_attach_refs;
	std::vector<VkAttachmentReference> m_resolve_attach_refs;
	VkAttachmentReference m_depth_attach_ref;
	bool m_has_depth_attach;
};

struct render_pass_t
{
	VkRenderPass m_vk_handle;
	std::vector<VkSubpassDescription> m_subpasses;
	std::vector<VkSubpassDependency> m_subpass_dependencies;
	std::vector<VkAttachmentDescription> m_attachments;
    
};

static render_pass_t s_render_pass;

void renderer_init(window_t* app_window)
{
    ASSERT(nullptr != app_window);
    s_window = app_window;
    s_instance = instance_create(); 
    s_surface = surface_create(s_instance, *s_window);
    s_device = device_create(s_instance, s_surface);
    s_swapchain = swapchain_create(s_device, s_surface, *s_window);
    s_graphics_command_pool = command_pool_create(s_device, VK_QUEUE_GRAPHICS_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    // meshes
    s_viking_room_mesh = mesh_load_from_obj("models/viking_room.obj");
    s_viking_room_renderable_mesh = renderable_mesh_create(s_device, s_graphics_command_pool, s_viking_room_mesh);

    s_world_axes_mesh = mesh_load_axes();
    s_world_axes_renderable_mesh = renderable_mesh_create(s_device, s_graphics_command_pool, s_world_axes_mesh);

    // textures
    s_viking_room_texture = texture_create_from_file(s_device, s_graphics_command_pool, "models/viking_room.obj");
    s_color_target = texture_create_color_target(s_device, s_swapchain.m_format, s_swapchain.m_extent.width, s_swapchain.m_extent.height);
    s_depth_target = texture_create_depth_target(s_device, s_swapchain.m_format, s_swapchain.m_extent.width, s_swapchain.m_extent.height);

    // samplers
    s_sampler = sampler_create(s_device, s_viking_room_texture.m_num_mips);

    // uniform buffers
	s_uniform_buffers.resize(s_swapchain.m_num_images);
	for (size_t i = 0; i < s_swapchain.m_num_images; i++)
	{
        s_uniform_buffers[i] = buffer_create(s_device, BufferType::kUniformBuffer, sizeof(mvp_buffer_t));
	}

    // render pass
    {
        // attachments
        VkAttachmentDescription color_target_desc = setup_attachment_desc(s_swapchain.m_format, s_device.m_max_num_msaa_samples, 
                                                                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                                                                          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                                                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                                          0);

        VkAttachmentDescription depth_target_desc = setup_attachment_desc(s_swapchain.m_format, s_device.m_max_num_msaa_samples, 
                                                                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
                                                                          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                                          0);

        VkAttachmentDescription resolve_target_desc = setup_attachment_desc(s_swapchain.m_format, VK_SAMPLE_COUNT_1_BIT, 
                                                                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
                                                                            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                                            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                                            0);
        s_render_pass.m_attachments.push_back(color_target_desc);
        s_render_pass.m_attachments.push_back(depth_target_desc);
        s_render_pass.m_attachments.push_back(resolve_target_desc);

        // subpass
        subpass_t subpass;

            // attach refs
            VkAttachmentReference color_attach_ref = {};
            color_attach_ref.attachment = 0;
            color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpass.m_color_attach_refs.push_back(color_attach_ref);

            VkAttachmentReference depth_attach_ref = {};
            depth_attach_ref.attachment = 1;
            depth_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpass.m_depth_attach_ref = depth_attach_ref;
            subpass.m_has_depth_attach = true;

            VkAttachmentReference resolve_attach_ref = {};
            resolve_attach_ref.attachment = 1;
            resolve_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subpass.m_resolve_attach_refs.push_back(resolve_attach_ref);

            // subpass desc
            VkSubpassDescription subpass_desc = {};
            subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_desc.colorAttachmentCount = (u32)subpass.m_color_attach_refs.size();
            subpass_desc.pColorAttachments = subpass.m_color_attach_refs.data();
            subpass_desc.pResolveAttachments = subpass.m_resolve_attach_refs.data();
            subpass_desc.pDepthStencilAttachment = subpass.m_has_depth_attach ? &subpass.m_depth_attach_ref : VK_NULL_HANDLE;

            s_render_pass.m_subpasses.push_back(subpass_desc);

            // subpass dependency 
            VkSubpassDependency sub_pass_dependency = {};
            sub_pass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            sub_pass_dependency.dstSubpass = 0;
            sub_pass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            sub_pass_dependency.srcAccessMask = 0;
            sub_pass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            sub_pass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            s_render_pass.m_subpass_dependencies.push_back(sub_pass_dependency);

        // finally create render pass
        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = (u32)s_render_pass.m_attachments.size();
        create_info.pAttachments = s_render_pass.m_attachments.data();
        create_info.subpassCount = (u32)s_render_pass.m_subpasses.size();
        create_info.pSubpasses = s_render_pass.m_subpasses.data();
        create_info.dependencyCount = (u32)s_render_pass.m_subpass_dependencies.size();
        create_info.pDependencies = s_render_pass.m_subpass_dependencies.data();

        VULKAN_ASSERT(vkCreateRenderPass(s_device.m_device_vk_handle, &create_info, nullptr, &s_render_pass.m_vk_handle));
    }

    // framebuffers
    std::vector<VkFramebuffer> framebuffers;
    framebuffers.resize(s_swapchain.m_num_images);
    {
        for(i32 i = 0; i < s_swapchain.m_num_images; i++)
        {
            std::vector<VkImageView> attachments { s_color_target.m_image_view, 
                                                   s_depth_target.m_image_view, 
                                                   s_swapchain.m_image_views[i] };

            VkFramebufferCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.renderPass = s_render_pass.m_vk_handle;
            create_info.pAttachments = attachments.data();
            create_info.attachmentCount = (u32)attachments.size();
            create_info.width = s_swapchain.m_extent.width;
            create_info.height = s_swapchain.m_extent.height;
            create_info.layers = 1;

            VULKAN_ASSERT(vkCreateFramebuffer(s_device.m_device_vk_handle, &create_info, nullptr, &framebuffers[i]));
        }
    }
    
    // descriptor sets
    {
        //m_pDescriptorSetLayout = new VulkanDescriptorSetLayout(m_pDevice);
        //m_pDescriptorSetLayout->AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        //m_pDescriptorSetLayout->AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        //m_pDescriptorSetLayout->Setup();

        //m_pDescriptorPool = new VulkanDescriptorPool(m_pDevice);
        //m_pDescriptorPool->AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_pSwapchain->GetNumImages());
        //m_pDescriptorPool->AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_pSwapchain->GetNumImages());
        //m_pDescriptorPool->Setup(m_pSwapchain->GetNumImages());

        //m_vkDescriptorSets = m_pDescriptorPool->AllocateDescriptorSets(m_pDescriptorSetLayout, m_pSwapchain->GetNumImages());

        //VulkanDescriptorSetWriter writer(m_pDevice);
        //for (size_t i = 0; i < m_pSwapchain->GetNumImages(); ++i)
        //{
        //    writer.Reset();
        //    writer.AddUniformBufferWrite(m_pUniformBuffers[i], 0, 0);
        //    writer.AddImageSamplerWrite(m_pVikingRoomTexture, m_pLinearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0);
        //    writer.WriteDescriptorSet(m_vkDescriptorSets[i]);
        //}
        
    }

    // pipelines
    {
        //// Viking Room
        //VulkanPipelineFactory pipelineMaker(m_pDevice);
        //pipelineMaker.SetVsPs("Shaders/tri-vert.spv", "main", "Shaders/tri-frag.spv", "main");
        //pipelineMaker.SetVertexInput(m_pVikingRoomMesh, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        //pipelineMaker.SetViewportAndScissor(m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());
        //pipelineMaker.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        //pipelineMaker.SetMultisampling(m_pDevice->GetMaxMsaaSampleCount());
        //pipelineMaker.SetDepthStencil(true, true, VK_COMPARE_OP_LESS, false, 0.0f, 1.0f);
        //
        //VulkanPipelineColorBlendAttachments colorBlendAttachments;
        //colorBlendAttachments.Add(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false,
        //                          VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
        //                          VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);
        //pipelineMaker.SetBlend(colorBlendAttachments, false, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);

        //std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { m_pDescriptorSetLayout->GetHandle() };
        //pipelineMaker.SetPipelineLayout(descriptorSetLayouts);

        //m_pVikingRoomPipeline = pipelineMaker.CreatePipeline(m_pRenderPass, 0);

        //// World axes
        //pipelineMaker.Reset();
        //pipelineMaker.SetVsPs("Shaders/simple-color-vert.spv", "main", "Shaders/simple-color-frag.spv", "main");
        //pipelineMaker.SetVertexInput(m_pWorldAxesMesh, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        //pipelineMaker.SetViewportAndScissor(m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());
        //pipelineMaker.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        //pipelineMaker.SetMultisampling(m_pDevice->GetMaxMsaaSampleCount());
        //pipelineMaker.SetDepthStencil(true, true, VK_COMPARE_OP_LESS, false, 0.0f, 1.0f);
        //
        //colorBlendAttachments.Reset();
        //colorBlendAttachments.Add(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false,
        //                          VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
        //                          VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);
        //pipelineMaker.SetBlend(colorBlendAttachments, false, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);

        //descriptorSetLayouts = { m_pDescriptorSetLayout->GetHandle() };
        //pipelineMaker.SetPipelineLayout(descriptorSetLayouts);

        //m_pWorldAxesPipeline = pipelineMaker.CreatePipeline(m_pRenderPass, 0);
    }

    // command buffers
    {
        //m_vkCommandBuffers = m_pGraphicsCommandPool->AllocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_pSwapchain->GetNumImages());

        //for (size_t i = 0; i < m_vkCommandBuffers.size(); i++)
        //{
        //    VulkanCommands::BeginCommandBuffer(m_vkCommandBuffers[i]);
        //    
        //    VkOffset2D offset{ 0, 0 };
        //    std::vector<VkClearValue> clearColors = { {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0} };
        //    VulkanCommands::BeginRenderPass(m_vkCommandBuffers[i], m_pRenderPass, m_vkSwapChainFrameBuffers[i], offset, m_pSwapchain->GetExtent(), clearColors);
        //    {
        //        // Normal draw
        //        {
        //            std::vector<VkDescriptorSet> descriptorSets = { m_vkDescriptorSets[i] };
        //            VulkanCommands::DrawRenderableMesh(m_vkCommandBuffers[i], m_pVikingRoomMesh, m_pVikingRoomPipeline, descriptorSets);
        //        }
        //         
        //        // World Axes
        //        {
        //            std::vector<VkDescriptorSet> descriptorSets = { m_vkDescriptorSets[i] };
        //            VulkanCommands::DrawRenderableMesh(m_vkCommandBuffers[i], m_pWorldAxesMesh, m_pWorldAxesPipeline, descriptorSets);
        //        }
        //    }
        //    VulkanCommands::EndRenderPass(m_vkCommandBuffers[i]);

        //    VulkanCommands::EndCommandBuffer(m_vkCommandBuffers[i]);
        //}
    }

    // sync objects
    {
        //m_vkImageAvailableSemaphores.resize(MAX_NUM_FRAMES_IN_FLIGHT);
        //m_vkRenderFinishedSemaphores.resize(MAX_NUM_FRAMES_IN_FLIGHT);
        //m_vkFrameFinishedFences.resize(MAX_NUM_FRAMES_IN_FLIGHT);
        //m_vkSwapChainImagesInFlight.resize(static_cast<U32>(m_pSwapchain->GetNumImages()), VK_NULL_HANDLE);

        //VkSemaphoreCreateInfo semaphoreInfo{};
        //semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        //VkFenceCreateInfo fenceInfo{};
        //fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        //fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        //for (size_t iSyncObj = 0; iSyncObj < MAX_NUM_FRAMES_IN_FLIGHT; iSyncObj++)
        //{
        //    VULKAN_ASSERT(vkCreateSemaphore(m_pDevice->GetHandle(), &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[iSyncObj]));
        //    VULKAN_ASSERT(vkCreateSemaphore(m_pDevice->GetHandle(), &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[iSyncObj]));
        //    VULKAN_ASSERT(vkCreateFence(m_pDevice->GetHandle(), &fenceInfo, nullptr, &m_vkFrameFinishedFences[iSyncObj]));
        //}
    }
}

void renderer_set_main_camera(camera_t* camera)
{
    ASSERT(nullptr != camera); 
    s_camera = camera;
}

void renderer_render_frame()
{
	//// Wait for previous frame to finish, this blocks on CPU
	//vkWaitForFences(m_pDevice->GetHandle(), 1, &m_vkFrameFinishedFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	//// Acquire an image from the swap chain
	//U32 imageIndex;
	//VkResult res = vkAcquireNextImageKHR(m_pDevice->GetHandle(), m_pSwapchain->GetHandle(), UINT64_MAX, m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	//if (res == VK_ERROR_OUT_OF_DATE_KHR)
	//{
	//	TeardownSwapchain();
	//	return;
	//}

	//ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);

	//UpdateUniformBuffer(imageIndex);

	//if (m_vkSwapChainImagesInFlight[imageIndex] != VK_NULL_HANDLE)
	//{
	//	vkWaitForFences(m_pDevice->GetHandle(), 1, &m_vkSwapChainImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	//}

	//m_vkSwapChainImagesInFlight[imageIndex] = m_vkFrameFinishedFences[m_currentFrame];

	//// Submit command buffer
	//VkSubmitInfo submitInfo;
	//MemZero(submitInfo);
	//submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//VkSemaphore waitSemaphores[] = { m_vkImageAvailableSemaphores[m_currentFrame] };
	//VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	//submitInfo.waitSemaphoreCount = 1;
	//submitInfo.pWaitSemaphores = waitSemaphores;
	//submitInfo.pWaitDstStageMask = waitStages;
	//submitInfo.commandBufferCount = 1;
	//submitInfo.pCommandBuffers = &m_vkCommandBuffers[imageIndex];

	//VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphores[m_currentFrame] };
	//submitInfo.signalSemaphoreCount = 1;
	//submitInfo.pSignalSemaphores = signalSemaphores;

	//vkResetFences(m_pDevice->GetHandle(), 1, &m_vkFrameFinishedFences[m_currentFrame]);
	//VULKAN_ASSERT(vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, m_vkFrameFinishedFences[m_currentFrame]));

	//// Present to screen
	//VkPresentInfoKHR presentInfo;
	//MemZero(presentInfo);
	//presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	//presentInfo.waitSemaphoreCount = 1;
	//presentInfo.pWaitSemaphores = signalSemaphores;
	//VkSwapchainKHR swapChains[] = { m_pSwapchain->GetHandle() };
	//presentInfo.swapchainCount = 1;
	//presentInfo.pSwapchains = swapChains;
	//presentInfo.pImageIndices = &imageIndex;
	//presentInfo.pResults = nullptr;

	//res = vkQueuePresentKHR(m_pDevice->GetGraphicsQueue(), &presentInfo);

	//if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || m_pWindow->WasResized())
	//{
	//	ResetupSwapChain();
	//}
	//else
	//{
	//	VULKAN_ASSERT(res);
	//}

	//m_currentFrame = (m_currentFrame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
}

void renderer_deinit()
{
    sampler_destroy(s_device, s_sampler);
    texture_destroy(s_device, s_depth_target);
    texture_destroy(s_device, s_color_target);
    texture_destroy(s_device, s_viking_room_texture);
    renderable_mesh_destroy(s_device, s_world_axes_renderable_mesh);
    renderable_mesh_destroy(s_device, s_viking_room_renderable_mesh);
    delete s_world_axes_mesh;
    delete s_viking_room_mesh;

    command_pool_destroy(s_device, s_graphics_command_pool);
    swapchain_destroy(s_device, s_swapchain);
    device_destroy(s_device);
    surface_destroy(s_instance, s_surface);
    instance_destroy(s_instance);
}
