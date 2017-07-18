#pragma once

#include <vector>
#include "sparki/game.h"
#include "sparki/utility.h"
#include "math/math.h"
#include <windows.h>

using namespace math;


namespace sparki {

struct window_desc final {
	std::string title;
	uint2		position;
	uint2		viewport_size;
	bool		fullscreen;
};

class platform final {
public:

	explicit platform(const window_desc& window_desc);

	platform(platform&&) = delete;
	platform& operator=(platform&&) = delete;

	~platform() noexcept;


	HWND p_hwnd() noexcept
	{
		return p_hwnd_;
	}

	const mouse& mouse() const noexcept
	{
		return mouse_;
	}

	// Processes all the system messages that are in the message queue at the moment.
	// Returns true if the application has to terminate.
	bool process_sys_messages(game& game);

	void show_window() noexcept;


	void enqueue_mouse_button(const mouse_buttons& mb);

	void enqueue_mouse_enter(const mouse_buttons& mb, const uint2 p);

	void enqueue_mouse_leave();

	void enqueue_mouse_move(const uint2& p);

	void enqueue_window_resize();


private:

	// The struct stores any system message's data.
	// type tells what kind of message occured. 
	// Which fields contain data depends on type of a sys message.
	struct sys_message final {
		enum class type : unsigned char {
			none,
			mouse_button,
			mouse_enter,
			mouse_leave,
			mouse_move,
			viewport_resize
		};

		type			type;
		mouse_buttons	mouse_buttons;
		uint2			uint2;
	};

	static constexpr char* window_class_name = "sparki_window_class";


	void init_window(const window_desc& window_desc);


	std::vector<sys_message>	sys_messages_;
	sparki::mouse				mouse_;
	HWND						p_hwnd_;
};


bool is_valid_window_desc(const window_desc& desc) noexcept;


} // namespace sparki
