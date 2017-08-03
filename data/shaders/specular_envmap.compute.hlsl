#include "common.hlsl"
#include "common_pbr.hlsl"


cbuffer cb_compute_shader : register(b0) {
	float g_roughness : packoffset(c0);
};

TextureCube<float4>			g_tex_skybox : register(t0);
SamplerState				g_sampler : register(s0);
RWTexture2DArray<float4>	g_tex_cubemap : register(u0);


[numthreads(8, 8, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	static const float w = 128;				// envmap_texture_builder::envmap_side_size;
	static const float h = w;
	static const float mipmap_count = 5;	// envmap_texture_builder::envmap_mipmap_count
	static const uint sample_count = 1024;

	const float3 dir_ws = float3(1, -1, 1) * cube_direction(dt_id, w, h);
	const float lvl = mipmap_count - 3 - 1.15 * log2(g_roughness) - 1;
	//const float lvl = cube_mipmap_level(g_roughness, mipmap_count);

	/*float3 filtered_rgb = 0.0f;
	float total_weight = 0.0f;

	for (uint i = 0; i < sample_count; ++i) {
		const float2 xi = hammersley(i, sample_count);
		const float3 h_ws = tangent_to_world(importance_sample_ggx(xi, g_roughness), dir_ws);
		const float3 l_ws = 2 * dot(dir_ws, h_ws) * h_ws - dir_ws;

		const float dot_nl = saturate(dot(dir_ws, l_ws));
		if (dot_nl > 0) {
			filtered_rgb += g_tex_skybox.SampleLevel(g_sampler, dir_ws, lvl).rgb * dot_nl;
			total_weight += dot_nl;
		}
	}*/

	//g_tex_cubemap[dt_id] = float4(filtered_rgb / total_weight, 1.0f);
	g_tex_cubemap[dt_id] = float4(lvl, lvl, lvl, 1.0f);
}
