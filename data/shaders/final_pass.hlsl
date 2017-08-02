// ----- vertex shader -----

struct vertex {
	uint id : SV_VertexID;
};

struct vs_output {
	float4 position : SV_Position;
	float2 uv		: PIXEL_UV;
};

vs_output vs_main(vertex vertex)
{
	static const float2 square_positions[4] = {
		float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0, 1.0), float2(1.0, 1.0) };

	const float2 p = square_positions[vertex.id];
	vs_output o;
	o.position = float4(p, 0, 1);
	o.uv = p * float2(0.5, -0.5) + 0.5f;

	return o;
}

// ----- pixel shader -----

static const float power = 1.0f / 2.2f;

Texture2D<float4>	g_tex_color	: register(t0);
SamplerState		g_sampler	: register(s0);


struct ps_output {
	float4 rt_color0 : SV_Target0;
};

ps_output ps_main(vs_output pixel)
{
	// hell of a tone mapping is performed
	const float4 hdra = g_tex_color.SampleLevel(g_sampler, pixel.uv, 0.0);
	const float3 rgb = pow(hdra.rgb / (hdra.rgb + 1.0f), power);

	ps_output o;
	o.rt_color0 = float4(rgb, 1.0f);;
	return o;
}
