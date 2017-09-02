#pragma once

#include "sparki/platform/platform_input.h"
#include "sparki/rnd/rnd.h"
#include "sparki/rnd/rnd_imgui.h"


namespace sparki {

// Tracks camera's current and previous position and orientation.
// Also stores mouse input and appropriate roll angles which.
// Mouse input messages are converted into roll angles (game::on_mouse_move).
// Roll angles are converted into camera_transform (game::update).
// camera_transform is used to set frame's camera properties.
struct camera final {

	camera() noexcept = default;

	camera(const float3& p, const float3& t, const float3 u = float3::unit_y)
		: position(p), target(t), up(u),
		prev_position(p), prev_target(t), prev_up(u)
	{}

	float3 position = -float3::unit_z;
	float3 target = float3::zero;
	float3 up = float3::unit_y;
	float3 prev_position;
	float3 prev_target;
	float3 prev_up;

	float2 roll_angles;
	float2 mouse_position_prev;
};

class game final : public event_listener_i {
public:

	game(HWND p_hwnd, const uint2& viewport_size,
		const mouse& mouse);

	game(game&&) = delete;
	game& operator=(game&&) = delete;


	void draw_frame(float interpolation_factor);

	void update();

	void on_mouse_click() override;

	void on_mouse_move() override;

	void on_resize_viewport(const uint2& size) override;

	void terminate();

private:

	static constexpr float projection_fov = pi_3;
	static constexpr float projection_near = 0.1f;
	static constexpr float projection_far = 1000.0f;


	// ::game context::
	const mouse&	mouse_;
	rnd::renderer	renderer_;
	ImGuiIO&		imgui_io_;
	bool			viewport_is_visible_;
	// ::game state:: 
	camera			camera_;
	rnd::frame		frame_;
};

} // namespace sparki
