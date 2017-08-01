// ----- vertex shader ------

cbuffer cb_vertex_shader : register(b0) {
	float4x4 g_pvm_matrix		: packoffset(c0);
	float4x4 g_model_matrix	: packoffset(c4);
	float4x4 g_normal_matrix	: packoffset(c8);
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
	float2 uv			: PIXEL_UV;
	float3 t			: PIXEL_T;
	float3 b			: PIXEL_B;
};

vs_output vs_main(vertex vertex)
{
	// tangent space matrix
	const float3 normal_ws = (mul(g_normal_matrix, float4(vertex.normal_ms, 1.0f))).xyz;
	const float3 tangent_ws = (mul(g_normal_matrix, float4(vertex.tangent_space_ms.xyz, 1.0f))).xyz;
	const float3 bitangent_ws = vertex.tangent_space_ms.w * cross(normal_ws, tangent_ws);
	const float3x3 world_to_tangent = float3x3(tangent_ws, bitangent_ws, normal_ws);

	vs_output o;
	o.position_cs = mul(g_pvm_matrix, float4(vertex.position_ms, 1));
	o.n_ms = vertex.normal_ms;
	o.uv = vertex.uv;
	o.t = tangent_ws;
	o.b = bitangent_ws;

	return o;
}

// ----- pixel shader -----

Texture2D<float2>	g_tex_brdf : register(t0);
SamplerState		g_sampler : register(s0);


struct ps_output {
	float4 rt_color0 : SV_Target0;
};

ps_output ps_main(vs_output pixel)
{
	//const float2 brdf = g_tex_brdf.SampleLevel(g_sampler, pixel.uv, 0.0);
	
	ps_output o;
	o.rt_color0 = float4(pixel.t, 1);
	return o;
}
