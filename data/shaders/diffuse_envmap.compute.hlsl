#include "common.hlsl"
#include "common_pbr.hlsl"


TextureCube<float4>			g_tex_skybox	: register(t0);
SamplerState				g_sampler_state	: register(s0);
RWTexture2DArray<float4>	g_tex_envmap	: register(u0);


[numthreads(32, 32, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	static const float side_size = 32; // envmap_texture_builder::diffuse_envmap_size_size;
	static const float mip_level = 4; // mip level #4 corresponds to the 32x32 skybox size.
	static const float sample_step = 0.025;

	const float3 dir_ws = float3(1, -1, 1) * cube_direction(dt_id, side_size, side_size);
	const float3x3 tw_matrix = tangent_to_world_matrix(dir_ws);

	float3 irradiance = 0.0f;
	float total_weight = 0.0f;

	for (float phi = 0.0f; phi < c_2pi; phi += sample_step) {
		const float cos_phi = cos(phi);
		const float sin_phi = sin(phi);

		for (float theta = 0.0f; theta < c_pi_2; theta += sample_step) {
			const float cos_theta = cos(theta);
			const float sin_theta = sin(theta);
			const float3 l_ts = float3(cos_phi * sin_theta, sin_phi * sin_theta, cos_theta);

			const float3 l_ws = mul(tw_matrix, l_ts);
			irradiance += g_tex_skybox.SampleLevel(g_sampler_state, l_ws, mip_level) * cos_theta * sin_theta;
			++total_weight;
		}
	}

	g_tex_envmap[dt_id] = float4(c_pi * irradiance / total_weight, total_weight);
}