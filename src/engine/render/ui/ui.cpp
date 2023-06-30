#include "engine/render/ui/ui.h"
#include "engine/render/ui/ui_debug_log.h"
#include "engine/core/debug.h"

static bool s_show_demo_window = false;
static bool s_show_debug_log = false;

static
void draw_main_menu_bar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::MenuItem("UI Demo"))
        {
            s_show_demo_window = true;
        }
        if (ImGui::MenuItem("Log"))
        {
            s_show_debug_log = true;
        }
        ImGui::EndMainMenuBar();
    }
}

void ui_begin_frame()
{
    ui_log_begin_frame(); 
}

void ui_render()
{
    draw_main_menu_bar(); 

    if(s_show_demo_window)
    {
        ImGui::ShowDemoWindow(&s_show_demo_window);
    }

    if(s_show_debug_log)
    {
        ui_log_draw(&s_show_debug_log);
    }
}
