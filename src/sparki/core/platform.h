#pragma once

#include <vector>
#include "sparki/core/platform_input.h"
#include "sparki/core/utility.h"
#include "math/math.h"
#include <windows.h>

using namespace math;


namespace sparki {
namespace core {

struct window_desc final {
	std::string title;
	uint2		position;
	uint2		viewport_size;
	bool		fullscreen;
};

class platform_system final {
public:

	explicit platform_system(const window_desc& window_desc);

	platform_system(platform_system&&) = delete;
	platform_system& operator=(platform_system&&) = delete;

	~platform_system() noexcept;


	HWND p_hwnd() noexcept
	{
		return p_hwnd_;
	}

	const input_state& mouse() const noexcept
	{
		return input_state_;
	}

	void before_main_loop();

	// Processes all the system messages that are in the message queue at the moment.
	// Returns true if the application has to terminate.
	bool process_sys_messages(event_listener_i& listener);


	void enqueue_keypress(key k, key_state s);

	void enqueue_mouse_button(mouse_buttons mb);

	void enqueue_mouse_enter(mouse_buttons mb, const uint2& p);

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
			keypress,
			mouse_button,
			mouse_enter,
			mouse_leave,
			mouse_move,
			viewport_resize
		};

		type			type;
		key				key;
		key_state		key_state;
		mouse_buttons	mouse_buttons;
		uint2			uint2;
	};

	static constexpr char* window_class_name = "sparki_window_class";


	void init_window(const window_desc& window_desc);


	std::vector<sys_message>	sys_messages_;
	sparki::core::input_state	input_state_;
	HWND						p_hwnd_;
};


bool is_valid_window_desc(const window_desc& desc) noexcept;


} // namespace core
} // namespace sparki
