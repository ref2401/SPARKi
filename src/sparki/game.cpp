#include "sparki/game.h"

#include <cassert>


namespace sparki {

// ----- game -----

game::game(HWND p_hwnd, const uint2& viewport_size)
	: renderer_(p_hwnd, viewport_size),
	viewport_is_visible_(true)
{
	camera_.transform_curr_ = camera_transform(float3::zero, -float3::unit_z);
	camera_.transform_prev_ = camera_transform(camera_.transform_curr_);

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
{}

void game::on_mouse_click()
{
}

void game::on_mouse_move()
{

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
