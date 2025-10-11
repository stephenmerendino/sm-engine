#include "sm/render/vulkan/vk_debug.h"
#include "sm/core/debug.h"

using namespace sm;

void sm::queue_begin_debug_label(VkQueue queue, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkQueueBeginDebugUtilsLabelEXT(queue, &queue_label);
}

void sm::queue_end_debug_label(VkQueue queue)
{
    if(!is_running_in_debug())
    {
        return;
    }

    vkQueueEndDebugUtilsLabelEXT(queue);
}

void sm::queue_insert_debug_label(VkQueue queue, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkQueueInsertDebugUtilsLabelEXT(queue, &queue_label);
}

void sm::command_buffer_begin_debug_label(VkCommandBuffer command_buffer, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkCmdBeginDebugUtilsLabelEXT(command_buffer, &queue_label);
}

void sm::command_buffer_end_debug_label(VkCommandBuffer command_buffer)
{
    if(!is_running_in_debug())
    {
        return;
    }

    vkCmdEndDebugUtilsLabelEXT(command_buffer);
}

void sm::command_buffer_insert_debug_label(VkCommandBuffer command_buffer, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkCmdInsertDebugUtilsLabelEXT(command_buffer, &queue_label);
}

void sm::set_debug_name(render_context_t& context, VkObjectType type, u64 handle, const char* debug_name)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT debug_name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = type,
        .objectHandle = handle,
        .pObjectName = debug_name 
    };
    SM_VULKAN_ASSERT(vkSetDebugUtilsObjectNameEXT(context.device, &debug_name_info));
}