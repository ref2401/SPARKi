#pragma once

#include "sparki/utility.h"
#include "sparki/asset/model.h"


// NOTE(ref2401): .geo is SPARKi internal geometry format.

namespace sparki {

// Describes pixel's channels and their size. 
enum class pixel_format :unsigned char {
	none = 0,

	rgb_32f,
	rgba_32f,

	red_8,
	rg_8,
	rgb_8,
	rgba_8
};


struct hlsl_compute_desc final {

	static constexpr char* compute_shader_entry_point = "cs_main";
	static constexpr char* compute_shader_model = "cs_5_0";

	hlsl_compute_desc() = default;

	explicit hlsl_compute_desc(const char* p_filename);


	std::string source_code;
	std::string source_filename;
	// The D3DCOMPILE constants specify how the compiler compiles the HLSL code.
	uint32_t compile_flags = 0;
};

// hlsl_shader_desc struct stores all required and optional params
// which are used in hlsl shader creation process.
struct hlsl_shader_desc final {

	static constexpr char* vertex_shader_entry_point = "vs_main";
	static constexpr char* vertex_shader_model = "vs_5_0";
	static constexpr char* hull_shader_entry_point = "hs_main";
	static constexpr char* hull_shader_model = "hs_5_0";
	static constexpr char* domain_shader_entry_point = "ds_main";
	static constexpr char* domain_shader_model = "ds_5_0";
	static constexpr char* pixel_shader_entry_point = "ps_main";
	static constexpr char* pixel_shader_model = "ps_5_0";


	hlsl_shader_desc() = default;

	explicit hlsl_shader_desc(const char* filename, bool tesselation_stage = false);


	std::string source_code;
	std::string source_filename;
	// The D3DCOMPILE constants specify how the compiler compiles the HLSL code.
	uint32_t compile_flags = 0;
	// True indicates that there are hull & domain shaders.
	bool tesselation_stage = false;
};

struct image_2d final {
	image_2d() noexcept = default;

	image_2d(const char* p_filename, uint8_t channel_count = 0, bool flip_vertically = false);

	image_2d(image_2d&& image) noexcept;
	image_2d& operator=(image_2d&& image) noexcept;

	~image_2d() noexcept;


	void dispose() noexcept;

	// Pointer to the underlying buffer serving as pixel storage.
	void* p_data = nullptr;
	// Size of the image in pixels.
	math::uint2 size;
	// Pixel format of this image.
	pixel_format pixel_format = pixel_format::none;
};


// Returns the number of bytes occupied by one pixel of the specified format.
size_t byte_count(const pixel_format& fmt) noexcept;

void convert_fbx_to_geo(const char* p_fbx_filename, const char* p_desc_filename);

// Reads mesh geometry from the specified .fbx file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_fbx(const char* p_filename);

// Reads mesh geometry from the specified .geo file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_geo(const char* p_filename);

// Reads hlsl shader source code from the specified .hlsl file.
std::string read_hlsl(const char* p_filename);

// Writes mesh geometry in the specified .geo file.
void write_geo(const char* p_filename, const mesh_geometry<vertex_attribs::p_n_uv_ts>& mesh);

} // namespace sparki
