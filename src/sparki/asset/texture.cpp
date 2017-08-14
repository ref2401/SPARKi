#include "sparki/asset/texture.h"

#include <cassert>
#include <cstdio>
#include <memory>
#include "sparki/utility.h"
#pragma warning(push)
#pragma warning(disable:4244) // C4244 '=': conversion from 'int' to 'stbi__uint16', possible loss of data.
#pragma warning(disable:4456) // C4456 '=': declaration of 'k' hides previous local declaration.
	#define STB_IMAGE_IMPLEMENTATION
	#define STBI_FAILURE_USERMSG 
	#include "stb/stb_image.h"
#pragma warning(pop)


#pragma warning(push)
#pragma warning(disable:4996) // C4996 'fopen': This function or variable may be unsafe.

namespace sparki {

// ----- texture_data -----

texture_data_new::texture_data_new(texture_type type, const math::uint3& size, 
	uint32_t mipmap_count, uint32_t array_size, pixel_format fmt)
	: type(type), size(size), mipmap_count(mipmap_count), array_size(array_size), format(fmt)
{
	assert(type != texture_type::unknown);
	assert(size.z == 1); // the case z > 1 has not been implemented yet.
	assert(mipmap_count > 0);
	assert(array_size > 0);
	assert(fmt != pixel_format::none);

	const size_t c = byte_count(type, size, mipmap_count, array_size, fmt);
	buffer.resize(c);
}

// ----- funcs -----

size_t byte_count(pixel_format fmt) noexcept
{
	switch (fmt) {
		default:
		case pixel_format::none:		return 0;

		case pixel_format::rg_16f:		return sizeof(float);		// 2 * sizeof(float) / 2 = sizeof(float)
		case pixel_format::rgba_16f:	return 2 * sizeof(float);	// 4 * sizeof(float) / 2 = 2 * sizeof(float)

		case pixel_format::rg_32f:		return 2 * sizeof(float);
		case pixel_format::rgb_32f:		return 3 * sizeof(float);
		case pixel_format::rgba_32f:	return 4 * sizeof(float);
		case pixel_format::red_8:		return 1;
		case pixel_format::rg_8:		return 2;
		case pixel_format::rgb_8:		return 3;
		case pixel_format::rgba_8:		return 4;
	}
}

size_t byte_count(texture_type type, const math::uint3& size, uint32_t mipmap_count,
	uint32_t array_size, pixel_format fmt) noexcept
{
	assert(type != texture_type::unknown);
	assert(size.z == 1); // the case z > 1 has not been implemented yet.
	assert(mipmap_count > 0);
	assert(array_size > 0);
	assert(fmt != pixel_format::none);

	const size_t fmt_bytes = byte_count(fmt);

	size_t array_slice_bytes = 0;
	for (uint32_t i = 0; i < mipmap_count; ++i) {
		const uint32_t wo = size.x >> i;
		const uint32_t ho = size.y >> i;
		assert((wo > 0) || (ho > 0)); // ensure size and mipmap_level_count compatibility

		array_slice_bytes += std::max(1u, wo) * std::max(1u, ho) * fmt_bytes;
	}

	return (type == texture_type::texture_2d) 
		? array_slice_bytes
		: array_slice_bytes * array_size;
}

bool is_valid_texture_data(const texture_data_new& td) noexcept
{
	assert(td.size.z == 1); // the case z > 1 has not been implemented yet.

	const bool res = (td.type != texture_type::unknown)
		&& (td.size > 0)
		&& (td.mipmap_count > 0)
		&& (td.array_size > 0)
		&& (td.format != pixel_format::none);

	if (!res) return false;
	if (td.type == texture_type::texture_cube && td.array_size != 6) return false;

	// check size and mipmap_count compatibility
	for (uint32_t i = 0; i < td.mipmap_count; ++i) {
		const uint32_t w = td.size.x >> i;
		const uint32_t h = td.size.y >> i;

		if (w == 0 || h == 0) return false;
	}

	// check whether the buffer's memory is large enough
	const size_t expected_bc = byte_count(td.type, td.size, td.mipmap_count, td.array_size, td.format);
	return (byte_count(td.buffer) == expected_bc);
}

texture_data_new load_from_image_file(const char* p_filename, uint8_t channel_count, bool flip_vertically)
{	
	struct stb_image final {
		stb_image() noexcept = default;
		~stb_image() noexcept { stbi_image_free(p_data); p_data = nullptr;  }

		uint8_t* p_data = nullptr;
	};

	// load stb image from the file
	stbi_set_flip_vertically_on_load(flip_vertically);

	stb_image img;
	int width = 0;
	int height = 0;
	int actual_channel_count = 0;
	const bool is_hdr = stbi_is_hdr(p_filename);

	if (is_hdr)
		img.p_data = reinterpret_cast<uint8_t*>(stbi_loadf(p_filename, &width, &height, &actual_channel_count, channel_count));
	else
		img.p_data = stbi_load(p_filename, &width, &height, &actual_channel_count, channel_count);

	if (!img.p_data) {
		const char* p_stb_error = stbi_failure_reason();
		throw std::runtime_error(EXCEPTION_MSG("Loading ", p_filename, " image error. ", p_stb_error));
	}

	// determine the pixel format
	pixel_format format = pixel_format::none;
	const uint8_t cc = (channel_count) ? channel_count : uint8_t(actual_channel_count);
	switch (cc) {
		case 1: format = pixel_format::red_8; break;
		case 2: format = pixel_format::rg_8; break;
		case 3: format = (is_hdr) ? (pixel_format::rgb_32f) : (pixel_format::rgb_8); break;
		case 4: format = (is_hdr) ? (pixel_format::rgba_32f) : (pixel_format::rgba_8); break;
	}

	// create texture data object and fill it with image's contents.
	texture_data_new td(texture_type::texture_2d, math::uint3(width, height, 1), 1, 1, format);
	std::memcpy(td.buffer.data(), img.p_data, byte_count(td.buffer));

	return td;
}

texture_data_new load_from_tex_file(const char* p_filename)
{
	assert(p_filename);

	try {
		std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(p_filename, "rb"), &std::fclose);
		ENFORCE(file, "Failed to open file ", p_filename);

		// header
		texture_type	type = texture_type::unknown;
		math::uint3		size;
		uint32_t		mipmap_count = 0;
		uint32_t		array_size = 0;
		pixel_format	format = pixel_format::none;

		std::fread(&type, sizeof(texture_type), 1, file.get());
		std::fread(&size.x, sizeof(math::uint3), 1, file.get());
		std::fread(&mipmap_count, sizeof(uint32_t), 1, file.get());
		std::fread(&array_size, sizeof(uint32_t), 1, file.get());
		std::fread(&format, sizeof(pixel_format), 1, file.get());
		// texture data
		texture_data_new td(type, size, mipmap_count, array_size, format);
		std::fread(td.buffer.data(), byte_count(td.buffer), 1, file.get());

		return td;
	} 
	catch (...) {
		std::string exc_msg = EXCEPTION_MSG("Load .tex data error. File: ", p_filename);
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

void save_to_tex_file(const char* p_filename, const texture_data_new& td)
{
	assert(p_filename);
	assert(is_valid_texture_data(td));

	try {
		std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(p_filename, "wb"), &std::fclose);
		ENFORCE(file, "Failed to create/open the file ", p_filename);

		// header
		std::fwrite(&td.type, sizeof(texture_type), 1, file.get());
		std::fwrite(&td.size.x, sizeof(math::uint3), 1, file.get());
		std::fwrite(&td.mipmap_count, sizeof(uint32_t), 1, file.get());
		std::fwrite(&td.array_size, sizeof(uint32_t), 1, file.get());
		std::fwrite(&td.format, sizeof(pixel_format), 1, file.get());
		// texture data
		std::fwrite(td.buffer.data(), byte_count(td.buffer), 1, file.get());
	}
	catch (...) {
		std::string exc_msg = EXCEPTION_MSG("Save texture file error. File: ", p_filename);
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

} // namespace sparki

#pragma warning(pop)
