// ----- vertex shader ------

cbuffer cb_vertex_shader : register(b0) {
	float4x4 g_pvm_matrix		: packoffset(c0);
	float4x4 g_model_matrix		: packoffset(c4);
	float4x4 g_normal_matrix	: packoffset(c8);
	float3 g_view_position_ms	: packoffset(c12.x);
	float3 g_view_position_ws	: packoffset(c13.x);
};

struct vertex {
	float3 position_ms		: VERT_POSITION_MS;
	float3 normal_ms		: VERT_NORMAL_MS;
	float2 uv				: VERT_UV;
	float4 tangent_space_ms	: VERT_TANGENT_SPACE_MS;
};

struct vs_output {
	float4 position_cs	: SV_Position;
	float3 n_ms			: PIXEL_N_MS;
	float3 v_ms			: PIXEL_V_MS;
	float2 uv			: PIXEL_UV;
};

vs_output vs_main(vertex vertex)
{
	// tangent space matrix
	const float4 ts_ms				= vertex.tangent_space_ms * 2.0f - 1.0f;
	const float3 normal_ws			= (mul(g_normal_matrix, float4(vertex.normal_ms, 1.0f))).xyz;
	const float3 tangent_ws			= (mul(g_normal_matrix, float4(ts_ms.xyz, 1.0f))).xyz;
	const float3 bitangent_ws		= ts_ms.w * cross(normal_ws, tangent_ws);
	const float3x3 world_to_tangent = float3x3(tangent_ws, bitangent_ws, normal_ws);

	vs_output o;
	o.position_cs	= mul(g_pvm_matrix, float4(vertex.position_ms, 1));
	o.n_ms			= normal_ws;
	o.v_ms			= (g_view_position_ms - vertex.position_ms);
	o.uv = vertex.uv;

	return o;
}

// ----- pixel shader -----

TextureCube<float4>	g_tex_envmap	: register(t0);
Texture2D<float2>	g_tex_brdf		: register(t1);
SamplerState		g_sampler		: register(s0);


struct ps_output {
	float4 rt_color0 : SV_Target0;
};

float3 ibl_light(float3 n_ms, float3 v_ms, float3 f0, float roughness)
{
	// sample envmap
	const float3 r_ms = reflect(-v_ms, n_ms);
	const float lod_level = roughness * 5.0f; // roughness * (envmap_mipmap_level_count::envmap_mipmap_level_count - 1)
	const float3 envmap = g_tex_envmap.SampleLevel(g_sampler, r_ms, lod_level).rgb;
	// sample brdf lut
	const float dot_nv = saturate(dot(n_ms, v_ms));
	const float2 brdf = g_tex_brdf.Sample(g_sampler, float2(dot_nv, roughness));

	return envmap * (f0 * brdf.x + brdf.y);
}

ps_output ps_main(vs_output pixel)
{
	static const float3 g_material_f0 = 0.04f;
	static const float g_material_roughness = 1.0f;

	const float3 n_ms = normalize(pixel.n_ms);

	float3 color = 0;
	color += ibl_light(n_ms, normalize(pixel.v_ms), g_material_f0, g_material_roughness);

	ps_output o;
	o.rt_color0 = float4(color, 1);
	return o;
}
