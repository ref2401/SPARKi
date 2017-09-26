#include "common.hlsl"
#include "common_pbr.hlsl"


// ----- vertex shader ------

cbuffer cb_vertex_shader : register(b0) {
	float4x4 g_pv_matrix					: packoffset(c0);
	float4x4 g_model_matrix					: packoffset(c4);
	float4x4 g_normal_matrix				: packoffset(c8);
	float3 g_view_position_ws				: packoffset(c12.x);
	float3 g_view_position_ms				: packoffset(c13.x);
};

struct vertex {
	float3 position_ms		: VERT_POSITION_MS;
	float3 normal_ms		: VERT_NORMAL_MS;
	float2 uv				: VERT_UV;
	float4 tangent_space_ms	: VERT_TANGENT_SPACE_MS;
};

struct vs_output {
	float4 position_cs			: SV_Position;
	float3 v_ts					: PIXEL_V_TS;
	float2 uv					: PIXEL_UV;
	float3x3 model_to_tangent	: PIXEL_MODEL_TO_TANGENT;
};

vs_output vs_main(vertex vertex)
{
	// tangent space matrix
	const float4 ts_ms				= vertex.tangent_space_ms * 2.0f - 1.0f;
	const float3 bitangent_ms		= ts_ms.w * cross(vertex.normal_ms, ts_ms.xyz);
	const float3x3 model_to_tangent = float3x3(ts_ms.xyz, bitangent_ms, vertex.normal_ms);
	const float3x3 world_to_tangent = mul(model_to_tangent, float3x3(
		g_model_matrix._m00_m10_m20,
		g_model_matrix._m01_m11_m21,
		g_model_matrix._m02_m12_m22));

	const float3 p_ws = mul(g_model_matrix, float4(vertex.position_ms, 1)).xyz;
	const float3 v_ws = (g_view_position_ws - p_ws);

	vs_output o;
	o.position_cs		= mul(g_pv_matrix, float4(p_ws, 1.0));
	o.v_ts				= mul(world_to_tangent, v_ws);
	o.uv				= vertex.uv;
	o.model_to_tangent	= model_to_tangent;
	return o;
}

// ----- pixel shader -----

TextureCube<float4> g_tex_diffuse_envmap	: register(t0);
TextureCube<float4>	g_tex_specular_envmap	: register(t1);
Texture2D<float2>	g_tex_specular_brdf		: register(t2);
Texture2D<float4>	g_tex_base_color		: register(t3);
Texture2D<float4>	g_tex_reflect_color		: register(t4);
Texture2D<float4>	g_tex_normal_map		: register(t5);
Texture2D<float2>	g_tex_properties		: register(t6);
SamplerState		g_sampler				: register(s0);


struct ps_output {
	float4 rt_color0 : SV_Target0;
};

float3 eval_ibl(float3 cube_dir_ms, float dot_nv, float linear_roughness, float3 f0, float3 diffuse_color)
{
	// diffuse envmap
	const float3 diffuse_envmap = g_tex_diffuse_envmap.SampleLevel(g_sampler, cube_dir_ms, 0).rgb;

	// specular envmap & brdf
	const float lvl = linear_roughness * linear_roughness * 4.0f; // 4.0 == (envmap_texture_builder::envmap_mipmap_count - 1)
	const float3 specular_envmap = g_tex_specular_envmap.SampleLevel(g_sampler, cube_dir_ms, lvl).rgb;
	const float2 brdf = g_tex_specular_brdf.SampleLevel(g_sampler, float2(dot_nv, linear_roughness), 0);

	return diffuse_color * diffuse_envmap + specular_envmap * (f0 * brdf.x + brdf.y);
}

ps_output ps_main(vs_output pixel)
{
	const float3 n_ts = normalize(g_tex_normal_map.Sample(g_sampler, pixel.uv).xyz * 2.0f - 1.0f); // sample normal from a normal map.
	const float3 v_ts	= normalize(pixel.v_ts);
	const float	dot_nv	= saturate(dot(n_ts, v_ts));

	// material properties ---
	const float3	base_color			= g_tex_base_color.Sample(g_sampler, pixel.uv).rgb;
	const float3	reflect_color		= 0.16 * pow(g_tex_reflect_color.Sample(g_sampler, pixel.uv).rgb, 2);
	const float2	props				= g_tex_properties.Sample(g_sampler, pixel.uv);
	const float		metallic_mask		= props.x;
	const float		linear_roughness	= props.y;

	// cube sample direction ---
	const float3 rv_ts = reflect(-v_ts, n_ts);
	const float3 cube_dir_ts = specular_dominant_dir(n_ts, rv_ts, linear_roughness);
	const float3 cube_dir_ms = mul(cube_dir_ts, pixel.model_to_tangent); // == mul(transpose(model_to_tangent), cube_dir_ts);

	// eval brdf ---
	const float3 f0				= lerp(reflect_color, base_color.xyz, metallic_mask);
	const float3 fresnel		= fresnel_schlick(f0, dot_nv);
	const float3 diffuse_color	= base_color * ((float3)1.0 - fresnel) * (1 - metallic_mask);

	float3 color = 0;
	color += eval_ibl(cube_dir_ms, dot_nv, linear_roughness, f0, diffuse_color);

	ps_output o;
	o.rt_color0 = float4(color, 1);
	return o;
}
