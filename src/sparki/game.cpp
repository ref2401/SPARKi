#include "sparki/game.h"

#include <cassert>


namespace sparki {

// ----- game_system -----

game_system::game_system(HWND p_hwnd, const uint2& viewport_size, const core::input_state& input_state)
	:input_state_(input_state),
	render_system_(p_hwnd, viewport_size),
	viewport_is_visible_(true),
	camera_(float3::unit_z, float3::zero)
{
	p_material_editor_view_ = std::make_unique<material_editor_view>(p_hwnd, render_system_.material_editor_tool());

	frame_.projection_matrix = math::perspective_matrix_directx(
		game_system::projection_fov, aspect_ratio(viewport_size),
		game_system::projection_near, game_system::projection_far);
}

void game_system::draw_frame(float interpolation_factor)
{
	assert(0.0f <= interpolation_factor && interpolation_factor <= 1.0f);
	if (!viewport_is_visible_) return;

	ImGui::NewFrame();
	p_material_editor_view_->show();

	ImGui::ShowTestWindow();

	frame_.material = render_system_.material_editor_tool().current_material(); // NOTE(ref2401): this crap is temporary.
	frame_.camera_position = lerp(camera_.position, camera_.prev_position, interpolation_factor);
	frame_.camera_target = lerp(camera_.target, camera_.prev_target, interpolation_factor);
	frame_.camera_up = lerp(camera_.up, camera_.prev_up, interpolation_factor);

	ImGui::Render(); // NOTE(ref2401): render_system_ does the actual imgui rendering.
	frame_.p_imgui_draw_data = ImGui::GetDrawData();
	render_system_.draw_frame(frame_);
}

void game_system::update()
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

void game_system::on_keypress(core::key, core::key_state)
{

}

void game_system::on_mouse_click()
{
}

void game_system::on_mouse_move()
{
	const float2 curr_pos(float(input_state_.mouse_position.x), float(input_state_.mouse_position.y));
	const float2 diff = curr_pos - camera_.mouse_position_prev;
	camera_.mouse_position_prev = curr_pos;

	if (!is_mouse_middle_down(input_state_) || approx_equal(diff, float2::zero)) return;

	// mouse offset by x means rotation around OY (yaw)
	const bool x_offset_sufficient = !approx_equal(diff.x, 0.0f, 0.01f);
	if (x_offset_sufficient)
		camera_.roll_angles.y += (diff.x > 0.0f) ? -pi_128 : pi_128;

	// mouse offset by x means rotation around OX (pitch)
	const bool y_offset_sufficient = !approx_equal(diff.y, 0.0f, 0.01f);
	if (y_offset_sufficient)
		camera_.roll_angles.x += (diff.y > 0.0f) ? pi_128 : -pi_128;
}

void game_system::on_resize_viewport(const uint2& size)
{
	if (size.x == 0 || size.y == 0) {
		viewport_is_visible_ = false;
		return;
	}

	ImGui::GetIO().DisplaySize = ImVec2(float(size.x), float(size.y));

	viewport_is_visible_ = true;
	frame_.projection_matrix = math::perspective_matrix_directx(game_system::projection_fov, 
		aspect_ratio(size), game_system::projection_near, game_system::projection_far);
	render_system_.resize_viewport(size);
}

void game_system::terminate()
{
	ImGui::Shutdown();
}

} // namespace sparki
