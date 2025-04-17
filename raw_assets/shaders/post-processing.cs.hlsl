[[vk::binding(0, 0)]]
RWTexture2D<float4> g_main_draw_result : register(u0, space0);

[[vk::binding(1, 0)]]
RWTexture2D<float4> g_post_processing_result : register(u1, space0);

struct post_processing_params_t
{
	uint2 texture_size;
};
ConstantBuffer<post_processing_params_t> g_post_processing_params : register(b2, space0);

struct frame_render_data_t
{
	float elapsed_time_seconds;
	float delta_time_seconds;
};
ConstantBuffer<frame_render_data_t> g_frame_render_data : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint2 xy_pos = dispatch_thread_id.xy;
	float4 main_draw_color = g_main_draw_result[xy_pos];
	main_draw_color.xyz = pow(main_draw_color.xyz, 1.0f / 2.0f);
    g_post_processing_result[xy_pos] = main_draw_color;
}