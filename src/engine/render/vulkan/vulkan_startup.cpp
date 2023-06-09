#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/core/macros.h"
#include "engine/render/vulkan/vulkan_commands.h"
#include "engine/render/window.h"
#include "engine/render/vulkan/vulkan_formats.h"
#include "engine/render/vulkan/vulkan_startup.h"
#include "engine/render/vulkan/vulkan_resources.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

#include <set>

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
        SM_ASSERT(check_validation_layer_support());
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
    SM_VULKAN_ASSERT(vkCreateInstance(&create_info, nullptr, &vulkan_instance.handle));

    load_vulkan_instance_funcs(vulkan_instance.handle);

    // debug messenger
    if (ENABLE_VALIDATION_LAYERS)
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = setup_debug_messenger_create_info(vulkan_debug_cb);
        SM_VULKAN_ASSERT(vkCreateDebugUtilsMessengerEXT(vulkan_instance.handle, &debug_messenger_create_info, nullptr, &vulkan_instance.debug_messenger_handle));
    }

    return vulkan_instance;
}

static
surface_t surface_create(instance_t& instance, window_t& window)
{
    VkWin32SurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.hwnd = window.handle;
    create_info.hinstance = GetModuleHandle(nullptr);

    surface_t surface;
    SM_VULKAN_ASSERT(vkCreateWin32SurfaceKHR(instance.handle, &create_info, nullptr, &surface.handle));
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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface.handle, &details.capabilities);

    // surface formats
    u32 num_surface_formats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface.handle, &num_surface_formats, nullptr);

    if (num_surface_formats != 0)
    {
        details.formats.resize(num_surface_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface.handle, &num_surface_formats, details.formats.data());
    }

    // present modes
    u32 num_present_modes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.handle, &num_present_modes, nullptr);

    if (num_present_modes != 0)
    {
        details.present_modes.resize(num_present_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.handle, &num_present_modes, details.present_modes.data());
    }

    return details;
}

static
bool queue_family_indices_has_required(queue_family_indices_t& indices)
{
    return indices.graphics_family != queue_family_indices_t::INVALID && 
        indices.present_family != queue_family_indices_t::INVALID;
}

static 
swapchain_details_t query_swapchain_support(device_t& device, surface_t& surface)
{
    return query_swapchain_support(device.phys_device_handle, surface);
}

static 
queue_family_indices_t queue_family_find_all(VkPhysicalDevice device, surface_t& surface)
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
            indices.graphics_family = i;
        }

        // query for presentation support on this queue
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface.handle, &present_support);

        if (present_support)
        {
            indices.present_family = i;
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
    if (swapchain_details.formats.empty() || swapchain_details.present_modes.empty())
    {
        return false;
    }

    queue_family_indices_t queue_indices = queue_family_find_all(device, surface);
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
        vkEnumeratePhysicalDevices(instance.handle, &device_count, nullptr);
        SM_ASSERT(device_count != 0);

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance.handle, &device_count, devices.data());

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
                queue_families = queue_family_find_all(selected_physical_device, surface);
                max_num_msaa_samples = get_max_msaa_sample_count(selected_physical_device_props);
                break;
            }
        }

        SM_ASSERT(VK_NULL_HANDLE != selected_physical_device);
    }

    // logical device
    VkDevice logical_device = VK_NULL_HANDLE;
    {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<i32> unique_queue_families = { queue_families.graphics_family, queue_families.present_family };

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

        SM_VULKAN_ASSERT(vkCreateDevice(selected_physical_device, &create_info, nullptr, &logical_device));

        load_vulkan_device_funcs(logical_device);
    }

    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    {
        vkGetDeviceQueue(logical_device, queue_families.graphics_family, 0, &graphics_queue);
        vkGetDeviceQueue(logical_device, queue_families.present_family, 0, &present_queue);
    }

    device_t device = {};
    device.phys_device_handle = selected_physical_device;
    device.device_handle = logical_device;
    device.graphics_queue = graphics_queue;
    device.present_queue = present_queue;
    device.max_num_msaa_samples = max_num_msaa_samples;
    device.queue_families = queue_families;
    device.phys_device_properties = selected_physical_device_props;
    device.depth_format = format_find_depth(device);
    return device;
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
    //VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
    //VK_PRESENT_MODE_MAILBOX_KHR = 1,
    //VK_PRESENT_MODE_FIFO_KHR = 2,
    //VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3,
    //VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR = 1000111000,
    //VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR = 1000111001,
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
swapchain_t swapchain_create(device_t& device, surface_t& surface, window_t& window)
{
    swapchain_details_t swapchain_details = query_swapchain_support(device, surface);

    VkSurfaceFormatKHR surface_format   = swapchain_choose_surface_format(swapchain_details.formats);
    VkPresentModeKHR present_mode       = swapchain_choose_present_mode(swapchain_details.present_modes);
    VkExtent2D image_extent             = swapchain_choose_extent(swapchain_details.capabilities, window.width, window.height);

    u32 image_count = swapchain_details.capabilities.minImageCount + 1; // one extra image to prevent waiting on driver
    if (swapchain_details.capabilities.maxImageCount > 0)
    {
        image_count = min(image_count, swapchain_details.capabilities.maxImageCount);
    }

    VkSurfaceFullScreenExclusiveInfoEXT full_screen_info = {};
    full_screen_info.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
    full_screen_info.pNext = nullptr;
    full_screen_info.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr; // use full_screen_info here
    create_info.surface = surface.handle;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.presentMode = present_mode;
    create_info.imageExtent = image_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    queue_family_indices_t indices = device.queue_families;
    u32 queue_family_indices[] = { (u32)indices.graphics_family, (u32)indices.present_family };

    if (indices.graphics_family != indices.present_family)
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

    create_info.preTransform = swapchain_details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    swapchain_t swapchain = {};
    SM_VULKAN_ASSERT(vkCreateSwapchainKHR(device.device_handle, &create_info, nullptr, &swapchain.handle));

    vkGetSwapchainImagesKHR(device.device_handle, swapchain.handle, &swapchain.num_images, nullptr);

    swapchain.images.resize(swapchain.num_images);
    vkGetSwapchainImagesKHR(device.device_handle, swapchain.handle, &swapchain.num_images, swapchain.images.data());

    swapchain.format = surface_format.format;
    swapchain.extent = image_extent;
    swapchain.image_in_flight_fences.resize(swapchain.num_images);

    return swapchain;
}

static
void swapchain_do_initial_transitions(context_t& context)
{
    // manually init swapchain images to the correct starting layout
    VkCommandBuffer setup_swapchain_images_commands = command_begin_single_time(context);
    for(i32 i = 0; i < context.swapchain.num_images; i++)
    {
        command_transition_image_layout(setup_swapchain_images_commands, context.swapchain.images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
    }
    command_end_single_time(context, setup_swapchain_images_commands);
}

static
command_pool_t command_pool_create(device_t& device, VkQueueFlags queue_families, VkCommandPoolCreateFlags create_flags)
{
    queue_family_indices_t queue_family_indices = device.queue_families;

    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = create_flags;

    if (queue_families & VK_QUEUE_GRAPHICS_BIT)
    {
        create_info.queueFamilyIndex = queue_family_indices.graphics_family;
    }

    command_pool_t command_pool;
    SM_VULKAN_ASSERT(vkCreateCommandPool(device.device_handle, &create_info, nullptr, &command_pool.handle));
    return command_pool;
}

static
void command_pool_destroy(device_t& device, command_pool_t& command_pool)
{
    vkDestroyCommandPool(device.device_handle, command_pool.handle, nullptr);
}

static
void swapchain_destroy(device_t& device, swapchain_t& swapchain)
{
    vkDestroySwapchainKHR(device.device_handle, swapchain.handle, nullptr);
}

static
void device_destroy(device_t& device)
{
    vkDestroyDevice(device.device_handle, nullptr);
}

static
void surface_destroy(instance_t& instance, surface_t& surface)
{
    vkDestroySurfaceKHR(instance.handle, surface.handle, nullptr);
}

static
void instance_destroy(instance_t& instance)
{
    if(instance.debug_messenger_handle != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(instance.handle, instance.debug_messenger_handle, nullptr);
    }
    vkDestroyInstance(instance.handle, nullptr);
}

context_t* context_create(window_t* window)
{
    SM_ASSERT(nullptr != window);
    context_t* context = new context_t; 
    context->window = window;
    context->instance = instance_create(); 
    context->surface = surface_create(context->instance, *context->window);
    context->device = device_create(context->instance, context->surface);
    context->swapchain = swapchain_create(context->device, context->surface, *context->window);
    context->graphics_command_pool = command_pool_create(context->device, VK_QUEUE_GRAPHICS_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    swapchain_do_initial_transitions(*context);
    return context;
}

void context_destroy(context_t* context)
{
    SM_ASSERT(nullptr != context);
    command_pool_destroy(context->device, context->graphics_command_pool);
    swapchain_destroy(context->device, context->swapchain);
    device_destroy(context->device);
    surface_destroy(context->instance, context->surface);
    instance_destroy(context->instance);
    delete context; 
}

void context_refresh_swapchain(context_t& context)
{
    if(context.window->was_closed)
    {
        return;
    }
        
    swapchain_destroy(context.device, context.swapchain);    
    context.swapchain = swapchain_create(context.device, context.surface, *context.window);
    swapchain_do_initial_transitions(context);
}
