
RWTexture2DArray<float> g_tex_cubemap;

[numthreads(512, 2, 1)]
void cs_main(uint3 gt_id : SV_GroupThreadId)
{
	g_tex_cubemap[gt_id] = (float3)gt_id;
}
