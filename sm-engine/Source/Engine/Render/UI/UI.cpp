#include "Engine/Render/UI/UI.h"
#include "Engine/ThirdParty/imgui/imgui.h"

static bool s_showDemoWindow = false;
static bool s_showLog = false;

// Logging
static ImGuiTextBuffer s_persistentTextBuffer;
static ImGuiTextFilter s_persistentTextFilter;
static ImVector<int>   s_persistentLineOffsets;
static bool            s_persistentAutoScroll = true;
static ImGuiTextBuffer s_frameTextBuffer;
static ImGuiTextFilter s_frameTextFilter;
static ImVector<int>   s_frameLineOffsets;

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
    if (ImGui::BeginChild("FrameLog", ImVec2(0, 150), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = s_frameTextBuffer.begin();
        const char* buf_end = s_frameTextBuffer.end();
        ImGuiListClipper clipper;
        clipper.Begin(s_frameLineOffsets.Size);
        while (clipper.Step())
        {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                const char* line_start = buf + s_frameLineOffsets[line_no];
                const char* line_end = (line_no + 1 < s_frameLineOffsets.Size) ? (buf + s_frameLineOffsets[line_no + 1] - 1) : buf_end;
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
        ImGui::Checkbox("Auto-scroll", &s_persistentAutoScroll);
        ImGui::EndPopup();
    }

    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }

    ImGui::SameLine();
    bool doClear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool doCopy = ImGui::Button("Copy");
    ImGui::SameLine();
    s_persistentTextFilter.Draw("Filter", -100.0f);


    if (ImGui::BeginChild("PersistentLog", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (doClear)
            UI::ClearMsgLog(UI::kPersistent);
        if (doCopy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* bufBegin = s_persistentTextBuffer.begin();
        const char* bufEnd = s_persistentTextBuffer.end();
        if (s_persistentTextFilter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have random access to the result of our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < s_persistentLineOffsets.Size; line_no++)
            {
                const char* line_start = bufBegin + s_persistentLineOffsets[line_no];
                const char* line_end = (line_no + 1 < s_persistentLineOffsets.Size) ? (bufBegin + s_persistentLineOffsets[line_no + 1] - 1) : bufEnd;
                if (s_persistentTextFilter.PassFilter(line_start, line_end))
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
            clipper.Begin(s_persistentLineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* lineStart = bufBegin + s_persistentLineOffsets[line_no];
                    const char* lineEnd = (line_no + 1 < s_persistentLineOffsets.Size) ? (bufBegin + s_persistentLineOffsets[line_no + 1] - 1) : bufEnd;
                    ImGui::TextUnformatted(lineStart, lineEnd);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (s_persistentAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
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
    ImGuiTextBuffer& textBuffer = (msgType == UI::MsgType::kPersistent) ? s_persistentTextBuffer : s_frameTextBuffer;
    ImVector<int>& lineOffsets = (msgType == UI::MsgType::kFrame) ? s_persistentLineOffsets : s_frameLineOffsets;

	int oldSize = textBuffer.size();
	textBuffer.append(msg);
	for (int newSize = textBuffer.size(); oldSize < newSize; oldSize++)
	{
		if (textBuffer[oldSize] == '\n')
		{
			lineOffsets.push_back(oldSize + 1);
		}
	}
}

void UI::LogMsgFmt(UI::MsgType msgType, const char* fmt, ...)
{
    ImGuiTextBuffer& textBuffer = (msgType == UI::MsgType::kPersistent) ? s_persistentTextBuffer : s_frameTextBuffer;
    ImVector<int>& lineOffsets = (msgType == UI::MsgType::kFrame) ? s_persistentLineOffsets : s_frameLineOffsets;

	va_list args;
	va_start(args, fmt);
	int oldSize = textBuffer.size();
	textBuffer.appendfv(fmt, args);
	for (int newSize = textBuffer.size(); oldSize < newSize; oldSize++)
	{
		if (textBuffer[oldSize] == '\n')
		{
			lineOffsets.push_back(oldSize + 1);
		}
	}
	va_end(args);
}

void UI::ClearMsgLog(UI::MsgType msgType)
{
    if (msgType == UI::kPersistent)
    {
        s_persistentTextBuffer.clear();
        s_persistentLineOffsets.clear();
        s_persistentLineOffsets.push_back(0);
    }
    else if (msgType == UI::kFrame)
    {
        s_frameTextBuffer.clear();
        s_frameLineOffsets.clear();
        s_frameLineOffsets.push_back(0);
    }
}