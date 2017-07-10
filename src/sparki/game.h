#pragma once

#include "sparki/rnd/render.h"


namespace sparki {

class game final {
public:

	game(HWND p_hwnd, const uint2& viewport_size);

	game(game&&) = delete;
	game& operator=(game&&) = delete;


	void draw_frame();

	void resize_viewport(const uint2& size);

private:

	renderer	renderer_;
	bool		viewport_is_visible_;
};

} // namespace sparki
