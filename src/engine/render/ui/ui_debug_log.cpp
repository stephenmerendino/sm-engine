#include "engine/render/ui/ui_debug_log.h"

ui_debug_log::ui_debug_log()
{
    clear();
}

void ui_debug_log::clear()
{
    text_buffer.clear();
    line_offsets.clear();
    line_offsets.push_back(0);
}

void ui_debug_log::log_msg(const char* msg)
{
    int old_size = text_buffer.size();
    text_buffer.append(msg);
    for (int new_size = text_buffer.size(); old_size < new_size; old_size++)
    {
        if (text_buffer[old_size] == '\n')
        {
            line_offsets.push_back(old_size + 1);
        }
    }
}

void ui_debug_log::log_msg_fmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_msg_fmt(fmt, args);
    va_end(args);
}

void ui_debug_log::log_msg_fmt(const char* fmt, va_list args)
{
    int old_size = text_buffer.size();
    text_buffer.appendfv(fmt, args);
    for (int new_size = text_buffer.size(); old_size < new_size; old_size++)
    {
        if (text_buffer[old_size] == '\n')
        {
            line_offsets.push_back(old_size + 1);
        }
    }
}

void ui_debug_log::draw()
{
    static bool show_log = false; 

    const char* title = "Log";
    if (!ImGui::Begin(title, &show_log))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        ImGui::EndPopup();
    }

    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }

    ImGui::SameLine();
    bool do_clear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool do_copy = ImGui::Button("Copy");
    ImGui::SameLine();
    text_filter.Draw("Filter", -100.0f);

    ImGui::Separator();

    if (ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (do_clear)
            clear();
        if (do_copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = text_buffer.begin();
        const char* buf_end = text_buffer.end();
        if (text_filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have random access to the result of our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < line_offsets.Size; line_no++)
            {
                const char* line_start = buf + line_offsets[line_no];
                const char* line_end = (line_no + 1 < line_offsets.Size) ? (buf + line_offsets[line_no + 1] - 1) : buf_end;
                if (text_filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   ImGui::TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
            // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
            // within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
            // on your side is recommended. Using ImGuiListClipper requires
            // - A) random access into your data
            // - B) items all being the  same height,
            // both of which we can handle since we have an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display
            // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
            // it possible (and would be recommended if you want to search through tens of thousands of entries).
            ImGuiListClipper clipper;
            clipper.Begin(line_offsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + line_offsets[line_no];
                    const char* line_end = (line_no + 1 < line_offsets.Size) ? (buf + line_offsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::End();
}
