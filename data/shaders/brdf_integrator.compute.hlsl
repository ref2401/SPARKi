#include "common.hlsl"
#include "common_pbr.hlsl"


RWTexture2D<float2> g_tex_brdf : register(u0);


float2 integrate_brdf(float dot_nv, float roughness)
{
	const float3 v_ts = float3(sqrt(1 - dot_nv * dot_nv), 0.0f, dot_nv);

	float a = 0.0f;
	float b = 0.0f;
	const uint sample_count = 512;

	for (uint i = 0; i < sample_count; ++i) {
		const float2 xi = hammersley(i, sample_count);
		const float3 h_ts = importance_sample_ggx(xi, roughness);
		const float3 l_ts = 2 * dot(v_ts, h_ts) * h_ts - v_ts;

		const float dot_nl = saturate(l_ts.z);

		if (dot_nl > 0.0f) {
			const float g = g_smith_correlated(dot_nl, dot_nv, roughness);

			const float dot_nh = saturate(h_ts.z);
			const float dot_vh = saturate(dot(v_ts, h_ts));
			const float g_vis = (g * dot_vh) / (dot_nh * dot_nv);

			const float fc = pow(1 - dot_vh, 5);
			a += (1 - fc) * g_vis;
			b += fc * g_vis;
		}
	}

	a /= float(sample_count);
	b /= float(sample_count);

	return float2(a, b);
}

[numthreads(256, 4, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	float w = 0;
	float h = 0;
	g_tex_brdf.GetDimensions(w, h);

	const float dot_nv = (dt_id.x + 1) / w;
	const float roughness = dt_id.y / h;

	g_tex_brdf[dt_id.xy] = integrate_brdf(dot_nv, roughness);
}
