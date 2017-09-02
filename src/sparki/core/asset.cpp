#include "sparki/core/asset.h"

#include <cassert>
#include <cstdio>
#include <memory>


#pragma warning(push)
#pragma warning(disable:4996) // C4996 'fopen': This function or variable may be unsafe.

namespace sparki {
namespace core {

// ----- hlsl_compute_desc -----

hlsl_compute_desc::hlsl_compute_desc(const char* p_filename)
	: source_code(read_text(p_filename)),
	source_filename(p_filename)
{}

// ----- hlsl_shader_desc -----

hlsl_shader_desc::hlsl_shader_desc(const char* p_filename, bool tesselation_stage)
	: source_code(read_text(p_filename)),
	source_filename(p_filename),
	tesselation_stage(tesselation_stage)
{}

// ----- funcs -----

std::string read_text(const char* p_filename)
{
	std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(p_filename, "rb"), &std::fclose);
	ENFORCE(file, "Failed to open file ", p_filename);

	// byte count
	std::fseek(file.get(), 0, SEEK_END);
	const size_t byte_count = std::ftell(file.get());
	std::rewind(file.get());
	if (byte_count == 0) return {};

	// read the file's contents
	std::string str;
	str.reserve(byte_count);

	char buffer[1024];
	while (std::feof(file.get()) == 0) {
		// ab - actual number of bytes read.
		const size_t ab = std::fread(buffer, sizeof(char), std::extent<decltype(buffer)>::value, file.get());
		str.append(buffer, ab);
	}

	return str;
}

} // namespace core
} // namespace sparki

#pragma warning(pop)
