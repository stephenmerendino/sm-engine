#pragma once

#include "engine/thirdparty/imgui/imgui.h"

void ui_render();

struct ui_log
{
    ImGuiTextBuffer text_buffer;
    ImGuiTextFilter text_filter;
    ImVector<int>   line_offsets;
    bool            auto_scroll = true;

    void clear()
    {
        text_buffer.clear();
        line_offsets.clear();
        line_offsets.push_back(0);
    }

    void log_msg(const char* fmt, ...)
    {
        int old_size = text_buffer.size();

        va_list args;
        va_start(args, fmt);
        text_buffer.appendfv(fmt, args);
        va_end(args);

        for (int new_size = text_buffer.size(); old_size < new_size; old_size++)
        {
            if (text_buffer[old_size] == '\n')
            {
                line_offsets.push_back(old_size + 1);
            }
        }
    }

    void draw()
    {
        const char* title = "Log";
        bool open;

        if (!ImGui::Begin(title, &open))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Options"))
        {
            ImGui::OpenPopup("Options");
        }

        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        text_filter.Draw("Filter", -100.0f);

        //ImGui::Separator();
    }
};

