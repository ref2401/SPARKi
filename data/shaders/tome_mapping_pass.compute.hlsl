
Texture2D<float4>	g_tex_color	: register(t0);
RWTexture2D<float4>	g_tex_ldr	: register(u0);

[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId) {
	g_tex_ldr[dt_id.xy] = g_tex_color.Load(uint3(dt_id.xy, 0));
}
