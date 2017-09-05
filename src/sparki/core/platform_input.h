#pragma once

#include <type_traits>
#include "math/math.h"


namespace sparki {
namespace core {

enum class key {
	alt_left,
	alt_right,
	// Menu key. Microsoft natural keyboard.
	apps,
	arrow_down,
	arrow_left,
	arrow_right,
	arrow_up,
	backspace,
	caps_lock,
	// ",<"
	comma,
	ctrl_left,
	ctrl_right,
	del,
	digit_0,
	digit_1,
	digit_2,
	digit_3,
	digit_4,
	digit_5,
	digit_6,
	digit_7,
	digit_8,
	digit_9,
	end,
	enter,
	escape,
	f1,
	f2,
	f3,
	f4,
	f5,
	f6,
	f7,
	f8,
	f9,
	f10,
	f11,
	f12,
	f13,
	f14,
	f15,
	home,
	insert,
	// -_
	minus,
	numpad_0,
	numpad_1,
	numpad_2,
	numpad_3,
	numpad_4,
	numpad_5,
	numpad_6,
	numpad_7,
	numpad_8,
	numpad_9,
	numpad_comma,
	numpad_devide,
	num_lock,
	numpad_minus,
	numpad_multiply,
	numpad_plus,
	page_down,
	page_up,
	pause,
	// .>
	period,
	// =+
	plus,
	print,
	prtsrc,
	// '"
	quote,
	// `~
	quote_back,
	scroll_lock,
	// for us ";:"
	semicolon,
	shift_left,
	shift_right,
	// for us "\|"
	slash,
	// "/?"
	slash_back,
	space,
	// "]}"
	square_bracket_close,
	// [{
	square_bracket_open,
	tab,
	// Windows key. Microsoft natural keyboard.
	win_left,
	// Windows key. Microsoft natural keyboard.
	win_right,
	a,
	b,
	c,
	d,
	e,
	f,
	g,
	h,
	i,
	j,
	k,
	l,
	m,
	n,
	o,
	p,
	q,
	r,
	s,
	t,
	u,
	v,
	w,
	x,
	y,
	z,
	unknown
};

enum class key_state : unsigned char {
	released,
	down
};

enum class mouse_buttons : unsigned char {
	none = 0,
	left = 1,
	middle = 2,
	right = 4
};

class event_listener_i {
public:

	virtual ~event_listener_i() {};


	virtual void on_keypress(key k, key_state s) = 0;

	virtual void on_mouse_click() = 0;

	virtual void on_mouse_move() = 0;

	virtual void on_resize_viewport(const math::uint2& size) = 0;
};

struct input_state final {

	input_state();

	// keyboard state.
	key_state keys[static_cast<size_t>(key::unknown) + 1];


	// Mouse position within the window's client area. 
	// The value is relative to the bottom-left corner.
	// The value is undefined if is_out returns true.
	math::uint2	mouse_position;

	// State of all the mouse's buttons.
	mouse_buttons mouse_buttons = mouse_buttons::none;

	// True if cursor left the client area of the window.
	bool mouse_is_out = true;
};


constexpr mouse_buttons operator|(mouse_buttons l, mouse_buttons r) noexcept
{
	using ut = std::underlying_type<mouse_buttons>::type;
	return static_cast<mouse_buttons>(static_cast<ut>(l) | static_cast<ut>(r));
}

constexpr mouse_buttons operator&(mouse_buttons l, mouse_buttons r) noexcept
{
	using ut = std::underlying_type<mouse_buttons>::type;
	return static_cast<mouse_buttons>(static_cast<ut>(l) & static_cast<ut>(r));
}

inline mouse_buttons& operator|=(mouse_buttons& l, mouse_buttons r) noexcept
{
	l = l | r;
	return l;
}

inline mouse_buttons& operator&=(mouse_buttons& l, mouse_buttons r) noexcept
{
	l = l & r;
	return l;
}

inline mouse_buttons operator~(mouse_buttons mb) noexcept
{
	auto res = mouse_buttons::none;

	if ((mb & mouse_buttons::left) == mouse_buttons::none)
		res |= mouse_buttons::left;

	if ((mb & mouse_buttons::middle) == mouse_buttons::none)
		res |= mouse_buttons::middle;

	if ((mb & mouse_buttons::right) == mouse_buttons::none)
		res |= mouse_buttons::right;

	return res;
}

inline bool is_down(const input_state& s, key k)
{
	return s.keys[static_cast<size_t>(k)] == key_state::down;
}

// true if left mouse button has been pressed.
inline bool is_mouse_left_down(const input_state& s) noexcept
{
	return (s.mouse_buttons & mouse_buttons::left) == mouse_buttons::left;
}

// true if middle mouse button has been pressed.
inline bool is_mouse_middle_down(const input_state& s) noexcept
{
	return (s.mouse_buttons & mouse_buttons::middle) == mouse_buttons::middle;
}

// true if right mouse button has been pressed.
inline bool is_mouse_right_down(const input_state& s) noexcept
{
	return (s.mouse_buttons & mouse_buttons::right) == mouse_buttons::right;
}

inline void set_key_state(input_state& input_state, key k, key_state s)
{
	input_state.keys[static_cast<size_t>(k)] = s;
}

} // namespace core
} // namespace sparki

