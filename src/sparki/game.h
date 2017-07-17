#pragma once

#include "sparki/rnd/render.h"


namespace sparki {

// Position and orientation of a viewer (camera).
struct viewpoint final {

	viewpoint() noexcept = default;

	viewpoint(const float3& p, const float3& t, const float3 up = float3::unit_y)
		: position(p), target(t), up(up)
	{}

	float3 position = -float3::unit_z;
	float3 target = float3::zero;
	float3 up = float3::unit_y;
};

class game final {
public:

	game(HWND p_hwnd, const uint2& viewport_size);

	game(game&&) = delete;
	game& operator=(game&&) = delete;


	void draw_frame(float interpolation_factor);

	void resize_viewport(const uint2& size);

	void update();

private:

	static constexpr float projection_fov = pi_3;
	static constexpr float projection_near = 0.1f;
	static constexpr float projection_far = 1000.0f;

	frame		frame_;
	renderer	renderer_;
	viewpoint	viewpoint_curr_;
	viewpoint	viewpoint_prev_;
	bool		viewport_is_visible_;
};


inline viewpoint lerp(const viewpoint& l, const viewpoint& r, float factor) noexcept
{
	return viewpoint(
		lerp(l.position, r.position, factor),
		lerp(l.target, r.target, factor),
		lerp(l.up, r.up, factor)
	);
}

inline float4x4 view_matrix(const viewpoint& vp) noexcept
{
	return view_matrix(vp.position, vp.target, vp.up);
}

} // namespace sparki
