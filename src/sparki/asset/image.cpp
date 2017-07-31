#include "sparki/asset/image.h"

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

// ----- image_2d -----

image_2d::image_2d(const char* p_filename, uint8_t channel_count, bool flip_vertically)
{
	assert(p_filename);

	stbi_set_flip_vertically_on_load(flip_vertically);

	int width = 0;
	int height = 0;
	int actual_channel_count = 0;

	const bool is_hdr = stbi_is_hdr(p_filename);
	if (is_hdr)
		p_data_ = stbi_loadf(p_filename, &width, &height, &actual_channel_count, channel_count);
	else
		p_data_ = stbi_load(p_filename, &width, &height, &actual_channel_count, channel_count);

	if (!p_data_) {
		const char* p_stb_error = stbi_failure_reason();
		throw std::runtime_error(EXCEPTION_MSG("Loading ", p_filename, " image error. ", p_stb_error));
	}

	size_.x = width;
	size_.y = height;
	uint8_t cc = (channel_count) ? channel_count : uint8_t(actual_channel_count);
	switch (cc) {
		case 1: pixel_format_ = pixel_format::red_8; break;
		case 2: pixel_format_ = pixel_format::rg_8; break;
		case 3: pixel_format_ = (is_hdr) ? (pixel_format::rgb_32f) : (pixel_format::rgb_8); break;
		case 4: pixel_format_ = (is_hdr) ? (pixel_format::rgba_32f) : (pixel_format::rgba_8); break;
	}
}

image_2d::image_2d(image_2d&& image) noexcept
	: p_data_(image.p_data_),
	size_(image.size_),
	pixel_format_(image.pixel_format_)
{
	image.p_data_ = nullptr;
	image.size_ = math::uint2::zero;
	image.pixel_format_ = pixel_format::none;
}

image_2d::~image_2d() noexcept
{
	dispose();
}

image_2d& image_2d::operator=(image_2d&& image) noexcept
{
	if (this == &image) return *this;

	dispose();

	p_data_ = image.p_data_;
	size_ = image.size_;
	pixel_format_ = image.pixel_format_;

	image.p_data_ = nullptr;
	image.size_ = math::uint2::zero;
	image.pixel_format_ = pixel_format::none;

	return *this;
}

void image_2d::dispose() noexcept
{
	if (!p_data_) return;

	stbi_image_free(p_data_);
	p_data_ = nullptr;
	size_ = math::uint2::zero;
	pixel_format_ = pixel_format::none;
}

// ----- texture_data -----

texture_data::texture_data(const math::uint3& size, uint32_t mipmap_level_count, pixel_format format)
	: size(size), mipmap_level_count(mipmap_level_count), format(format)
{
	assert(size > 0);
	assert(mipmap_level_count > 0);
	assert(format != pixel_format::none);

	const size_t c = byte_count(size, mipmap_level_count, format);
	buffer.resize(c);
}


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

size_t byte_count(const math::uint3& size, uint32_t mipmap_level_count, pixel_format fmt) noexcept
{
	assert(size > 0);
	assert(mipmap_level_count > 0);
	assert(fmt != pixel_format::none);

	const size_t fmt_bytes = byte_count(fmt);

	size_t array_slice_bytes = 0;
	for (uint32_t i = 0; i < mipmap_level_count; ++i) {
		const uint32_t wo = size.x >> i;
		const uint32_t ho = size.y >> i;
		assert((wo > 0) || (ho > 0)); // ensure size and mipmap_level_count compatibility

		array_slice_bytes += std::max(1u, wo) * std::max(1u, ho) * fmt_bytes;
	}

	return array_slice_bytes * size.z;
}

bool is_valid_texture_data(const texture_data& td) noexcept
{
	bool res = (td.size > 0)
		&& (td.mipmap_level_count > 0)
		&& (td.format != pixel_format::none);

	if (!res) return false;

	// check size and mipmap_level_count compatibility
	for (uint32_t i = 0; i < td.mipmap_level_count; ++i) {
		const uint32_t w = td.size.x >> i;
		const uint32_t h = td.size.y >> i;

		if (w == 0 && h == 0) return false;
	}

	// check buffers memory is large enough
	return (byte_count(td.buffer) == byte_count(td.size, td.mipmap_level_count, td.format));
}

texture_data read_tex(const char* p_filename)
{
	assert(p_filename);

	try {
		std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(p_filename, "rb"), &std::fclose);
		ENFORCE(file, "Failed to open file ", p_filename);

		// header
		math::uint3 size;
		uint32_t mipmap_count;
		pixel_format format = pixel_format::none;
		std::fread(&size.x, sizeof(math::uint3), 1, file.get());
		std::fread(&mipmap_count, sizeof(uint32_t), 1, file.get());
		std::fread(&format, sizeof(pixel_format), 1, file.get());
		// texture data
		texture_data td(size, mipmap_count, format);
		std::fread(td.buffer.data(), byte_count(td.buffer), 1, file.get());

		return td;
	}
	catch (...) {
		std::string exc_msg = EXCEPTION_MSG("Load texture data error. File: ", p_filename);
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

void write_tex(const char* p_filename, const texture_data& td)
{
	assert(p_filename);
	assert(is_valid_texture_data(td));

	try {
		std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(p_filename, "wb"), &std::fclose);
		ENFORCE(file, "Failed to create/open the file ", p_filename);

		// header
		std::fwrite(&td.size.x, sizeof(math::uint3), 1, file.get());
		std::fwrite(&td.mipmap_level_count, sizeof(uint32_t), 1, file.get());
		std::fwrite(&td.format, sizeof(pixel_format), 1, file.get());
		// texture data
		std::fwrite(td.buffer.data(), byte_count(td.buffer), 1, file.get());
	}
	catch (...) {
		std::string exc_msg = EXCEPTION_MSG("Write texture file error. File: ", p_filename);
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

} // namespace sparki

#pragma warning(pop)
