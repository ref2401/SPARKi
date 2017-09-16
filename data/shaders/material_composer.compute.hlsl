#include "common.hlsl"

static const uint c_property_mapping_max_count = 32; // material_composer::c_property_mapping_max_count

cbuffer cb : register(b0) {
	float2	g_base_color_output_size								: packoffset(c0.x);
	float2	g_reflect_color_output_size								: packoffset(c0.z);
	uint	g_property_mapping_colors[c_property_mapping_max_count]	: packoffset(c1);
	float2	g_property_mappings[c_property_mapping_max_count]		: packoffset(c9);
	uint	g_property_mapping_count								: packoffset(c24);
};

Texture2D<float4>	g_tex_base_color_input		: register(t0);
Texture2D<float4>	g_tex_reflect_color_input	: register(t1);
Texture2D<float4>	g_tex_property_mask			: register(t2);
SamplerState		g_sampler_linear			: register(s0);
SamplerState		g_sampler_point				: register(s1);
RWTexture2D<float4>	g_tex_base_color_output		: register(u0);
RWTexture2D<float4>	g_tex_reflect_color_output	: register(u1);


struct result {
	bool	valid;
	float4	rgba;
};

result make_invalid_result()
{
	result r;
	r.valid = false;
	r.rgba = float4(0, 0, 0, 1);
	return r;
}

result make_valid_result(float4 rgba)
{
	result r;
	r.valid = true;
	r.rgba = rgba;
	return r;
}

uint sample_property_mask_color_8_8_8_8(float2 uv)
{
	const float4 c = g_tex_property_mask.SampleLevel(g_sampler_point, uv, 0);
	return pack_unorm_into_8_8_8_8(c);
}

float2 search_property_values(uint c)
{
	if (g_property_mapping_count == 0) return g_property_mappings[0];

	uint offset = g_property_mapping_count >> 1;
	uint m = offset;
	while (true) {
		offset = offset >> 1;

		const uint ref_c = g_property_mapping_colors[m];
		if (c < ref_c)		m -= offset;
		else if (c > ref_c) m += offset;
		else				return g_property_mappings[m];

		if (offset == 0) return g_property_mappings[0];
	}
}


result process_base_color(float2 pc)
{
	if ((pc.x >= g_base_color_output_size.x) || (pc.y >= g_base_color_output_size.y)) 
		return make_invalid_result();

	const float2	uv				= pc / g_base_color_output_size;
	const float3	base_color		= g_tex_base_color_input.SampleLevel(g_sampler_linear, uv, 0).rgb;
	const uint		mask_color		= sample_property_mask_color_8_8_8_8(uv);
	const float2	metallic_mask	= search_property_values(mask_color).x;

	return make_valid_result(float4(1, 0, 0, 1));
}


[numthreads(32, 32, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadID)
{
	const float2 uv_base_color = 

	g_tex_base_color_output[dt_id.xy] = float4(1, 0, 0, 1);
	g_tex_reflect_color_output[dt_id.xy] = float4(0, 1, 0, 1);
}
