#include "engine/render/ui/ui.h"

static bool s_show_demo_window = false;

static
void draw_main_menu_bar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("Show UI Ref"))
        {
            s_show_demo_window = true;
        }
        ImGui::EndMainMenuBar();
    }
}

void ui_render()
{
    draw_main_menu_bar(); 

    if(s_show_demo_window)
    {
        ImGui::ShowDemoWindow(&s_show_demo_window);
    }
}
