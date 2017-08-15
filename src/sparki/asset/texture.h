#pragma once

#include <vector>
#include "math/math.h"


namespace sparki {

// Describes pixel's channels and their size. 
enum class pixel_format : unsigned char {
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

enum class texture_type : unsigned char {
	unknown = 0,
	texture_2d,
	texture_cube
};

struct texture_data final {
	texture_data() noexcept = default;

	texture_data(texture_type type, const math::uint3& size, uint32_t mipmap_count,
		uint32_t array_size, pixel_format fmt);

	texture_type 			type = texture_type::unknown;
	math::uint3				size;
	uint32_t				mipmap_count = 0;
	uint32_t				array_size = 0;
	pixel_format			format = pixel_format::none;
	std::vector<uint8_t> 	buffer;
};


// Returns the number of bytes occupied by one pixel of the specified format.
size_t byte_count(pixel_format fmt) noexcept;

// Returns the number of bytes occupied by a texture of the specified type, size, format
// and with the specified number of mipmap levels.
size_t byte_count(texture_type type, const math::uint3& size, uint32_t mipmap_count, 
	uint32_t array_size, pixel_format fmt) noexcept;

bool is_valid_texture_data(const texture_data& td) noexcept;

// Reads texture data from the specified file.
// The file may be .jpg, .png, .hdr
texture_data load_from_image_file(const char* p_filename, uint8_t channel_count, 
	bool flip_vertically = false);

// Reads texture data from the specified file.
// The file may be .tex
texture_data load_from_tex_file(const char* p_filename);

// Writes texture into the specified .tex file.
void save_to_tex_file(const char* p_filename, const texture_data& td);

} // namespace sparki

