#include "common.hlsl"
#include "common_pbr.hlsl"


cbuffer cb_compute_shader : register(b0) {
	float g_linear_roughness : packoffset(c0.x);
	float g_side_size : packoffset(c0.y);
};

TextureCube<float4>			g_tex_skybox	: register(t0);
SamplerState				g_sampler		: register(s0);
RWTexture2DArray<float4>	g_tex_envmap	: register(u0);


[numthreads(8, 8, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	static const float skybox_side_size = 512;		// envmap_texture_builder::skybox_side_size
	static const float skybox_mipmap_count = 10;	// envmap_texture_builder::skybox_mipmap_count
	static const uint min_sample_count = 4;
	static const uint max_sample_count = 32;

	const uint sample_count = lerp(min_sample_count, max_sample_count, g_linear_roughness);
	const float lvl = (skybox_mipmap_count - 1) * g_linear_roughness;
	const float3 dir_ws = float3(1, -1, 1) * cube_direction(dt_id, g_side_size, g_side_size);
	const float3x3 tw_matrix = tangent_to_world_matrix(dir_ws);

	float3 filtered_rgb = 0.0f;
	float total_weight = 0.0f;

	for (uint i = 0; i < sample_count; ++i) {
		const float2 xi = hammersley(i, sample_count);
		const float3 h_ws = mul(tw_matrix, importance_sample_ggx(xi, g_linear_roughness).xyz);
		const float3 l_ws = 2 * dot(dir_ws, h_ws) * h_ws - dir_ws;
		
		const float dot_nl = saturate(dot(dir_ws, l_ws));
		if (dot_nl > 0) {
			filtered_rgb += g_tex_skybox.SampleLevel(g_sampler, l_ws, lvl).rgb * dot_nl;
			total_weight += dot_nl;
		}
	}

	total_weight = max(0.001, total_weight);
	g_tex_envmap[dt_id] = float4(filtered_rgb / total_weight, 1.0);
}

