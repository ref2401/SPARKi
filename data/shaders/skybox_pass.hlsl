
// ----- vertex shader -----

static const float3 cube_vertices[8] = {
	float3(1.0,	1.0, 1.0), float3(-1.0, 1.0, 1.0), float3(1.0,	1.0, -1.0), float3(-1.0, 1.0, -1.0),
	float3(1.0,	-1.0, 1.0), float3(-1.0, -1.0, 1.0), float3(-1.0, -1.0, -1.0), float3(1.0,	-1.0, -1.0)
};

static const uint cube_indices[14] = { 3, 2, 6, 7, 4, 2, 0, 3, 1, 6, 5, 4, 1, 0 };


cbuffer cb_vertex_shader : register(b0) {
	float4x4 g_pvm_matrix : packoffset(c0);
};

struct vs_output {
	float4 position		: SV_Position;
	float3 position_ms	: PIXEL_POSITION_MS;
};

vs_output vs_main(uint vid : SV_VertexID)
{
	const float3 p_ms = cube_vertices[cube_indices[vid]];

	vs_output o;
	o.position = mul(g_pvm_matrix, float4(p_ms, 1.0f)).xyww;
	o.position_ms = p_ms;
	return o;
}

// ----- pixel shader -----

static const float power = 1.0f / 2.2f;

TextureCube		g_tex_cubemap	: register(t0);
SamplerState	g_sampler		: register(s0);

struct ps_output {
	float4 rt_color_0 : SV_Target0;
}; 

ps_output ps_main(vs_output pixel)
{
	const float3 d = normalize(pixel.position_ms);
	const float3 hdr = g_tex_cubemap.Sample(g_sampler, d).rgb;
	const float3 rgb = pow(hdr / (hdr + 1.0f), power);

	ps_output o;
	o.rt_color_0 = float4(rgb, 1.0f);
	return o;
}
