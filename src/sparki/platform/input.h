#pragma once

#include <type_traits>
#include "math/math.h"


namespace sparki {

enum class mouse_buttons : unsigned char {
	none	= 0,
	left	= 1,
	middle	= 2,
	right	= 4
};

struct mouse final {
	// Mouse position within the window's client area. 
	// The value is relative to the bottom-left corner.
	// The value is undefined if is_out returns true.
	math::uint2	position;
	
	// State of all the mouse's buttons.
	mouse_buttons buttons = mouse_buttons::none;
	
	// True if cursor left the client area of the window.
	bool is_out = true;
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

} // namespace sparki

