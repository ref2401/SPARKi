#include "sparki/core/utility.h"

#include <cassert>


namespace {

void accumulate_exception_message_impl(std::string& dest, const std::exception& exc)
{
	dest.append("- ");
	dest.append(exc.what());
	dest.push_back('\n');

	try {
		std::rethrow_if_nested(exc);
	}
	catch (const std::exception& nested_exc) {
		accumulate_exception_message_impl(dest, nested_exc);
	}
	catch (...) {
		// https://www.youtube.com/watch?v=umDr0mPuyQc
		// do not throw non descendants of 
		assert(false);
	}
}

} // namespace


namespace sparki {
namespace core {

std::string make_exception_message(const std::exception& exc)
{
	std::string msg;
	accumulate_exception_message_impl(msg, exc);
	return msg;
}

} // namespace core
} // namespace sparki
