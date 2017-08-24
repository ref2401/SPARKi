static const float edge_threshold_min = 0.0833;
static const float edge_threshold_max = 0.125;

Texture2D<float4>	g_tex_tone_mapping	: register(t0);
SamplerState		g_sampler			: register(s0);
RWTexture2D<float4> g_tex_aa			: register(u0);


[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w, h;
	g_tex_aa.GetDimensions(w, h);
	if (dt_id.x >= w || dt_id.y >= h) return;

	const float2 uv = float2(float2(dt_id.xy) / float2(w, h));
	const float3 rgb_c = g_tex_tone_mapping.Load(uint3(dt_id.xy, 0)).rgb;
	
	// ----- local contrast test -----
	//
	const float4 luma_4a = g_tex_tone_mapping.GatherAlpha(g_sampler, uv);
	const float4 luma_4b = g_tex_tone_mapping.GatherAlpha(g_sampler, uv, uint2(-1, -1));
	const float luma_n	= luma_4b.z;
	const float luma_nw = luma_4b.w;
	const float luma_w	= luma_4b.x;
	const float luma_s	= luma_4a.x;
	const float luma_se = luma_4a.y;
	const float luma_e	= luma_4a.z;
	const float luma_c	= luma_4a.w; // same as luma_c = luma_4b.y;
	const float max_luma = max(luma_n, max(luma_nw, max(luma_w, max(luma_s, max(luma_se, max(luma_e, luma_c))))));
	const float min_luma = min(luma_n, min(luma_nw, min(luma_w, min(luma_s, min(luma_se, min(luma_e, luma_c))))));
	const float local_contrast = max_luma - min_luma;
	const bool early_exit = local_contrast < max(edge_threshold_min, max_luma * edge_threshold_max);
	if (early_exit) {
		g_tex_aa[dt_id.xy] = float4(rgb_c, 1);
		return;
	}

	g_tex_aa[dt_id.xy] = float4(0, 1, 0, 1);
}
