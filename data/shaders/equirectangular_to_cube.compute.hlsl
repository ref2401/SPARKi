
//Texture2D					g_tex_equirectangular_cubemap;
//SamplerState				g_sampler;
RWTexture2DArray<float4>	g_tex_cubemap;

static const float3 cube_vertices[8] = {
	float3(-1, -1, -1), float3(1, -1, -1), float3(1, 1, -1), float3(-1, 1, -1),
	float3(-1, -1, 1), float3(1, -1, 1), float3(1, 1, 1), float3(-1, 1, 1),
};

static const float3 masks[6] = {
	/* X+/- */ float3(0.5, 0, 1), float3(0.5, 1, 0),
	/* Y+/- */ float3(1, 0.5, 0), float3(0, 0.5, 1),
	/* Z+/- */ float3(1, 0, 0.5), float3(0, 1, 0.5)
};

float3 cube_direction(uint3 dt_id)
{
	const float u_factor = float(dt_id.x) / 1024.0f;
	const float v_factor = 1.0f - float(dt_id.y) / 1024.0f;

	// X+
	if (dt_id.z == 0) {
		const float3 u_mask = masks[0];
		const float3 v_mask = masks[1];

		return normalize(u_mask * lerp(cube_vertices[1], cube_vertices[5], u_factor)
			+ v_mask * lerp(cube_vertices[1], cube_vertices[2], v_factor));
	}
	// X-
	else if (dt_id.z == 1) {
		const float3 u_mask = masks[0];
		const float3 v_mask = masks[1];

		return normalize(u_mask * lerp(cube_vertices[4], cube_vertices[0], u_factor)
			+ v_mask * lerp(cube_vertices[4], cube_vertices[7], v_factor));
	}
	// Y+
	else if (dt_id.z == 2) {
		const float3 u_mask = masks[2];
		const float3 v_mask = masks[3];

		return normalize(u_mask * lerp(cube_vertices[3], cube_vertices[2], u_factor)
			+ v_mask * lerp(cube_vertices[3], cube_vertices[7], v_factor));
	}
	// Y-
	else if (dt_id.z == 3) {
		const float3 u_mask = masks[2];
		const float3 v_mask = masks[3];

		return normalize(u_mask * lerp(cube_vertices[0], cube_vertices[1], u_factor)
			+ v_mask * lerp(cube_vertices[0], cube_vertices[4], v_factor));
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

		return normalize(u_mask * lerp(cube_vertices[0], cube_vertices[1], u_factor)
			+ v_mask * lerp(cube_vertices[0], cube_vertices[3], v_factor));
	}

	return (float3)0;
}

[numthreads(512, 2, 1)]
void cs_main(uint3 dt_id : SV_DispatchThreadId)
{
	const float3 dir = cube_direction(dt_id);

	g_tex_cubemap[dt_id] = float4(dir, 1.0);
}
