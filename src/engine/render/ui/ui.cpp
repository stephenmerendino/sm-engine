#include "engine/render/ui/ui.h"
#include "engine/render/ui/ui_debug_log.h"

static bool s_show_demo_window = false;
static ui_debug_log log;

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

    log.draw();
}

void ui_log_msg(const char* msg)
{
    log.log_msg(msg); 
}

void ui_log_msg_fmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log.log_msg_fmt(fmt, args);
    va_end(args);
}
