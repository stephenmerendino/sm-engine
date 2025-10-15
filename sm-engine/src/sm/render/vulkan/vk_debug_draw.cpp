#include "sm/render/vulkan/vk_debug_draw.h"
#include "sm/render/vulkan/vk_mesh_instances.h"
#include "sm/render/vulkan/vk_renderer.h"
#include "sm/render/vulkan/vk_resources.h"
#include "sm/render/mesh_data.h"
#include "sm/core/array.h"

using namespace sm;

struct debug_draw_sphere_t
{
	sphere_t s;
	color_f32_t color;
	u32 num_frames_to_draw;
};

arena_t* s_debug_draw_arena = nullptr;
array_t<debug_draw_sphere_t> s_debug_spheres;

static gpu_mesh_t* s_sphere_mesh = nullptr;
static VkDescriptorSetLayout s_debug_material_descriptor_set_layout;
static material_t s_debug_material;

static void collect_mesh_instances(arena_t* frame_allocator, mesh_instances_t* frame_mesh_instances)
{
	mesh_instances_t debug_mesh_instances;
	mesh_instances_init(frame_allocator, &debug_mesh_instances, 1024);

	for (int i = 0; i < s_debug_spheres.cur_size; i++)
	{
        const material_t* material = g_debug_draw_material;

		debug_draw_push_constants_t* debug_draw_push_constants = arena_alloc_struct(frame_allocator, debug_draw_push_constants_t);
		debug_draw_push_constants->color = to_vec3(color_f32_t::RED);

		push_constants_t push_constants;
		push_constants.data = debug_draw_push_constants;
		push_constants.size = sizeof(debug_draw_push_constants_t);

		transform_t t;
		t.model = init_scale(s_debug_spheres[i].s.radius) * init_translation(s_debug_spheres[i].s.center);

		u32 flags = (u32)mesh_instance_flags_t::IS_DEBUG;

		mesh_instances_add(&debug_mesh_instances, s_sphere_mesh, material, push_constants, t, flags);
	}

	mesh_instances_append(frame_mesh_instances, &debug_mesh_instances);
}

void sm::debug_draw_init(render_context_t& context)
{
	s_debug_draw_arena = arena_init(MiB(1));
	renderer_register_collect_mesh_instances_cb(collect_mesh_instances);
	s_debug_spheres = array_init<debug_draw_sphere_t>(s_debug_draw_arena, 1024);

	s_sphere_mesh = arena_alloc_struct(s_debug_draw_arena, gpu_mesh_t);
	const cpu_mesh_t* sphere_cpu_mesh = mesh_data_get_primitive(primitive_t::UV_SPHERE);
	gpu_mesh_init(context, *sphere_cpu_mesh, *s_sphere_mesh);
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

void sm::debug_draw_sphere(const sphere_t& sphere, const color_f32_t& color, u32 num_frames)
{
	array_push<debug_draw_sphere_t>(s_debug_spheres, { .s = sphere, .color = color, .num_frames_to_draw = num_frames });
}