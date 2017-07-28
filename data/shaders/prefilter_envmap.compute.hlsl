#include "common.hlsl"
#include "common_pbr.hlsl"


cbuffer cb_compute_shader : register(b0) {
	float g_roughness : packoffset(c0);
};

TextureCube<float4>			g_tex_skybox : register(t0);
SamplerState				g_sampler : register(s0);
RWTexture2DArray<float4>	g_tex_cubemap : register(u0);


[numthreads(16, 16, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w = 0;
	uint h = 0;
	uint c = 0;
	g_tex_cubemap.GetDimensions(w, h, c);
	const float3 dir_ws = float3(1, -1, 1) * cube_direction(dt_id, (float)w, (float)h);

	const uint sample_count = 1024;
	float3 filtered_rgb = 0.0f;
	float total_weight = 0.0f;

	for (uint i = 0; i < sample_count; ++i) {
		const float2 xi = hammersley(i, sample_count);
		const float3 h_ws = tangent_to_world(importance_sample_ggx(xi, g_roughness), dir_ws);
		const float3 l_ws = 2 * dot(dir_ws, h_ws) * h_ws - dir_ws;

		const float dot_nl = saturate(dot(dir_ws, l_ws));
		if (dot_nl > 0) {
			filtered_rgb += g_tex_skybox.SampleLevel(g_sampler, dir_ws, 0).rgb * dot_nl;
			total_weight += dot_nl;
		}
	}

	g_tex_cubemap[dt_id] = float4(filtered_rgb / total_weight, 1.0f);
}
