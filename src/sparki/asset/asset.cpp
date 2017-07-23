#include "sparki/asset/asset.h"

#include <cassert>
#include <cstdio>
#include <memory>
#include "fbxsdk.h"
#pragma warning(push)
#pragma warning(disable:4244) // C4244 '=': conversion from 'int' to 'stbi__uint16', possible loss of data.
#pragma warning(disable:4456) // C4456 '=': declaration of 'k' hides previous local declaration.
	#define STB_IMAGE_IMPLEMENTATION
	#define STBI_FAILURE_USERMSG 
	#include "stb/stb_image.h"
#pragma warning(pop)


namespace {

using namespace sparki;

template<typename T>
struct fbx_deleter final {
	void operator()(T* obj) const
	{
		obj->Destroy();
	}
};

template<typename T>
using fbx_ptr = std::unique_ptr<T, fbx_deleter<T>>;


inline math::float2 make_float2(const FbxVector2& v) noexcept
{
	return math::float2(float(v[0]), float(v[1]));
}

inline math::float3 make_float3(const FbxVector4& v) noexcept
{
	return math::float3(float(v[0]), float(v[1]), float(v[2]));
}

inline math::float4 make_float4(const FbxVector4& v) noexcept
{
	return math::float4(float(v[0]), float(v[1]), float(v[2]), float(v[3]));
}

fbx_ptr<FbxScene> read_fbx_scene(FbxManager* manager, const char* filename)
{
	assert(manager);
	assert(filename);

	fbx_ptr<FbxIOSettings> io_settings(FbxIOSettings::Create(manager, IOSROOT));
	manager->SetIOSettings(io_settings.get());

	fbx_ptr<FbxImporter> importer(FbxImporter::Create(manager, ""));
	bool r = importer->Initialize(filename, -1, manager->GetIOSettings());
	ENFORCE(r, "Importer initialization failure. ", importer->GetStatus().GetErrorString());

	fbx_ptr<FbxScene> scene(FbxScene::Create(manager, ""));
	importer->Import(scene.get());

	return scene;
}

void setup_triangle(FbxMesh* fbx_mesh, int polygon_index, int first_vertex_index,
	const char* fbx_uv_set_name, FbxLayerElementArrayTemplate<FbxVector4>& ts_direct_array,
	vertex<vertex_attribs::p_n_uv_ts>& v0,
	vertex<vertex_attribs::p_n_uv_ts>& v1,
	vertex<vertex_attribs::p_n_uv_ts>& v2) noexcept
{
	// normals
	FbxVector4 n0, n1, n2;
	const bool res_n0 = fbx_mesh->GetPolygonVertexNormal(polygon_index, 0, n0);
	const bool res_n1 = fbx_mesh->GetPolygonVertexNormal(polygon_index, 1, n1);
	const bool res_n2 = fbx_mesh->GetPolygonVertexNormal(polygon_index, 2, n2);
	assert(res_n0);
	assert(res_n1);
	assert(res_n2);
	v0.normal = math::normalize(make_float3(n0));
	v1.normal = math::normalize(make_float3(n1));
	v2.normal = math::normalize(make_float3(n2));

	// uv
	FbxVector2 uv0, uv1, uv2;
	bool uv0_unmapped, uv1_unmapped, uv2_unmapped;
	const bool res_uv0 = fbx_mesh->GetPolygonVertexUV(polygon_index, 0, fbx_uv_set_name, uv0, uv0_unmapped);
	const bool res_uv1 = fbx_mesh->GetPolygonVertexUV(polygon_index, 1, fbx_uv_set_name, uv1, uv1_unmapped);
	const bool res_uv2 = fbx_mesh->GetPolygonVertexUV(polygon_index, 2, fbx_uv_set_name, uv2, uv2_unmapped);
	assert(res_uv0);
	assert(res_uv1);
	assert(res_uv2);
	v0.uv = make_float2(uv0);
	v1.uv = make_float2(uv1);
	v2.uv = make_float2(uv2);

	// tangent space
	v0.tangent_h = math::pack_snorm_10_10_10_2(make_float4(ts_direct_array[first_vertex_index]));
	v1.tangent_h = math::pack_snorm_10_10_10_2(make_float4(ts_direct_array[first_vertex_index + 1]));
	v2.tangent_h = math::pack_snorm_10_10_10_2(make_float4(ts_direct_array[first_vertex_index + 2]));
}

} // namespace


namespace sparki {

// ----- hlsl_compute_desc -----

hlsl_compute_desc::hlsl_compute_desc(const char* p_filename)
	: source_code(read_hlsl(p_filename)),
	source_filename(p_filename)
{}

// ----- hlsl_shader_desc -----

hlsl_shader_desc::hlsl_shader_desc(const char* p_filename, bool tesselation_stage)
	: source_code(read_hlsl(p_filename)),
	source_filename(p_filename),
	tesselation_stage(tesselation_stage)
{}

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

// ----- funcs -----

size_t byte_count(const pixel_format& fmt) noexcept
{
	switch (fmt) {
		default:
		case pixel_format::none:		return 0;
		case pixel_format::rgb_32f:		return 3 * sizeof(float);
		case pixel_format::rgba_32f:	return 4 * sizeof(float);
		case pixel_format::red_8:		return 1;
		case pixel_format::rg_8:		return 2;
		case pixel_format::rgb_8:		return 3;
		case pixel_format::rgba_8:		return 4;
	}
}

void convert_fbx_to_geo(const char* p_fbx_filename, const char* p_desc_filename)
{
	assert(p_fbx_filename);
	assert(p_desc_filename);

	try {
		auto mesh_geometry = read_fbx(p_fbx_filename);
		write_geo(p_desc_filename, mesh_geometry);
	}
	catch (...) {
		std::throw_with_nested(std::runtime_error("Convert fbx to geo error."));
	}
}

mesh_geometry<vertex_attribs::p_n_uv_ts> read_fbx(const char* filename)
{
	using mesh_geometry_t = mesh_geometry<vertex_attribs::p_n_uv_ts>;

	assert(filename);

	try {
		fbx_ptr<FbxManager> fbx_manager(FbxManager::Create());
		fbx_ptr<FbxScene> scene = read_fbx_scene(fbx_manager.get(), filename);
		assert(scene);

		FbxNode* root = scene->GetRootNode();
		ENFORCE(root && root->GetChildCount() == 1, "Fbx scene must contain only one object.");
		FbxMesh* fbx_mesh = root->GetChild(0)->GetMesh();
		ENFORCE(fbx_mesh && fbx_mesh->IsTriangleMesh(), "Fbx mesh must be triangulated.");

		// position stuff
		FbxVector4* positions = fbx_mesh->GetControlPoints();
		int* position_indices = fbx_mesh->GetPolygonVertices();
		// fbx_uv_set_name
		FbxLayer* uv_layer_obj = fbx_mesh->GetLayer(0, FbxLayerElement::EType::eUV);
		ENFORCE(uv_layer_obj, "Mesh does not contain uv vertex attribute.");
		FbxLayerElementUV* uv_layer = uv_layer_obj->GetUVs();
		assert(uv_layer);
		const char* fbx_uv_set_name = uv_layer->GetName();
		// tangent space direct array
		const bool res_gen_ts = fbx_mesh->GenerateTangentsData(fbx_uv_set_name);
		assert(res_gen_ts);
		FbxLayer* ts_layer_obj = fbx_mesh->GetLayer(0, FbxLayerElement::EType::eTangent);
		assert(ts_layer_obj);
		FbxLayerElementTangent* ts_layer = uv_layer_obj->GetTangents();
		assert(ts_layer);
		assert(ts_layer->GetReferenceMode() == FbxLayerElement::EReferenceMode::eDirect);
		FbxLayerElementArrayTemplate<FbxVector4>& ts_direct_array = ts_layer->GetDirectArray();

		mesh_geometry_t mesh;
		mesh.vertices.reserve(fbx_mesh->GetPolygonCount() * 3);
		mesh.indices.reserve(fbx_mesh->GetPolygonCount() * 3);
		for (int pi = 0; pi < fbx_mesh->GetPolygonCount(); ++pi) {
			assert(fbx_mesh->GetPolygonSize(pi) == 3);

			const int first_vertex_index = pi * 3;
			mesh_geometry_t::vertex_t v0;
			mesh_geometry_t::vertex_t v1;
			mesh_geometry_t::vertex_t v2;

			const int poly_start_index = fbx_mesh->GetPolygonVertexIndex(pi);
			v0.position = make_float3(positions[position_indices[poly_start_index]]);
			v1.position = make_float3(positions[position_indices[poly_start_index + 1]]);
			v2.position = make_float3(positions[position_indices[poly_start_index + 2]]);

			setup_triangle(fbx_mesh, pi, first_vertex_index, fbx_uv_set_name, ts_direct_array, v0, v1, v2);
			mesh.vertices.push_back(v0);
			mesh.vertices.push_back(v1);
			mesh.vertices.push_back(v2);
			mesh.indices.push_back(first_vertex_index + 0);
			mesh.indices.push_back(first_vertex_index + 1);
			mesh.indices.push_back(first_vertex_index + 2);
		}

		return mesh;
	}
	catch (...) {
		std::string exc_msg = EXCEPTION_MSG("Fbx reading error. ", filename);
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

#pragma warning(push)
#pragma warning(disable:4996) // C4996 'fopen': This function or variable may be unsafe.

mesh_geometry<vertex_attribs::p_n_uv_ts> read_geo(const char* filename)
{
	assert(filename);

	try {
		std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(filename, "rb"), &std::fclose);
		ENFORCE(file, "Failed to open file ", filename);

		// header
		uint64_t vertex_count = 0;
		uint64_t index_count = 0;
		std::fread(&vertex_count, sizeof(uint64_t), 1, file.get());
		std::fread(&index_count, sizeof(uint64_t), 1, file.get());

		mesh_geometry<vertex_attribs::p_n_uv_ts> mesh(vertex_count, index_count);
		std::fread(mesh.vertices.data(), byte_count(mesh.vertices), 1, file.get());
		std::fread(mesh.indices.data(), byte_count(mesh.indices), 1, file.get());

		return mesh;
	}
	catch (...) {
		std::string exc_msg = EXCEPTION_MSG("Load model geometry error. File: ", filename);
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

std::string read_hlsl(const char* p_filename)
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

void write_geo(const char* p_filename, const mesh_geometry<vertex_attribs::p_n_uv_ts>& mesh)
{
	assert(p_filename);
	assert((mesh.vertices.size() > 0) && (mesh.indices.size() > 0));

	try {
		std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(p_filename, "wb"), &std::fclose);
		ENFORCE(file, "Failed to create/open the file ", p_filename);

		// header
		// rex/geometry.h model_geometry_file_header
		uint64_t vertex_count = uint64_t(mesh.vertices.size());
		uint64_t index_count = uint64_t(mesh.indices.size());
		std::fwrite(&vertex_count, sizeof(uint64_t), 1, file.get());
		std::fwrite(&index_count, sizeof(uint64_t), 1, file.get());
		// vertices
		std::fwrite(mesh.vertices.data(),byte_count(mesh.vertices), 1, file.get());
		// indices
		std::fwrite(mesh.indices.data(), byte_count(mesh.indices), 1, file.get());
	}
	catch (...) {
		std::string exc_msg = EXCEPTION_MSG("Write geometry file error. File: ", p_filename);
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

#pragma warning(pop)

} // namespace sparki
