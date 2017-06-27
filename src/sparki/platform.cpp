#include "sparki/platform.h"

#include <cassert>


namespace {

// The global is set by sparki::platform's ctor and reset to null by sparki::platform's dctor.
// It meant to be used only in window_proc.
sparki::platform* gp_platform = nullptr;

struct sys_message final {
	enum class type : unsigned char {
		none,
		window_resize
	};

	type type;
	uint2 uint2;
};

// Retrieves and dispatches all the system messages that are in the message queue at the moment.
// Returns true if the application has to terminate.
bool pump_sys_messages() noexcept
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) return true;

		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	return false;
}

// Returns viewport size of the specified window.
inline uint2 viewport_size(HWND p_hwnd)
{
	assert(p_hwnd);

	RECT rect;
	GetClientRect(p_hwnd, &rect);
	return uint2(rect.right - rect.left, rect.bottom - rect.top);
}

LRESULT CALLBACK window_proc(HWND p_hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
	if (!gp_platform)
		return DefWindowProc(p_hwnd, message, w_param, l_param);

	switch (message) {
		default:
			return DefWindowProc(p_hwnd, message, w_param, l_param);

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		{
			return 0;
		}

		case WM_KEYDOWN:
		{
			return 0;
		}

		case WM_KEYUP:
		{
			return 0;
		}

		//case WM_MOUSEMOVE:
		//{
		//	if (!g_app->window().focused())
		//		return DefWindowProc(hwnd, message, w_param, l_param);

		//	uint2 p = get_point(l_param);

		//	if (g_app->mouse().is_out()) {
		//		g_app->enqueue_mouse_enter_message(get_mouse_buttons(w_param), p);
		//	}
		//	else {
		//		g_app->enqueue_mouse_move_message(p);
		//	}

		//	TRACKMOUSEEVENT tme{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, g_app->window().hwnd(), 0 };
		//	TrackMouseEvent(&tme);
		//	return 0;
		//}

		case WM_MOUSELEAVE:
		{
			return 0;
		}


		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}

		case WM_SIZE:
		{
			gp_platform->enqueue_window_resize();
			return 0;
		}
	}
}

} // namespace


namespace sparki {

// ----- platform  -----

platform::platform(const window_desc& window_desc)
{
	assert(is_valid_window_desc(window_desc));
	sys_messages_.reserve(256);
	init_window(window_desc);

	gp_platform = this;
}

platform::~platform() noexcept
{
	gp_platform = nullptr;

	if (IsWindow(p_hwnd_))
		DestroyWindow(p_hwnd_);
	p_hwnd_ = nullptr;

	// window class
	UnregisterClass(platform::window_class_name, GetModuleHandle(nullptr));
}

void platform::enqueue_window_resize()
{
	sys_message msg;
	msg.type = sys_message::type::viewport_resize;
	msg.uint2 = viewport_size(p_hwnd_);
	sys_messages_.push_back(msg);
}

void platform::init_window(const window_desc& desc)
{
	HINSTANCE p_hinstance = GetModuleHandle(nullptr);
	assert(!desc.fullscreen); // NOTE(ref2401): Not implemented

	// register the window's class
	WNDCLASSEX wnd_class = {};
	wnd_class.cbSize = sizeof(wnd_class);
	wnd_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wnd_class.lpfnWndProc = window_proc;
	wnd_class.cbClsExtra = 0;
	wnd_class.cbWndExtra = 0;
	wnd_class.hInstance = p_hinstance;
	wnd_class.hIcon = nullptr;
	wnd_class.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
	wnd_class.hbrBackground = nullptr;
	wnd_class.lpszMenuName = nullptr;
	wnd_class.lpszClassName = platform::window_class_name;

	ATOM reg_res = RegisterClassEx(&wnd_class);
	assert(reg_res != 0);

	// create a window
	RECT wnd_rect;
	wnd_rect.left = desc.position.x;
	wnd_rect.top = desc.position.y;
	wnd_rect.right = desc.position.x + desc.viewport_size.x;
	wnd_rect.bottom = desc.position.y + desc.viewport_size.y;
	AdjustWindowRectEx(&wnd_rect, WS_OVERLAPPEDWINDOW, false, WS_EX_APPWINDOW);

	p_hwnd_ = CreateWindowEx(WS_EX_APPWINDOW, platform::window_class_name, desc.title.c_str(),
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		wnd_rect.left, wnd_rect.top, wnd_rect.right - wnd_rect.left, wnd_rect.bottom - wnd_rect.top,
		nullptr, nullptr, p_hinstance, nullptr);
	assert(p_hwnd_);
}

bool platform::process_sys_messages(engine& engine)
{
	if (pump_sys_messages()) return true; // NOTE(ref2401): true - app has to terminate.
	if (sys_messages_.empty()) return false;

	for (const auto& msg : sys_messages_) {
		switch (msg.type) {
			default: 
			{
				assert(false);
				break;
			}

			case sys_message::type::viewport_resize: 
			{
				engine.resize_viewport(msg.uint2);
				break;
			}
		}
	}

	sys_messages_.clear();
	return false;
}

void platform::show_window() noexcept
{
	ShowWindow(p_hwnd_, SW_SHOW);
	SetForegroundWindow(p_hwnd_);
	SetFocus(p_hwnd_);
}

// ----- funcs -----

bool is_valid_window_desc(const window_desc& desc) noexcept
{
	return (desc.title.length() > 0)
		&& (desc.viewport_size > 0);
}

} // namespace sparki
