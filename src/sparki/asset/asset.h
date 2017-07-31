#pragma once

#include "sparki/utility.h"
#include "sparki/asset/geometry.h"
#include "sparki/asset/image.h"


namespace sparki {

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


// Reads hlsl shader source code from the specified .hlsl file.
std::string read_text(const char* p_filename);


} // namespace sparki
