#include "common.hlsl"
#include "common_pbr.hlsl"


TextureCube<float4>			g_tex_skybox	: register(t0);
SamplerState				g_sampler_state	: register(s0);
RWTexture2DArray<float4>	g_tex_envmap	: register(u0);


[numthreads(32, 32, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	static const float side_size = 32; // envmap_texture_builder::diffuse_envmap_size_size;

	const float3 dir_ws = float3(1, -1, 1) * cube_direction(dt_id, side_size, side_size);
	g_tex_envmap[dt_id] = float4(dir_ws, 1.0);
}