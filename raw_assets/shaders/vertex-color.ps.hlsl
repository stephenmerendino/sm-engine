#pragma pack_matrix(row_major)

struct ps_input_t 
{
	[[vk::location(0)]] float3 color : COLOR0;
};

struct ps_output_t 
{
	float4 color : SV_TARGET0;
};

ps_output_t main(ps_input_t IN)
{
	ps_output_t OUT;
	OUT.color = float4(IN.color, 1.0f);
	return OUT;
}
