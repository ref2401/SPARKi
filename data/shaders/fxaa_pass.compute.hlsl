
Texture2D<float4>	g_tex_tone_mapping	: register(t0);
RWTexture2D<float4> g_tex_aa			: register(u0);


[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint width, height;
	g_tex_aa.GetDimensions(width, height);
	if (dt_id.x >= width || dt_id.y >= height) return;

	const float3 rgb = g_tex_tone_mapping.Load(uint3(dt_id.xy, 0)).rgb;
	g_tex_aa[dt_id.xy] = float4(rgb, 1.0);
}
