#pragma pack_matrix(row_major)

struct InfiniteGridData
{
	float4x4 m_viewProjection;
	float4x4 m_inverseViewProjection;
    float m_depthFadeDistance;
    float m_majorLineThickness;
    float m_minorLineThickness;
};

[[vk::binding(0, 0)]]
ConstantBuffer<InfiniteGridData> g_infiniteGridData : register(b0, space0);

static float4 g_ndcPositions[6] = 
{ 
	float4(-1.0f, -1.0f, 0.5f, 1.0f),
	float4(-1.0f, 1.0f, 0.5f, 1.0f),
	float4(1.0f, 1.0f, 0.5f, 1.0f),
	float4(-1.0f, -1.0f, 0.5f, 1.0f),
	float4(1.0f, 1.0f, 0.5f, 1.0f),
	float4(1.0f, -1.0f, 0.5f, 1.0f)
};

struct VSInput
{
	[[vk::location(0)]] uint m_vertexIndex : SV_VertexID;
};

struct VSOutput
{
    float4 m_pos : SV_POSITION;
	[[vk::location(0)]] float4 m_worldPosNear : COLOR0;
	[[vk::location(1)]] float4 m_worldPosFar : COLOR1;
};

VSOutput Main(VSInput IN)
{
    float4 ndcPos = g_ndcPositions[IN.m_vertexIndex];
	
	VSOutput OUT;
	OUT.m_worldPosNear = mul(float4(ndcPos.x, ndcPos.y, 0.0f, 1.0f), g_infiniteGridData.m_inverseViewProjection);
	OUT.m_worldPosNear /= OUT.m_worldPosNear.w;

	OUT.m_worldPosFar = mul(float4(ndcPos.x, ndcPos.y, 1.0f, 1.0f), g_infiniteGridData.m_inverseViewProjection);
	OUT.m_worldPosFar /= OUT.m_worldPosFar.w;

	OUT.m_pos = ndcPos;

	return OUT;
}

//#version 450
//#extension GL_ARB_separate_shader_objects : enable
//
//layout(set = 0, binding = 0, row_major) uniform InfiniteGridData
//{
//	mat4x4 m_viewProjection;
//	mat4x4 m_inverseViewProjection;
//} infiniteGridData;
//
//vec4 g_ndcPositions[6] = 
//{ 
//	vec4(-1.0f, -1.0f, 0.5f, 1.0f),
//	vec4(-1.0f, 1.0f, 0.5f, 1.0f),
//	vec4(1.0f, 1.0f, 0.5f, 1.0f),
//	vec4(-1.0f, -1.0f, 0.5f, 1.0f),
//	vec4(1.0f, 1.0f, 0.5f, 1.0f),
//	vec4(1.0f, -1.0f, 0.5f, 1.0f)
//};
//
//layout(location = 0) out vec4 worldPosNear;
//layout(location = 1) out vec4 worldPosFar;
//
//void main()
//{
//    vec4 ndcPos = g_ndcPositions[gl_VertexIndex];
//	
//	worldPosNear = vec4(ndcPos.x, ndcPos.y, 0.0f, 1.0f) * infiniteGridData.m_inverseViewProjection;
//	worldPosNear.xyz /= worldPosNear.w;
//
//	worldPosFar = vec4(ndcPos.x, ndcPos.y, 1.0f, 1.0f) * infiniteGridData.m_inverseViewProjection;
//	worldPosFar.xyz /= worldPosFar.w;
//
//	gl_Position = ndcPos;
//}
