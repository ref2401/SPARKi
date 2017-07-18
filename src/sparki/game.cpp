#include "sparki/game.h"

#include <cassert>


namespace sparki {

// ----- game -----

game::game(HWND p_hwnd, const uint2& viewport_size, const mouse& mouse)
	: mouse_(mouse),
	renderer_(p_hwnd, viewport_size),
	viewport_is_visible_(true)
{
	camera_.transform_curr = camera_transform(float3::zero, -float3::unit_z);
	camera_.transform_prev = camera_transform(camera_.transform_curr);

	frame_.projection_matrix = math::perspective_matrix_directx(
		game::projection_fov, aspect_ratio(viewport_size),
		game::projection_near, game::projection_far);
}

void game::draw_frame(float interpolation_factor)
{
	assert(0.0f <= interpolation_factor && interpolation_factor <= 1.0f);
	if (!viewport_is_visible_) return;

	const camera_transform vp = lerp_camera_transform(camera_, interpolation_factor);
	frame_.camera_position = vp.position;
	frame_.camera_target = vp.target;
	frame_.camera_up = vp.up;

	renderer_.draw_frame(frame_);
}

void game::update()
{
	camera_.transform_prev = camera_.transform_curr;

	if (!approx_equal(camera_.roll_angles, float2::zero)) {
		const float dist = distance(camera_.transform_curr);
		float3 ox = cross(forward(camera_.transform_curr), camera_.transform_curr.up);
		ox.y = 0.0f; // ox is always parallel the world's OX.
		ox = normalize(ox);

		if (!approx_equal(camera_.roll_angles.y, 0.0f)) {
			const quat q = from_axis_angle_rotation(float3::unit_y, camera_.roll_angles.y);
			camera_.transform_curr.position = dist * normalize(rotate(q, camera_.transform_curr.position));

			ox = rotate(q, ox);
			ox.y = 0.0f;
			ox = normalize(ox);
		}

		if (!approx_equal(camera_.roll_angles.x, 0.0f)) {
			const quat q = from_axis_angle_rotation(ox, camera_.roll_angles.x);
			camera_.transform_curr.position = dist * normalize(rotate(q, camera_.transform_curr.position));
		}

		camera_.transform_curr.up = normalize(cross(ox, forward(camera_.transform_curr)));
	}

	camera_.roll_angles = float2::zero;
}

void game::on_mouse_click()
{
}

void game::on_mouse_move()
{
	const float2 curr_pos(float(mouse_.position.x), float(mouse_.position.y));
	const float2 diff = curr_pos - camera_.mouse_position_prev;
	camera_.mouse_position_prev = curr_pos;

	if (!middle_down(mouse_) || approx_equal(diff, float2::zero)) return;

	// mouse offset by x means rotation around OY (yaw)
	const bool x_offset_sufficient = !approx_equal(diff.x, 0.0f, 0.01f);
	if (x_offset_sufficient)
		camera_.roll_angles.y += (diff.x > 0.0f) ? -pi_128 : pi_128;

	// mouse offset by x means rotation around OX (pitch)
	const bool y_offset_sufficient = !approx_equal(diff.y, 0.0f, 0.01f);
	if (y_offset_sufficient)
		camera_.roll_angles.x += (diff.y > 0.0f) ? pi_128 : -pi_128;
}

void game::on_resize_viewport(const uint2& size)
{
	if (size.x == 0 || size.y == 0) {
		viewport_is_visible_ = false;
		return;
	}

	frame_.projection_matrix = math::perspective_matrix_directx(
		game::projection_fov, aspect_ratio(size),
		game::projection_near, game::projection_far);

	viewport_is_visible_ = true;
	renderer_.resize_viewport(size);
}

} // namespace sparki
