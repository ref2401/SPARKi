#include "common.hlsl"

static const uint c_property_color_max_count = 32; // material_properties_composer::c_property_color_max_count

cbuffer cb : register(b0) {
	uint2	g_tex_property_mask_size							: packoffset(c0);
	uint	g_property_count									: packoffset(c0.z);
	uint4	g_property_colors[c_property_color_max_count / 4]	: packoffset(c1);
	float4	g_property_values[c_property_color_max_count / 2]	: packoffset(c9);
};

Texture2D<float4>	g_tex_property_mask			: register(t0);
SamplerState		g_sampler_point				: register(s0);
RWTexture2D<float2>	g_tex_properties			: register(u0);


uint get_property_color(uint index)
{
	static uint arr[c_property_color_max_count] = (uint[c_property_color_max_count])g_property_colors;
	return arr[index];
}

float2 get_property_value(uint index)
{
	static float2 arr[c_property_color_max_count] = (float2[c_property_color_max_count])g_property_values;
	return arr[index];
}

float2 search_properties(uint c)
{
	if (g_property_count == 0) return get_property_value(0);

	uint b = 0;
	uint e = g_property_count;

	while (true) {
		const uint m = b + (e - b) / 2;
		const uint ref_c = get_property_color(m);

		if (c < ref_c)		e = m;
		else if (c > ref_c) b = m + 1;
		else				return get_property_value(m);

		if (b >= e) return float2(0xdeadbeef, 0xdeadbeef);
	}
}

[numthreads(32, 32, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadID)
{
	if ((dt_id.x >= g_tex_property_mask_size.x) 
		|| (dt_id.y >= g_tex_property_mask_size.y)) return;

	const float2	uv = float2(float2(dt_id.xy) / g_tex_property_mask_size);
	const float4	mask = g_tex_property_mask.SampleLevel(g_sampler_point, uv, 0);
	const uint		mask_color = pack_unorm_into_8_8_8_8(mask);

	g_tex_properties[dt_id.xy] = search_properties(mask_color);
}
