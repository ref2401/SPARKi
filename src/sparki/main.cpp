#include <iostream>
#include "sparki/platform.h"
#include "sparki/rnd/render.h"


int main() 
{
	using namespace sparki;

	bool leave_console_open = false;

	try {
		// init phase:
		const window_desc wnd_desc = {
			/* title */			"SPARKi",
			/* position */		uint2(90, 50),
			/* viewport_size */	uint2(960, 540),
			/* fullscreen */	false // NOTE(ref2401): true won't work cos it's not been implemented yet.
		};

		platform platform(wnd_desc);
		renderer renderer(platform.hwnd(), wnd_desc.viewport_size);

		// before the main loop phase:
		platform.show_window();

		// the main loop:
		while (true) {
			if (platform.pump_sys_messages()) 
				break;

			renderer.draw_frame();
		}


	}
	catch (const std::exception& e) {
		leave_console_open = true;

		const std::string msg = exception_message(e);
		std::cout << "----- Exception -----" << std::endl << msg << std::endl;
	}
	
	if (leave_console_open)
		std::cin.get();

	return 0;
}
