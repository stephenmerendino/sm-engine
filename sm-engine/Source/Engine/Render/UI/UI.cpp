#include "Engine/Render/UI/UI.h"
#include "Engine/ThirdParty/imgui/imgui.h"

static bool s_showDemoWindow = false;

static void DrawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("UI Demo"))
        {
            s_showDemoWindow = true;
        }
        ImGui::EndMainMenuBar();
    }
}
void UI::Render()
{
    DrawMainMenuBar();
    if(s_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&s_showDemoWindow);
    }
}