#include <iostream>
#include "sparki/platform.h"
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
	engine engine(platform.hwnd(), wnd_desc.viewport_size);

	// before the main loop phase:
	platform.show_window();

	// the main loop:
	while (true) {
		if (platform.process_sys_messages(engine))
			break;

		engine.draw_frame();
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

	try {
		auto report = ts::launch_task_system(ts_desc, sparki_main);

		std::cout << "----- Task System Report ----- " << std::endl
			<< "task executed: " << report.task_count << std::endl
			<< "immediate task executed: " << report.task_immediate_count << std::endl;
	}
	catch (const std::exception& e) {
		const std::string msg = sparki::exception_message(e);
		std::cout << "----- Exception -----" << std::endl << msg << std::endl;
	}

	std::cin.get();
	return 0;
}
