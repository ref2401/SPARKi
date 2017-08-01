#pragma once

#include <vector>
#include "math/math.h"


namespace sparki {

// All possible combinations of vertex attributes.
// p - position;
// n - normal;
// uv - texture coordinates in the range [0, 1];
// ts - tangent space (tangnet & handedness);
enum class vertex_attribs : unsigned char {
	p = 0,
	p_n_uv,
	p_n_uv_ts
};

// Desribes vertex attribs order, byte offset etc. 
// This information is used during the input layout creation.
template<vertex_attribs attribs>
struct vertex_interleaved_format;

template<> struct vertex_interleaved_format<vertex_attribs::p_n_uv_ts> final {
	static constexpr vertex_attribs attribs = vertex_attribs::p_n_uv_ts;
	static constexpr size_t	attrib_count	= 4;

	static constexpr size_t position_component_count		= 3;
	static constexpr size_t position_byte_count				= sizeof(float) * position_component_count;
	static constexpr size_t position_byte_offset			= 0;

	static constexpr size_t normal_component_count			= 3;
	static constexpr size_t normal_byte_count				= sizeof(float) * normal_component_count;
	static constexpr size_t normal_byte_offset				= position_byte_offset + position_byte_count;

	static constexpr size_t uv_component_count				= 2;
	static constexpr size_t uv_byte_count					= sizeof(float) * uv_component_count;
	static constexpr size_t uv_byte_offset					= normal_byte_offset + normal_byte_count;

	static constexpr size_t tangent_space_component_count	= 1;
	static constexpr size_t tangent_space_byte_count		= sizeof(uint32_t) * tangent_space_component_count;
	static constexpr size_t tangent_space_byte_offset		= uv_byte_offset + uv_byte_count;

	static constexpr size_t vertex_component_count =
		position_component_count
		+ normal_component_count
		+ uv_component_count
		+ tangent_space_component_count;

	static constexpr size_t vertex_byte_count =
		position_byte_count
		+ normal_byte_count
		+ uv_byte_count
		+ tangent_space_byte_count;
};

// Represents a vertex with the specified set of attributes.
template<vertex_attribs attribs>
struct vertex;

template<> struct vertex<vertex_attribs::p_n_uv_ts> final {

	static constexpr vertex_attribs attribs = vertex_attribs::p_n_uv_ts;


	vertex() noexcept = default;

	vertex(const math::float3& position, const math::float3& normal,
		const math::float2& uv, uint32_t tangent_h) noexcept
		: position(position),
		normal(normal),
		uv(uv),
		tangent_h(tangent_h)
	{}


	math::float3 position;
	math::float3 normal;
	math::float2 uv;
	uint32_t tangent_h;
};

//
template<vertex_attribs attribs>
struct mesh_geometry final {

	using format	= vertex_interleaved_format<attribs>;
	using vertex_t	= vertex<attribs>;


	mesh_geometry() noexcept = default;

	mesh_geometry(size_t vertex_count, size_t index_count)
	{
		assert(vertex_count > 0);
		assert(index_count > 0);

		vertices.resize(vertex_count);
		indices.resize(index_count);
	}


	std::vector<vertex_t> vertices;
	std::vector<uint32_t> indices;
};


void convert_fbx_to_geo(const char* p_fbx_filename, const char* p_desc_filename);

// Reads mesh geometry from the specified .fbx file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_fbx(const char* p_filename);

// Reads mesh geometry from the specified .geo file.
mesh_geometry<vertex_attribs::p_n_uv_ts> read_geo(const char* p_filename);

// Writes mesh geometry in the specified .geo file.
void write_geo(const char* p_filename, const mesh_geometry<vertex_attribs::p_n_uv_ts>& mesh);

} // namespace sparki

