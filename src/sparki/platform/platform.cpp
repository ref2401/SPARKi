#include "sparki/platform/platform.h"

#include <cassert>


namespace {

using namespace sparki;

// The global is set by sparki::platform's ctor and reset to null by sparki::platform's dctor.
// It is meant to be used only in window_proc.
platform* gp_platform = nullptr;


sparki::mouse_buttons mouse_buttons(WPARAM w_param) noexcept
{
	auto buttons = mouse_buttons::none;

	int btn_state = GET_KEYSTATE_WPARAM(w_param);
	if ((btn_state & MK_LBUTTON) == MK_LBUTTON) buttons |= mouse_buttons::left;
	if ((btn_state & MK_MBUTTON) == MK_MBUTTON) buttons |= mouse_buttons::middle;
	if ((btn_state & MK_RBUTTON) == MK_RBUTTON) buttons |= mouse_buttons::right;

	return buttons;
}

inline uint2 point_2d(LPARAM l_param) noexcept
{
	return uint2(LOWORD(l_param), HIWORD(l_param));
}

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

	assert(p_hwnd == gp_platform->p_hwnd());

	switch (message) {
		default:
			return DefWindowProc(p_hwnd, message, w_param, l_param);

// ----- mouse -----

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		{
			assert(!gp_platform->mouse().is_out);
			gp_platform->enqueue_mouse_button(mouse_buttons(w_param));
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			if (p_hwnd != GetFocus())
				return DefWindowProc(p_hwnd, message, w_param, l_param);

			// p0 is relative to the top-left window corner
			// p is relative to the bottom-left window corner
			const uint2 p0 = point_2d(l_param);
			const uint2 vp = viewport_size(p_hwnd);
			const uint2 p(p0.x, vp.y - p0.y - 1);

			if (gp_platform->mouse().is_out)
				gp_platform->enqueue_mouse_enter(mouse_buttons(w_param), p);
			else 
				gp_platform->enqueue_mouse_move(p);

			TRACKMOUSEEVENT tme { sizeof(TRACKMOUSEEVENT), TME_LEAVE, p_hwnd, 0 };
			TrackMouseEvent(&tme);
			return 0;
		}

		case WM_MOUSELEAVE:
		{
			gp_platform->enqueue_mouse_leave();
			return 0;
		}
		
// ----- keyboard -----

		case WM_KEYDOWN:
		{
			return 0;
		}

		case WM_KEYUP:
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

bool platform::process_sys_messages(event_listener_i& listener)
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

			case sys_message::type::mouse_button:
			{
				mouse_.buttons = msg.mouse_buttons;
				listener.on_mouse_click();
				break;
			}

			case sys_message::type::mouse_enter:
			{
				mouse_.is_out = false;
				mouse_.buttons = msg.mouse_buttons;
				mouse_.position = msg.uint2;
				break;
			}

			case sys_message::type::mouse_leave:
			{
				mouse_.is_out = true;
				break;
			}

			case sys_message::type::mouse_move:
			{
				mouse_.position = msg.uint2;
				listener.on_mouse_move();
				break;
			}

			case sys_message::type::viewport_resize: 
			{
				listener.on_resize_viewport(msg.uint2);
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

void platform::enqueue_mouse_button(const mouse_buttons& mb)
{
	sys_message msg;
	msg.type = sys_message::type::mouse_button;
	msg.mouse_buttons = mb;
	sys_messages_.push_back(msg);
}

void platform::enqueue_mouse_enter(const mouse_buttons& mb, const uint2 p)
{
	sys_message msg;
	msg.type = sys_message::type::mouse_enter;
	msg.mouse_buttons = mb;
	msg.uint2 = p;
	sys_messages_.push_back(msg);
}

void platform::enqueue_mouse_leave()
{
	sys_message msg;
	msg.type = sys_message::type::mouse_leave;
	sys_messages_.push_back(msg);
}

void platform::enqueue_mouse_move(const uint2& p)
{
	sys_message msg;
	msg.type = sys_message::type::mouse_move;
	msg.uint2 = p;
	sys_messages_.push_back(msg);
}

void platform::enqueue_window_resize()
{
	sys_message msg;
	msg.type = sys_message::type::viewport_resize;
	msg.uint2 = viewport_size(p_hwnd_);
	sys_messages_.push_back(msg);
}

// ----- funcs -----

bool is_valid_window_desc(const window_desc& desc) noexcept
{
	return (desc.title.length() > 0)
		&& (desc.viewport_size > 0);
}

} // namespace sparki
