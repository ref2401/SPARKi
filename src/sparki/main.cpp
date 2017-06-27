#include <iostream>
#include "sparki/platform.h"
#include "sparki/rnd/render.h"
#include "ts/task_system.h"


void sparki_main()
{
	using namespace sparki;

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

int main() 
{
	const ts::task_system_desc ts_desc = {
		/* thread_count */				2,
		/* fiber_count */				16,
		/* fiber_stack_byte_count */	128,
		/* queue_size */				64,
		/* queue_immediate_size */		8
	};
	
	bool leave_console_open = false;

	try {
		ts::launch_task_system(ts_desc, sparki_main);
	}
	catch (const std::exception& e) {
		leave_console_open = true;

		const std::string msg = sparki::exception_message(e);
		std::cout << "----- Exception -----" << std::endl << msg << std::endl;
	}

	if (leave_console_open)
		std::cin.get();

	return 0;
}
