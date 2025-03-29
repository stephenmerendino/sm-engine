#include "sm/render/ui.h"
#include "third_party/imgui/imgui.h"

using namespace sm;

static bool s_show_demo_window = false;

void sm::ui_init()
{
}

void sm::ui_render()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("ImGui Demo"))
        {
            s_show_demo_window = true;
        }
        ImGui::EndMainMenuBar();
    }

	if(s_show_demo_window)
	{
        ImGui::ShowDemoWindow(&s_show_demo_window);
	}
}
