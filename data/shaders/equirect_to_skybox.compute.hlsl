#include "common_cubemap.hlsl"

static const float2 inv_atan = float2 (0.1591, 0.3183);

Texture2D<float4>			g_tex_equirect : register(t0);
SamplerState				g_sampler : register(s0);
RWTexture2DArray<float4>	g_tex_cubemap : register(u0);


[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w = 0;
	uint h = 0;
	uint c = 0;
	g_tex_cubemap.GetDimensions(w, h, c);

	const float3 dir = cube_direction(dt_id, (float)w, (float)h);
	const float2 uv = float2(atan2(dir.z, dir.x), asin(dir.y)) * inv_atan + 0.5f;
	g_tex_cubemap[dt_id] = g_tex_equirect.SampleLevel(g_sampler, uv, 0);
}
