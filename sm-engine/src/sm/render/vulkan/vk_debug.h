#pragma once

#include "sm/render/vulkan/vk_include.h"
#include "sm/render/vulkan/vk_context.h"
#include "sm/core/color.h"

namespace sm
{
    void queue_begin_debug_label(VkQueue queue, const char* label, const color_f32_t& color);
    void queue_end_debug_label(VkQueue queue);
    void queue_insert_debug_label(VkQueue queue, const char* label, const color_f32_t& color);

    void command_buffer_begin_debug_label(VkCommandBuffer command_buffer, const char* label, const color_f32_t& color);
    void command_buffer_end_debug_label(VkCommandBuffer command_buffer);
    void command_buffer_insert_debug_label(VkCommandBuffer command_buffer, const char* label, const color_f32_t& color);

    void set_debug_name(render_context_t& context, VkObjectType type, u64 handle, const char* debug_name);

    class auto_queue_debug_label_t
    {
    public:
        auto_queue_debug_label_t() = delete;
        auto_queue_debug_label_t(VkQueue _queue, const char* label, const color_f32_t& color)
            :queue(_queue)
        {
            queue_begin_debug_label(queue, label, color);
        }

        ~auto_queue_debug_label_t()
        {
            queue_end_debug_label(queue);
        }

        VkQueue queue;
    };

    class auto_command_buffer_debug_label_t
    {
    public:
        auto_command_buffer_debug_label_t() = delete;
        auto_command_buffer_debug_label_t(VkCommandBuffer _command_buffer, const char* label, const color_f32_t& color)
            :command_buffer(_command_buffer)
        {
            command_buffer_begin_debug_label(command_buffer, label, color);
        }

        ~auto_command_buffer_debug_label_t()
        {
            command_buffer_end_debug_label(command_buffer);
        }

        VkCommandBuffer command_buffer;
    };

    #define SCOPED_QUEUE_DEBUG_LABEL(queue, label, color) \
        auto_queue_debug_label_t CONCATENATE(auto_queue_debug_label, __LINE__)(queue, label, color);

    #define SCOPED_COMMAND_BUFFER_DEBUG_LABEL(command_buffer, label, color) \
        auto_command_buffer_debug_label_t CONCATENATE(auto_command_buffer_debug_label, __LINE__) (command_buffer, label, color)
};
