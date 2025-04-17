#pragma pack_matrix(row_major)

[[vk::binding(0, 0)]]
SamplerState g_linear_sampler_2d : register(s0, space0);

[[vk::binding(0, 2)]]
Texture2D<float4> g_diffuse : register(t0, space2);

struct ps_input_t 
{
	[[vk::location(0)]] float2 uv : UV0;
	[[vk::location(1)]] float3 color : COLOR0;
};

struct ps_output_t 
{
	float4 color : SV_TARGET0;
};

ps_output_t main(ps_input_t IN)
{
    ps_output_t OUT;
    float4 diffuse_color = g_diffuse.Sample(g_linear_sampler_2d, IN.uv);
	OUT.color = diffuse_color * float4(IN.color, 1.0f);
	return OUT;
}
