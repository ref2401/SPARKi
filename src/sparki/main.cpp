#include <iostream>
#include "sparki/core/asset.h"
#include "sparki/core/platform.h"
#include "sparki/game.h"
#include "ts/task_system.h"


void sparki_main()
{
	using namespace sparki;
	using namespace sparki::core;

	// init phase:
	const window_desc wnd_desc = {
		/* title */			"SPARKi",
		/* position */		uint2(90, 50),
		/* viewport_size */	uint2(960, 540),
		/* fullscreen */	false // NOTE(ref2401): true won't work cos it's not been implemented yet.
	};

	platform_system platform_system(wnd_desc);
	game_system game(platform_system.p_hwnd(), wnd_desc.viewport_size, platform_system.mouse());

	// before the main loop phase:
	platform_system.show_window();

	// the main loop:
	while (true) {
		if (platform_system.process_sys_messages(game)) break;

		game.update();
		game.draw_frame(1.0f);
	}

	game.terminate();
}

int main() 
{
	bool keep_console_shown = false;
	const ts::task_system_desc ts_desc = {
		/* thread_count */				2,
		/* fiber_count */				16,
		/* fiber_stack_byte_count */	128,
		/* queue_size */				64,
		/* queue_immediate_size */		8
	};

	try {
		//sparki::convert_fbx_to_geo("../../data/geometry/sphere.fbx", "../../data/geometry/sphere.geo");

		auto report = ts::launch_task_system(ts_desc, sparki_main);

		std::cout << "----- Task System Report ----- " << std::endl
			<< "task executed: " << report.task_count << std::endl
			<< "immediate task executed: " << report.task_immediate_count << std::endl;
	}
	catch (const std::exception& e) {
		keep_console_shown = true;

		const std::string msg = sparki::core::make_exception_message(e);
		std::cout << "----- Exception -----" << std::endl << msg << std::endl;
	}

	if (keep_console_shown)
		std::cin.get();

	return 0;
}
