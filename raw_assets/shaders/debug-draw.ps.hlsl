#pragma pack_matrix(row_major)

struct ps_input_t 
{
	[[vk::location(0)]] float2 uv : UV0;
	[[vk::location(1)]] float3 color : COLOR0;
	[[vk::location(2)]] float3 normal : NORMAL0;
};

struct ps_output_t 
{
	float4 color : SV_TARGET0;
};

struct debug_draw_push_constants_t
{
	float4 color;
};

[[vk::push_constant]] debug_draw_push_constants_t debug_draw_push_constants;

ps_output_t main(ps_input_t IN)
{
	ps_output_t OUT;
	OUT.color = debug_draw_push_constants.color;
	return OUT;
}
