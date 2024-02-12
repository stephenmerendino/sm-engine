#include "Engine/Render/UI/UI.h"
#include "Engine/ThirdParty/imgui/imgui.h"

static bool s_showDemoWindow = false;
static bool s_showLog = false;

// Logging
static ImGuiTextBuffer s_persistent_text_buffer;
static ImGuiTextFilter s_persistent_text_filter;
static ImVector<int>   s_persistent_line_offsets;
static bool            s_persistent_auto_scroll = true;
static ImGuiTextBuffer s_frame_text_buffer;
static ImGuiTextFilter s_frame_text_filter;
static ImVector<int>   s_frame_line_offsets;

static void DrawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("UI Demo"))
        {
            s_showDemoWindow = true;
        }
        if (ImGui::MenuItem("Log"))
        {
            s_showLog = true;
        }
        ImGui::EndMainMenuBar();
    }
}

static void DrawLog(bool* open)
{
    const char* title = "Log";
    if (!ImGui::Begin(title, open))
    {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Frame Log");
    if (ImGui::BeginChild("frame_log", ImVec2(0, 150), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = s_frame_text_buffer.begin();
        const char* buf_end = s_frame_text_buffer.end();
        ImGuiListClipper clipper;
        clipper.Begin(s_frame_line_offsets.Size);
        while (clipper.Step())
        {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                const char* line_start = buf + s_frame_line_offsets[line_no];
                const char* line_end = (line_no + 1 < s_frame_line_offsets.Size) ? (buf + s_frame_line_offsets[line_no + 1] - 1) : buf_end;
                ImGui::TextUnformatted(line_start, line_end);
            }
        }
        clipper.End();
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::SeparatorText("Persistent Log");
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &s_persistent_auto_scroll);
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
    s_persistent_text_filter.Draw("Filter", -100.0f);


    if (ImGui::BeginChild("persistent_log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (do_clear)
            UI::ClearMsgLog(UI::kPersistent);
        if (do_copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = s_persistent_text_buffer.begin();
        const char* buf_end = s_persistent_text_buffer.end();
        if (s_persistent_text_filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have random access to the result of our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < s_persistent_line_offsets.Size; line_no++)
            {
                const char* line_start = buf + s_persistent_line_offsets[line_no];
                const char* line_end = (line_no + 1 < s_persistent_line_offsets.Size) ? (buf + s_persistent_line_offsets[line_no + 1] - 1) : buf_end;
                if (s_persistent_text_filter.PassFilter(line_start, line_end))
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
            clipper.Begin(s_persistent_line_offsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + s_persistent_line_offsets[line_no];
                    const char* line_end = (line_no + 1 < s_persistent_line_offsets.Size) ? (buf + s_persistent_line_offsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (s_persistent_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

void UI::Init()
{
    ClearMsgLog(kPersistent);
    ClearMsgLog(kFrame);
}

void UI::BeginFrame()
{
    ClearMsgLog(kFrame);
}

void UI::Render()
{
    DrawMainMenuBar();
    
    if(s_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&s_showDemoWindow);
    }

    if (s_showLog)
    {
        DrawLog(&s_showLog);
    }
}

void UI::LogMsg(UI::MsgType msgType, const char* msg)
{
    if (msgType == UI::kPersistent)
    {
        int old_size = s_persistent_text_buffer.size();
        s_persistent_text_buffer.append(msg);
        for (int new_size = s_persistent_text_buffer.size(); old_size < new_size; old_size++)
        {
            if (s_persistent_text_buffer[old_size] == '\n')
            {
                s_persistent_line_offsets.push_back(old_size + 1);
            }
        }
    }
    else if (msgType == UI::kFrame)
    {
        int old_size = s_frame_text_buffer.size();
        s_frame_text_buffer.append(msg);
        for (int new_size = s_frame_text_buffer.size(); old_size < new_size; old_size++)
        {
            if (s_frame_text_buffer[old_size] == '\n')
            {
                s_frame_line_offsets.push_back(old_size + 1);
            }
        }
    }
}

void UI::LogMsgFmt(UI::MsgType msgType, const char* fmt, ...)
{
    if (msgType == UI::kPersistent)
    {
        va_list args;
        va_start(args, fmt);
        int old_size = s_persistent_text_buffer.size();
        s_persistent_text_buffer.appendfv(fmt, args);
        for (int new_size = s_persistent_text_buffer.size(); old_size < new_size; old_size++)
        {
            if (s_persistent_text_buffer[old_size] == '\n')
            {
                s_persistent_line_offsets.push_back(old_size + 1);
            }
        }
        va_end(args);
    }
    else if (msgType == UI::kFrame)
    {
        va_list args;
        va_start(args, fmt);
        int old_size = s_frame_text_buffer.size();
        s_frame_text_buffer.appendfv(fmt, args);
        for (int new_size = s_frame_text_buffer.size(); old_size < new_size; old_size++)
        {
            if (s_frame_text_buffer[old_size] == '\n')
            {
                s_frame_line_offsets.push_back(old_size + 1);
            }
        }
        va_end(args);
    }
}

void UI::ClearMsgLog(UI::MsgType msgType)
{

    if (msgType == UI::kPersistent)
    {
        s_persistent_text_buffer.clear();
        s_persistent_line_offsets.clear();
        s_persistent_line_offsets.push_back(0);
    }
    else if (msgType == UI::kFrame)
    {
        s_frame_text_buffer.clear();
        s_frame_line_offsets.clear();
        s_frame_line_offsets.push_back(0);
    }
}