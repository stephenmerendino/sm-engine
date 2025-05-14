#include "sm/render/ui.h"
#include "third_party/imgui/imgui.h"

using namespace sm;

static bool s_show_demo_window = false;
static bool s_reload_shaders = false;
static build_scene_window_cb_t s_build_scene_window_cb = nullptr;

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

    // Main window
    {
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;

        // We specify a default position/size in case there's no data in the .ini file.
        // We only do it to make the demo applications a little more welcoming, but typically this isn't required.
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 0, main_viewport->WorkPos.y + 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(600, 1024), ImGuiCond_Always);

        // Main body of the Demo window starts here.
        if (!ImGui::Begin("Scene", nullptr, window_flags))
        {
            // Early out if the window is collapsed, as an optimization.
            ImGui::End();
            return;
        }

        if (s_build_scene_window_cb)
        {
            s_build_scene_window_cb();
        }

        ImGui::End();
    }
}

void sm::ui_set_build_scene_window_callback(build_scene_window_cb_t cb)
{
    s_build_scene_window_cb = cb;
}

bool sm::ui_was_reload_shaders_requested()
{
    return s_reload_shaders;
}
