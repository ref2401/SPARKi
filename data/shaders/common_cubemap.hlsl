
// Generates direction which can be used to sample a cubemap.
// Params:
//		- dt_id: dispatch thread id.
//		- width: cubemap's face width in pixels.
//		- height: cubemap's face height in pixels.
float3 cube_direction(uint3 dt_id, float width, float height)
{
	static const float3 cube_vertices[8] = {
		float3(-1, -1, -1), float3(1, -1, -1), float3(1, 1, -1), float3(-1, 1, -1),
		float3(-1, -1, 1), float3(1, -1, 1), float3(1, 1, 1), float3(-1, 1, 1),
	};

	static const float3 masks[6] = {
		/* X+/- */ float3(0.5, 0, 1), float3(0.5, 1, 0),
		/* Y+/- */ float3(1, 0.5, 0), float3(0, 0.5, 1),
		/* Z+/- */ float3(1, 0, 0.5), float3(0, 1, 0.5)
	};


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