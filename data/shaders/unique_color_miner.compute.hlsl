#include "common.hlsl"

Texture2D<float4>			g_tex_image		: register(t0);
RWBuffer<uint>				g_hash_buffer	: register(u0);
RWStructuredBuffer<uint>	g_color_buffer	: register(u1);


[numthreads(32, 32, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	static const uint c_color_buffer_count = 32; // unique_color_miner::color_buffer_count

	uint w, h;
	g_tex_image.GetDimensions(w, h);
	if (dt_id.x >= w || dt_id.y >= h) return;

	// Compute hash key for the given rgb.
	const float3	rgb		= g_tex_image.Load(uint3(dt_id.xy, 0)).rgb;
	const uint		key		= pack_unorm_into_8_8_8(rgb);

	// Check if the color has already been hashed.
	uint prev;
	InterlockedExchange(g_hash_buffer[key], 1, prev);
	if (prev) return; // the color has already been added.

	const uint index = g_color_buffer.IncrementCounter();
	if (index < c_color_buffer_count)
		g_color_buffer[index] = (key << 8) | 255;
}