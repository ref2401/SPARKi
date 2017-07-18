#pragma once

#include "sparki/platform/input.h"
#include "sparki/rnd/render.h"


namespace sparki {

// Position and orientation of a camera (viewer).
struct camera_transform final {
	camera_transform() noexcept = default;

	camera_transform(const float3& p, const float3& t, const float3 up = float3::unit_y)
		: position(p), target(t), up(up)
	{}

	float3 position = -float3::unit_z;
	float3 target = float3::zero;
	float3 up = float3::unit_y;
};

// Tracks camera's current and previous transforms.
// Also stores mouse input and appropriate roll angles which.
// Mouse input messages are converted into roll angles (game::on_mouse_move).
// Roll angles are converted into camera_transform (game::update).
// camera_transform is used to set frame's camera properties.
struct camera final {
	camera_transform	transform_curr;
	camera_transform	transform_prev;
	float2				roll_angles;
	float2				mouse_position_prev;
};

class game final {
public:

	game(HWND p_hwnd, const uint2& viewport_size,
		const mouse& mouse);

	game(game&&) = delete;
	game& operator=(game&&) = delete;


	void draw_frame(float interpolation_factor);

	void update();

	void on_mouse_click();

	void on_mouse_move();

	void on_resize_viewport(const uint2& size);

private:

	static constexpr float projection_fov = pi_3;
	static constexpr float projection_near = 0.1f;
	static constexpr float projection_far = 1000.0f;

	camera			camera_;
	frame			frame_;
	const mouse&	mouse_;
	renderer		renderer_;
	bool			viewport_is_visible_;
};


// Returns the distance bitween position and target of the specifie camera_transform.
inline float distance(const camera_transform& t) noexcept
{
	return len(t.target - t.position);
}

// Direction in which the specified camera_transform points to.
inline float3 forward(const camera_transform& t) noexcept
{
	return normalize(t.target - t.position);
}

inline camera_transform lerp_camera_transform(const camera& c, float factor) noexcept
{
	return camera_transform(
		lerp(c.transform_curr.position, c.transform_prev.position, factor),
		lerp(c.transform_curr.target, c.transform_prev.target, factor),
		lerp(c.transform_curr.up, c.transform_prev.up, factor)
	);
}

} // namespace sparki
