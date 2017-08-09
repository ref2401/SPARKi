#include "common.hlsl"
#include "common_pbr.hlsl"


cbuffer cb_compute_shader : register(b0) {
	float g_roughness : packoffset(c0.x);
	float g_side_size : packoffset(c0.y);
};

TextureCube<float4>			g_tex_skybox : register(t0);
SamplerState				g_sampler : register(s0);
RWTexture2DArray<float4>	g_tex_cubemap : register(u0);


[numthreads(8, 8, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	static const float skybox_side_size = 512;		// envmap_texture_builder::skybox_side_size
	static const float skybox_mipmap_count = 10;	// envmap_texture_builder::skybox_mipmap_count
	static const uint sample_count = 1024;

	const float lvl = (skybox_mipmap_count - 1) * g_roughness;
	const float3 dir_ws = float3(1, -1, 1) * cube_direction(dt_id, g_side_size, g_side_size);

	float3 filtered_rgb = (float3)0.0f;
	float total_weight = 0.0f;

	for (uint i = 0; i < sample_count; ++i) {
		const float2 xi = hammersley(i, sample_count);
		
		const float4 sample_ggx = importance_sample_ggx(xi, g_roughness);
		const float pdf = sample_ggx.w;
		
		const float3 h_ws = tangent_to_world(sample_ggx.xyz, dir_ws);
		const float3 l_ws = 2 * dot(dir_ws, h_ws) * h_ws - dir_ws;
		
		const float dot_nl = saturate(dot(dir_ws, l_ws));
		if (dot_nl > 0) {
			//const float lvl = cube_mipmap_level(g_roughness, pdf, skybox_side_size, skybox_mipmap_count, sample_count);
			filtered_rgb += g_tex_skybox.SampleLevel(g_sampler, dir_ws, lvl).rgb * dot_nl;
			total_weight += dot_nl;
		}
	}

	total_weight = max(0.001, total_weight);
	g_tex_cubemap[dt_id] = float4(filtered_rgb / total_weight, 1.0);
}
