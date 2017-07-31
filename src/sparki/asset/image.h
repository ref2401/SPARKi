#pragma once

#include <vector>
#include "math/math.h"


namespace sparki {

// Describes pixel's channels and their size. 
enum class pixel_format :unsigned char {
	none = 0,

	rg_16f,
	rgba_16f,

	rg_32f,
	rgb_32f,
	rgba_32f,

	red_8,
	rg_8,
	rgb_8,
	rgba_8
};

class image_2d final {
public:

	image_2d() noexcept = default;

	image_2d(const char* p_filename, uint8_t channel_count = 0, bool flip_vertically = false);

	image_2d(image_2d&& image) noexcept;
	image_2d& operator=(image_2d&& image) noexcept;

	~image_2d() noexcept;



	void dispose() noexcept;

	// Pointer to the underlying data serving as pixel storage.
	const void* p_data() const noexcept
	{
		return p_data_;
	}

	// ditto
	void* p_data() noexcept
	{
		return p_data_;
	}

	// Pixel format of this image.
	pixel_format pixel_format() const noexcept
	{
		return pixel_format_;
	}

	// Size of the image in pixels.
	math::uint2 size() const noexcept
	{
		return size_;
	}

	// Width of the image in pixels.
	math::uint2::component_type width() const noexcept
	{
		return size_.x;
	}

	// Height of the image in pixels.
	math::uint2::component_type height() const noexcept
	{
		return size_.y;
	}

private:

	void*					p_data_ = nullptr;
	math::uint2				size_;
	sparki::pixel_format	pixel_format_ = pixel_format::none;
};

struct texture_data final {

	texture_data() noexcept = default;

	texture_data(const math::uint3& size, uint32_t mipmap_level_count, pixel_format format);


	math::uint3				size;
	uint32_t				mipmap_level_count = 0;
	pixel_format			format = pixel_format::none;
	std::vector<uint8_t>	buffer;
};


// Returns the number of bytes occupied by one pixel of the specified format.
size_t byte_count(pixel_format fmt) noexcept;

// Returns the number of bytes occupied by a texture of the specified size, format
// and with the specified number of mipmap levels.
size_t byte_count(const math::uint3& size, uint32_t mipmap_level_count, pixel_format fmt) noexcept;

bool is_valid_texture_data(const texture_data& td) noexcept;

// Reads texture data from the specified .tex file.
texture_data read_tex(const char* p_filename);

// Writes texture into the specified .tex file.
void write_tex(const char* p_filename, const texture_data& td);

} // namespace sparki

