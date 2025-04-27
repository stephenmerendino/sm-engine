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

struct gizmo_push_constants_t
{
	float3 color;
};

[[vk::push_constant]] gizmo_push_constants_t gizmo_push_constants;

ps_output_t main(ps_input_t IN)
{
	ps_output_t OUT;
	//float3 light_dir = normalize(float3(-.707f, -.707f, -1.0f));
	//float3 color = (IN.color * 0.7f) + dot(IN.normal, normalize(float3(0.0f, 0.0f, 1.0f))) * IN.color;
	OUT.color = float4(gizmo_push_constants.color, 1.0f);
	return OUT;
}
