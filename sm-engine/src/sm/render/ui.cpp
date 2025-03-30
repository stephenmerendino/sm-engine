#include "sm/render/ui.h"
#include "third_party/imgui/imgui.h"

using namespace sm;

static bool s_show_demo_window = false;
static bool s_reload_shaders = false;

void sm::ui_init()
{
}

void sm::ui_begin_frame()
{
    s_reload_shaders = false;
}

void sm::ui_render()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("ImGui Demo"))
        {
            s_show_demo_window = true;
        }
        if (ImGui::MenuItem("Reload Shaders"))
        {
            s_reload_shaders = true;
        }
        ImGui::EndMainMenuBar();
    }

	if(s_show_demo_window)
	{
        ImGui::ShowDemoWindow(&s_show_demo_window);
	}
}

bool sm::ui_was_reload_shaders_requested()
{
    return s_reload_shaders;
}
