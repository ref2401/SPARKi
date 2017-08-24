
Texture2D<float4>	g_tex_color	: register(t0);
RWTexture2D<float4>	g_tex_ldr	: register(u0);


// Performs tone mapping and computes luminanece.
float4 tone_mapping_rgbl(float3 hdr)
{
	static const float gamma = 1.0f / 2.2f;

	const float3 ldr = hdr / (hdr + 1.0f);						// hell of a tone mapping
	const float3 s_rgb = pow(ldr, gamma);						// from linear to gamma space
	const float luma = dot(s_rgb, float3(0.299, 0.587, 0.114)); // compute luma

	return float4(s_rgb, luma);
}

[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId) 
{
	uint width, height;
	g_tex_ldr.GetDimensions(width, height);
	if (dt_id.x >= width || dt_id.y >= height) return;

	const float3 hdr = g_tex_color.Load(uint3(dt_id.xy, 0)).rgb;
	g_tex_ldr[dt_id.xy] = tone_mapping_rgbl(hdr);
}
