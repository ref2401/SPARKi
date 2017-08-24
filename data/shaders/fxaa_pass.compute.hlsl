static const float edge_threshold_min = 0.0833;
static const float edge_threshold_max = 0.125;
static const float exploration_iteration_count = 8;
static const float exploration_uv_offset_factors[exploration_iteration_count] = {
	1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 
};

Texture2D<float4>	g_tex_tone_mapping	: register(t0);
SamplerState		g_sampler			: register(s0);
RWTexture2D<float4> g_tex_aa			: register(u0);


[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w, h;
	g_tex_aa.GetDimensions(w, h);
	if (dt_id.x >= w || dt_id.y >= h) return;

	const float2 rcp_tex_size = 1.0 / float2(w, h);
	const float2 uv = float2(float2(dt_id.xy) * rcp_tex_size);
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

	// ----- choosing edge direction -----
	//
	const float luma_ne = g_tex_tone_mapping.Load(uint3(dt_id.xy, 0), uint2(1, -1)).rgb;
	const float luma_sw = g_tex_tone_mapping.Load(uint3(dt_id.xy, 0), uint2(-1, 1)).rgb;
	// edge horz
	const float luma_ns = luma_n + luma_s;
	const float luma_nwsw = luma_nw + luma_sw;
	const float luma_nese = luma_ne + luma_se;
	const float edge_horz = abs(luma_nwsw - 2.0 * luma_w) 
		+ 2.0 * abs(luma_ns - 2.0 * luma_c)
		+ abs(luma_nese - 2.0 * luma_e);
	// edge vert
	const float luma_we = luma_w + luma_e;
	const float luma_nwne = luma_nw + luma_ne;
	const float luma_swse = luma_sw + luma_se;
	const float edge_vert = abs(luma_nwne - 2.0 * luma_n)
		+ 2.0 * abs(luma_we - 2.0 * luma_c)
		+ abs(luma_swse - 2.0 * luma_s);
	const bool is_horizontal = (edge_horz >= edge_vert);

	// ----- find edge orientation -----
	// 
	const float luma_0 = (is_horizontal) ? (luma_n) : (luma_w);
	const float luma_1 = (is_horizontal) ? (luma_s) : (luma_e);
	const float gradient_0 = abs(luma_0 - luma_c);
	const float gradient_1 = abs(luma_1 - luma_c);

	const bool is_0_steepest = (gradient_0 >= gradient_1);
	const float step_size = (is_0_steepest) ? (-1.0f) : (0.0f)
		* (is_horizontal) ? (rcp_tex_size.y) : (rcp_tex_size.x);

	// edge is between center texel and the one with luma_0 or luma_1 depends on which is the steepest.
	// calc luma and uv of the edge.
	const float luma_edge = 0.5 * (luma_c + (is_0_steepest) ? (luma_0) : (luma_1));
	const float uv_edge = uv + (float2)(0.5 * step_size) 
		* (is_horizontal) ? float2(0, 1) : float2(1, 0);

	// ----- edge exploration (the first iteration) -----
	//
	const float gradient_scaled = 0.25 * max(gradient_0, gradient_1);
	const float uv_offset = (is_horizontal) ? float2(rcp_tex_size.x, 0) : float2(0, rcp_tex_size.y);
	float2 uv_end_0 = uv_edge - uv_offset;
	float2 uv_end_1 = uv_edge + uv_offset;
	float luma_end_0 = g_tex_tone_mapping.SampleLevel(g_sampler, uv_end_0, 0);
	float luma_end_1 = g_tex_tone_mapping.SampleLevel(g_sampler, uv_end_1, 0);
	float delta_end_0 = abs(luma_edge - luma_end_0);
	float delta_end_1 = abs(luma_edge - luma_end_1);
	bool reached_end_0 = (delta_end_0 >= gradient_scaled);
	bool reached_end_1 = (delta_end_1 >= gradient_scaled);
	bool reached_both_ends = (reached_end_0 & reached_end_1);
	
	// ----- edge exploration (iterations [0, exploration_iteration_count]) -----
	//
	if (!reached_both_ends) {
		for (uint i = 0; i < exploration_iteration_count; ++i) {
			const float uv_offset_factor = exploration_uv_offset_factors[i];
			if (!reached_end_0) uv_end_0 -= uv_offset * uv_offset_factor;
			if (!reached_end_1) uv_end_1 += uv_offset * uv_offset_factor;

			if (!reached_end_0) {
				float luma_end_0 = g_tex_tone_mapping.SampleLevel(g_sampler, uv_end_0, 0);
				float delta_end_0 = abs(luma_edge - luma_end_0);
				bool reached_end_0 = (delta_end_0 >= gradient_scaled);
			}

			if (!reached_end_1) {
				float luma_end_1 = g_tex_tone_mapping.SampleLevel(g_sampler, uv_end_1, 0);
				float delta_end_1 = abs(luma_edge - luma_end_1);
				bool reached_end_1 = (delta_end_1 >= gradient_scaled);
			}

			reached_both_ends = (reached_end_0 & reached_end_1);
			if (reached_both_ends) break;
		}
	}

	// ----- estimating offsets -----
	//
	const float dist_0 = (is_horizontal) ? (uv.x - uv_end_0.x) : (uv.y - uv_end_0.y);
	const float dist_1 = (is_horizontal) ? (uv_end_1.x - uv.x) : (uv_end_0.y - uv.y);
	const float min_dist = min(dist_0, dist_1);

	g_tex_aa[dt_id.xy] = float4(0, 1, 0, 1);
}
