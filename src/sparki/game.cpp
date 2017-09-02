#include "sparki/game.h"

#include <cassert>


namespace sparki {

// ----- game -----

game::game(HWND p_hwnd, const uint2& viewport_size, const mouse& mouse)
	:mouse_(mouse),
	renderer_(p_hwnd, viewport_size),
	imgui_io_(ImGui::GetIO()),
	viewport_is_visible_(true),
	camera_(float3::unit_z, float3::zero)
{
	imgui_io_.ImeWindowHandle = p_hwnd;


	frame_.projection_matrix = math::perspective_matrix_directx(
		game::projection_fov, aspect_ratio(viewport_size),
		game::projection_near, game::projection_far);
}

void game::draw_frame(float interpolation_factor)
{
	assert(0.0f <= interpolation_factor && interpolation_factor <= 1.0f);
	if (!viewport_is_visible_) return;

	frame_.camera_position = lerp(camera_.position, camera_.prev_position, interpolation_factor);
	frame_.camera_target = lerp(camera_.target, camera_.prev_target, interpolation_factor);
	frame_.camera_up = lerp(camera_.up, camera_.prev_up, interpolation_factor);

	renderer_.draw_frame(frame_);
}

void game::update()
{
	camera_.prev_position = camera_.position;
	camera_.prev_target = camera_.target;
	camera_.prev_up = camera_.prev_up;

	if (!approx_equal(camera_.roll_angles, float2::zero)) {
		const float dist = len(camera_.target - camera_.position);
		
		float3 fwd = normalize(camera_.target - camera_.position);
		float3 ox = cross(fwd, camera_.up);
		ox.y = 0.0f; // ox is always parallel the world's OX.
		ox = normalize(ox);

		if (!approx_equal(0.0f, camera_.roll_angles.y)) {
			const quat q = from_axis_angle_rotation(float3::unit_y, camera_.roll_angles.y);
			camera_.position = dist * normalize(rotate(q, camera_.position));

			ox = rotate(q, ox);
			ox.y = 0.0f;
			ox = normalize(ox);
		}

		if (!approx_equal(0.0f, camera_.roll_angles.x)) {
			const quat q = from_axis_angle_rotation(ox, camera_.roll_angles.x);
			camera_.position = dist * normalize(rotate(q, camera_.position));
		}

		fwd = normalize(camera_.target - camera_.position);
		camera_.up = normalize(cross(ox, fwd));
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

	imgui_io_.DisplaySize = ImVec2(float(size.x), float(size.y));

	viewport_is_visible_ = true;
	frame_.projection_matrix = math::perspective_matrix_directx(game::projection_fov, 
		aspect_ratio(size), game::projection_near, game::projection_far);
	renderer_.resize_viewport(size);
}

void game::terminate()
{
	ImGui::Shutdown();
}

} // namespace sparki
