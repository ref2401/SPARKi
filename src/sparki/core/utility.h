#pragma once

#include <exception>
#include <sstream>


#define EXCEPTION_MSG(...) sparki::core::concat(__FILE__, '(', __LINE__, "): ", __VA_ARGS__)

#define ENFORCE(expression, ...)								\
	if (!(expression)) {										\
		throw std::runtime_error(EXCEPTION_MSG(__VA_ARGS__));	\
	}


namespace sparki {
namespace core {

namespace intrinsic {

inline void concat_impl(std::ostringstream&) {}

template<typename T, typename... Args>
void concat_impl(std::ostringstream& string_stream, const T& obj, const Args&... args)
{
	string_stream << obj;
	concat_impl(string_stream, args...);
}

} // namespace intrinsic


// Returns the number of bytes occupied by elements of container.
// Take into account that container by itself may occupy more space.
template<typename Container>
inline size_t byte_count(const Container& container) noexcept
{
	return sizeof(typename Container::value_type) * container.size();
}

// Concatenates any number of std::string, c-style string or char arguments into one string.
template<typename... Args>
inline std::string concat(const Args&... args)
{
	std::ostringstream string_stream;
	sparki::core::intrinsic::concat_impl(string_stream, args...);
	return string_stream.str();
}

// Constructs exception message string considering all the nested exceptions.
// Each exception message is formatted as a new line and starts with " - " prefix. 
std::string make_exception_message(const std::exception& exc);

// Calculates how many bytes are in the specified number of kilobytes.
// Uses base 2 definition: 1 Kb = 1024 bytes = 2^10 bytes.
constexpr size_t kilobytes(size_t amount) noexcept
{
	return amount * 1024;
}

// Calculates how many bytes are in the specified number of megabytes.
// Uses base 2 definition: 1 Mb = 1048576 bytes = (1024 * 1024) bytes = 2^20 bytes.
constexpr size_t megabytes(size_t amount) noexcept
{
	return amount * 1048576;
}

} // namespace core
} // namespace sparki
