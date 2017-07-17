
RWTexture2DArray<float4> g_tex_cubemap;

[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	g_tex_cubemap[dt_id] = float4(dt_id, 1);
}
