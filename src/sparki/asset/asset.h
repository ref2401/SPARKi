#pragma once

#include "sparki/utility.h"
#include "sparki/asset/model.h"


// NOTE(ref2401): .geo is SPARKi internal geometry format.

namespace sparki {

// hlsl_shader_desc struct stores all required and optional params
// which are used in hlsl shader creation process.
// 
struct hlsl_shader_desc final {
	// Hlsl source code.
	std::string source_code;

	// Hlsl source code filename. Use in error messages.
	std::string source_filename;

	// The D3DCOMPILE constants specify how the compiler compiles the HLSL code.
	uint32_t compile_flags = 0;

	// True indicates that there are hull & domain shaders.
	bool tesselation_stage = false;
};


void convert_fbx_to_geo(const char* p_fbx_filename, const char* p_desc_filename);

// Reads mesh geometry from the specified .fbx file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_fbx(const char* filename);

// Reads mesh geometry from the specified .geo file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_geo(const char* filename);

// Reads hlsl shader source code from the specified .hlsl file.
std::string read_hlsl(const char* filename);

// Writes mesh geometry in the specified .geo file.
void write_geo(const char* filename, const mesh_geometry<vertex_attribs::p_n_uv_ts>& mesh);

} // namespace sparki
