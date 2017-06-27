#pragma once

#include "sparki/utility.h"
#include "math/math.h"
#include <windows.h>

using namespace math;


namespace sparki {

struct window_desc final {
	std::string title;
	uint2 position;
	uint2 viewport_size;
	bool fullscreen;
};

class platform final {
public:

	explicit platform(const window_desc& window_desc);

	platform(platform&&) = delete;
	platform& operator=(platform&&) = delete;

	~platform() noexcept;


	HWND hwnd() noexcept
	{
		return p_hwnd_;
	}

	// Processes all the system messages that are in the message queue at the moment.
	// Returns true if the application has to terminate.
	bool pump_sys_messages() noexcept;

	void show_window() noexcept;

private:

	static constexpr char* window_class_name = "sparki_window_class";


	void init_window(const window_desc& window_desc);


	HWND p_hwnd_;
};


bool is_valid_window_desc(const window_desc& desc) noexcept;


} // namespace sparki
