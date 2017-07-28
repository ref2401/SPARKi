RWTexture2D<float2> g_tex_brdf : register(u0);


[numthreads(1024, 1, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w = 0;
	uint h = 0;
	uint lc = 0;
	g_tex_brdf.GetDimensions(0, w, h, lc);



	g_tex_brdf[dt_id.xy] = (float2)dt_id.xy;
}
