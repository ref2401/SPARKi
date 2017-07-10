#include "sparki/game.h"



namespace sparki {

// ----- game -----

game::game(HWND p_hwnd, const uint2& viewport_size)
	: renderer_(p_hwnd, viewport_size),
	viewport_is_visible_(true)
{

}

void game::draw_frame()
{
	if (viewport_is_visible_)
		renderer_.draw_frame();
}

void game::resize_viewport(const uint2& size)
{
	if (size.x == 0 || size.y == 0) {
		viewport_is_visible_ = false;
		return;
	}

	viewport_is_visible_ = true;
	renderer_.resize_viewport(size);
}

} // namespace sparki
