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

ps_output_t main(ps_input_t IN)
{
	ps_output_t OUT;
	float3 normalColor = IN.normal * 0.5f + 0.5f;
	OUT.color = float4(normalColor, 1.0f);
	return OUT;
}
