#include "app.h"

#include "sm/config.h"
#include "sm/core/array.h"
#include "sm/core/string.h"
#include "sm/core/assert.h"
#include "sm/core/debug.h"
#include "sm/core/random.h"
#include "sm/core/time.h"
#include "sm/core/types.h"
#include "sm/io/input.h"
#include "sm/memory/arena.h"
#include "sm/render/window.h"
#include "sm/render/vk_renderer.h"
#include "sm/thread/thread.h"

static bool s_is_running = true;
static sm::window_t* s_app_window = nullptr;

static void report_fps(sm::f32 ds)
{
	static sm::i32 s_frame_count = 0;
	static sm::f32 s_frame_time_accrual = 0.0f;

	s_frame_count++;
	s_frame_time_accrual += ds;

	if (s_frame_time_accrual >= FPS_CALC_TIME_INTERVAL_SECONDS)
	{
		sm::f32 avg_current_fps = (sm::f32)s_frame_count / s_frame_time_accrual;

		char new_title[256];
		::sprintf_s(new_title, "sm workbench - fps: %f - frame time: %.2f ms\n", avg_current_fps, ds * 1000.0f);

		sm::window_set_title(s_app_window, new_title);

		s_frame_count = 0;
		s_frame_time_accrual = 0.0f;
	}
}

static void sleep_remaining_frame(sm::f32 ds)
{
	const sm::f32 max_seconds_per_frame = (1.0f / MAX_FPS);

	// frame took longer than our max time
	if (ds >= max_seconds_per_frame)
	{
		return;
	}

	sm::f32 time_to_sleep_seconds = max_seconds_per_frame - ds;
	sm::thread_sleep_seconds(time_to_sleep_seconds);
}

static void app_window_msg_handler(sm::window_msg_type_t msg_type, sm::u64 msg_data, void* user_args)
{
	if(msg_type == sm::window_msg_type_t::CLOSE_WINDOW)
	{
		s_is_running = false;
	}
}

int app_run()
{
    sm::arena_t* app_startup_arena = sm::arena_init(KiB(1));

    s_app_window = sm::window_init(app_startup_arena, "sm workbench", 1920, 1080, true);
	sm::window_add_msg_cb(s_app_window, app_window_msg_handler, nullptr);

    sm::time_init();
	sm::random_init();
	sm::input_init(s_app_window);
	sm::renderer_init(s_app_window);

	sm::f32 ds = 0.016f; // default to delta seconds of 60 fps frame time

    sm::stopwatch_t frame_stopwatch;
    sm::stopwatch_start(&frame_stopwatch);

	while (s_is_running)
	{
		// begin frame
		sm::input_begin_frame();
		sm::renderer_begin_frame();
		//game_begin_frame();

		// update
        sm::window_update(s_app_window);
		sm::input_update(ds);
		sm::renderer_update_frame(ds);
		//game_update();

		// render
		//game_render();
		sm::renderer_render_frame();

		// end Frame
		sm::f32 work_time_seconds = sm::stopwatch_get_elapsed_seconds(&frame_stopwatch);
		sleep_remaining_frame(work_time_seconds);

		// set up next frame
		ds = sm::stopwatch_get_elapsed_seconds(&frame_stopwatch);
		sm::stopwatch_start(&frame_stopwatch);

		report_fps(ds);

		if(sm::input_was_key_pressed(sm::key_code_t::KEY_ESCAPE))
		{
			s_is_running = false;
		}
	}

	return 0;
}