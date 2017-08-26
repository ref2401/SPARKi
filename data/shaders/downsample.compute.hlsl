
Texture2D<float4>	g_tex_src	: register(t0);
SamplerState		g_sampler	: register(s0);
RWTexture2D<float4> g_tex_dest	: register(u0);


[numthreads(32, 32, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w, h;
	g_tex_dest.GetDimensions(w, h);
	if (dt_id.x >= w || dt_id.y >= h) return;

	const float2 uv = float2(dt_id.x / float(w), dt_id.y / float(h));
	g_tex_dest[dt_id.xy] = g_tex_src.SampleLevel(g_sampler, uv, 0);
}
