#pragma once

#include "sparki/rnd/render.h"


namespace sparki {

class engine final {
public:

	engine(HWND p_hwnd, const uint2& viewport_size);

	engine(engine&&) = delete;
	engine& operator=(engine&&) = delete;


	void draw_frame();

	void resize_viewport(const uint2& size);

private:

	renderer	renderer_;
	bool		viewport_is_visible_;
};

} // namespace sparki
