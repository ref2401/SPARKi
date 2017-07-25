static const float3 cube_vertices[8] = {
	float3(-1, -1, -1), float3(1, -1, -1), float3(1, 1, -1), float3(-1, 1, -1),
	float3(-1, -1, 1), float3(1, -1, 1), float3(1, 1, 1), float3(-1, 1, 1),
};

static const float3 masks[6] = {
	/* X+/- */ float3(0.5, 0, 1), float3(0.5, 1, 0),
	/* Y+/- */ float3(1, 0.5, 0), float3(0, 0.5, 1),
	/* Z+/- */ float3(1, 0, 0.5), float3(0, 1, 0.5)
};

static const float2 inv_atan = float2 (0.1591, 0.3183);


Texture2D<float4>			g_tex_equirect : register(t0);
SamplerState				g_sampler : register(s0);
RWTexture2DArray<float4>	g_tex_cubemap : register(u0);


float3 cube_direction(uint3 dt_id, float width, float height)
{
	const float u_factor = float(dt_id.x) / width;
	const float v_factor = float(dt_id.y) / height;

	// X+
	if (dt_id.z == 0) {
		const float3 u_mask = masks[0];
		const float3 v_mask = masks[1];

		return normalize(u_mask * lerp(cube_vertices[5], cube_vertices[1], u_factor)
			+ v_mask * lerp(cube_vertices[5], cube_vertices[6], v_factor));
	}
	// X-
	else if (dt_id.z == 1) {
		const float3 u_mask = masks[0];
		const float3 v_mask = masks[1];

		return normalize(u_mask * lerp(cube_vertices[0], cube_vertices[4], u_factor)
			+ v_mask * lerp(cube_vertices[0], cube_vertices[3], v_factor));
	}
	// Y+
	else if (dt_id.z == 2) {
		const float3 u_mask = masks[2];
		const float3 v_mask = masks[3];

		return normalize(u_mask * lerp(cube_vertices[0], cube_vertices[1], u_factor)
			+ v_mask * lerp(cube_vertices[0], cube_vertices[4], v_factor));
	}
	// Y-
	else if (dt_id.z == 3) {
		const float3 u_mask = masks[2];
		const float3 v_mask = masks[3];

		return normalize(u_mask * lerp(cube_vertices[7], cube_vertices[6], u_factor)
			+ v_mask * lerp(cube_vertices[7], cube_vertices[3], v_factor));
	}
	// Z+
	else if (dt_id.z == 4) {
		const float3 u_mask = masks[4];
		const float3 v_mask = masks[5];

		return normalize(u_mask * lerp(cube_vertices[4], cube_vertices[5], u_factor)
			+ v_mask * lerp(cube_vertices[4], cube_vertices[7], v_factor));
	}
	// Z-
	else if (dt_id.z == 5) {
		const float3 u_mask = masks[4];
		const float3 v_mask = masks[5];

		return normalize(u_mask * lerp(cube_vertices[1], cube_vertices[0], u_factor)
			+ v_mask * lerp(cube_vertices[1], cube_vertices[2], v_factor));
	}

	return (float3)0;
}

[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	uint w = 0;
	uint h = 0;
	uint c = 0;
	g_tex_cubemap.GetDimensions(w, h, c);

	const float3 dir = cube_direction(dt_id, (float)w, (float)h);
	const float2 uv = float2(atan2(dir.z, dir.x), asin(dir.y)) * inv_atan + 0.5f;
	g_tex_cubemap[dt_id] = g_tex_equirect.SampleLevel(g_sampler, uv, 0);
}
