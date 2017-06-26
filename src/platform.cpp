#include "platform.h"

#include <cassert>


namespace {

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
	//if (g_app == nullptr)
	//	return DefWindowProc(hwnd, message, w_param, l_param);

	switch (message) {
		default:
			return DefWindowProc(hwnd, message, w_param, l_param);

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
	init_window(window_desc);
}

platform::~platform() noexcept
{
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

bool platform::pump_sys_messages() noexcept
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) return true;

		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

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
