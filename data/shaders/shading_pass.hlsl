// ----- vertex shader ------

cbuffer cb_vertex_shader : register(b0) {
	float4x4 g_pv_matrix		: packoffset(c0);
	float4x4 g_model_matrix		: packoffset(c4);
	float4x4 g_normal_matrix	: packoffset(c8);
	float3 g_view_position_ws	: packoffset(c12.x);
};

struct vertex {
	float3 position_ms		: VERT_POSITION_MS;
	float3 normal_ms		: VERT_NORMAL_MS;
	float2 uv				: VERT_UV;
	float4 tangent_space_ms	: VERT_TANGENT_SPACE_MS;
};

struct vs_output {
	float4 position_cs	: SV_Position;
	float3 v_ts			: PIXEL_V_TS;
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

	const float3 p_ws = mul(g_model_matrix, float4(vertex.position_ms, 1)).xyz;
	const float3 v_ws = (g_view_position_ws - p_ws);

	vs_output o;
	o.position_cs	= mul(g_pv_matrix, float4(p_ws, 1.0));
	o.v_ts			= mul(world_to_tangent, v_ws).xyz;
	o.uv			= vertex.uv;

	return o;
}

// ----- pixel shader -----

TextureCube<float4>	g_tex_envmap	: register(t0);
Texture2D<float2>	g_tex_brdf		: register(t1);
SamplerState		g_sampler		: register(s0);


struct ps_output {
	float4 rt_color0 : SV_Target0;
};

float3 eval_ibl(float3 n_ts, float3 v_ts, float f0, float roughness)
{
	// sample envmap
	const float3 r_ts = reflect(-v_ts, n_ts);
	const float lvl = roughness * 4.0f; // roughness * (envmap_mipmap_level_count::envmap_mipmap_level_count - 1)
	const float3 envmap = g_tex_envmap.SampleLevel(g_sampler, r_ts, lvl).rgb;
	// sample brdf lut
	const float dot_nv = saturate(dot(n_ts, v_ts));
	const float2 brdf = g_tex_brdf.Sample(g_sampler, float2(dot_nv, roughness));

	return envmap * (f0 * brdf.x + brdf.y);
}

ps_output ps_main(vs_output pixel)
{
	//static const float g_material_f0 = 0.04f;
	static const float g_material_f0 = 0.5f;
	static const float g_material_roughness = 1.0f;

	const float3 n_ts = float3(0, 0, 1);
	const float3 v_ts = normalize(pixel.v_ts);

	float3 color = 0;
	color += eval_ibl(n_ts, v_ts, g_material_f0, g_material_roughness);

	ps_output o;
	o.rt_color0 = float4(color, 1);
	return o;
}
