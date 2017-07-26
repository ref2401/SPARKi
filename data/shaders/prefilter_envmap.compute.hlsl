#include "common.hlsl"
#include "common_cubemap.hlsl"


cbuffer cb_compute_shader : register(b0) {
	float g_roughness : packoffset(c0);
};

TextureCube<float4>			g_tex_skybox : register(t0);
SamplerState				g_sampler : register(s0);
RWTexture2DArray<float4>	g_tex_cubemap : register(u0);


//float2 hammersley(uint index, uint count)
//{
//	uint bits = index;
//	bits = (bits << 16u) | (bits >> 16u);
//	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
//	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
//	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
//	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
//	const float van_der_corpus = float(bits) * 2.3283064365386963e-10;
//
//	return float2(float(index) / float(count), van_der_corpus);
//}

float2 hammersley(uint i, uint count)
{
	uint bits = i;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	const float van_der_corpus = float(bits) * 2.3283064365386963e-10;
	return float2(float(i) / float(count), van_der_corpus);
}

float3 importance_sample_ggx(float2 xi, float3 n, float roughness)
{
	const float a = roughness * roughness;

	const float phi = 2.0 * pi * xi.x;
	const float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
	const float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

	// from spherical coordinates to cartesian coordinates
	const float3 h_ts = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
	// from tangent-space vector to world-space sample vector
	const float3 up = abs(n.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	const float3 tangent = normalize(cross(up, n));
	const float3 bitangent = cross(n, tangent);

	return normalize((tangent * h_ts.x) + (bitangent * h_ts.y) + (n * h_ts.z));
}

float3 importance_sample_ggx(float2 xi, float roughness)
{
	const float a = roughness * roughness;

	// spherical coords
	const float phi = 2.0 * pi * xi.x;
	const float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
	const float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

	// spherical coords to cartesian coords
	return float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

[numthreads(128, 8, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w = 0;
	uint h = 0;
	uint c = 0;
	g_tex_cubemap.GetDimensions(w, h, c);
	const float3 dir_ws = normalize(cube_direction(dt_id, (float)w, (float)h));

	const uint sample_count = 1024;
	float3 filtered_rgb = 0.0f;
	float total_weight = 0.0f;

	for (uint i = 0; i < sample_count; ++i) {
		const float2 xi = hammersley(i, sample_count);
		//const float3 h_ws = importance_sample_ggx(xi, dir_ws, g_roughness);
		const float3 h_ws = tangent_to_world(importance_sample_ggx(xi, g_roughness), dir_ws);
		const float3 l_ws = 2 * dot(dir_ws, h_ws) * h_ws - dir_ws;

		const float dot_nl = saturate(dot(dir_ws, l_ws));
		if (dot_nl > 0) {
			filtered_rgb += g_tex_skybox.SampleLevel(g_sampler, dir_ws, 0).rgb * dot_nl;
			total_weight += dot_nl;
		}
	}

	g_tex_cubemap[dt_id] = float4(filtered_rgb / total_weight, 1.0f);
	//g_tex_cubemap[dt_id] = float4(length(dir_ws), 0, 0, 1.0);
}
