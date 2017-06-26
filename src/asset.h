#pragma once

#include "model.h"
#include "utility.h"


// NOTE(ref2401): .geo is SPARKi internal geometry format.

namespace sparki {

void convert_fbx_to_geo(const char* p_fbx_filename, const char* p_desc_filename);

// Read mesh geometry from the specified .fbx file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_fbx(const char* filename);

// Read mesh geometry from the specified .geo file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_geo(const char* filename);

// Writes mesh geometry in the specified .geo file.
void write_geo(const char* filename, const mesh_geometry<vertex_attribs::p_n_uv_ts>& mesh);

} // namespace sparki
