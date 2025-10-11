#include "sm/render/vulkan/vk_debug_draw.h"
#include "sm/render/vulkan/vk_mesh_instances.h"
#include "sm/render/vulkan/vk_renderer.h"
#include "sm/core/array.h"

using namespace sm;

struct debug_draw_sphere_t
{
	sphere_t s;
	u32 num_frames_to_draw;
};

arena_t* s_debug_draw_arena = nullptr;
array_t<debug_draw_sphere_t> s_debug_spheres;

static void collect_mesh_instances(arena_t* frame_allocator, mesh_instances_t* frame_mesh_instances)
{
	for (int i = 0; i < s_debug_spheres.cur_size; i++)
	{
		//mesh_t* mesh = mesh_get_primitive(SPHERE);
        //material_t* material;
        //push_constants_t push_constants;
        //transform_t initial_transform;
		//u32 flags = 0;
		//mesh_instances_add(frame_mesh_instances, mesh, material, push_constants, initial_transform, flags);
	}
}

void sm::debug_draw_init()
{
	s_debug_draw_arena = arena_init(MiB(1));
	renderer_register_collect_mesh_instances_cb(collect_mesh_instances);
	s_debug_spheres = array_init<debug_draw_sphere_t>(s_debug_draw_arena, 1024);
}

void sm::debug_draw_update()
{
	// loop through current debug draws and decrement num frames to draw
	for (int i = 0; i < s_debug_spheres.cur_size; i++)
	{
		s_debug_spheres[i].num_frames_to_draw--;
		if (s_debug_spheres[i].num_frames_to_draw == 0)
		{
			array_back_swap_delete(s_debug_spheres, i);
			i--;
		}
	}
}

void sm::debug_draw_sphere(const sphere_t& sphere, u32 num_frames)
{
	array_push<debug_draw_sphere_t>(s_debug_spheres, { .s = sphere, .num_frames_to_draw = num_frames });
}