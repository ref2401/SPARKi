#include "sparki/core/rnd_tool.h"

#include <cassert>
#include "ts/task_system.h"


namespace sparki {
namespace core {

// ----- brdf_integrator -----

brdf_integrator::brdf_integrator(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug);

	hlsl_compute_desc compute_desc("../../data/shaders/specular_brdf_integrator.compute.hlsl");
	specular_brdf_compute_ = hlsl_compute(p_device, compute_desc);
}

void brdf_integrator::perform(const char* p_specular_brdf_filename)
{
	assert(p_specular_brdf_filename);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = brdf_integrator::brdf_side_size;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16G16_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	
	com_ptr<ID3D11Texture2D> p_tex;
	HRESULT hr = p_device_->CreateTexture2D(&desc, nullptr, &p_tex.ptr);
	assert(hr == S_OK);
	com_ptr<ID3D11UnorderedAccessView> p_tex_uav;
	hr = p_device_->CreateUnorderedAccessView(p_tex, nullptr, &p_tex_uav.ptr);
	assert(hr == S_OK);

	p_ctx_->CSSetShader(specular_brdf_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_tex_uav.ptr, nullptr);

#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif

	const UINT gx = brdf_integrator::brdf_side_size / brdf_integrator::compute_group_x_size;
	const UINT gy = brdf_integrator::brdf_side_size / brdf_integrator::compute_group_y_size;
	p_ctx_->Dispatch(gx, gy, 1);

	// reset uav binding
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	// write brdf lut to a file
	{
		const texture_data td = make_texture_data(p_device_, p_ctx_, texture_type::texture_2d, p_tex);
		save_to_tex_file(p_specular_brdf_filename, td);
	}
}

// ----- envmap_texture_builder -----

envmap_texture_builder::envmap_texture_builder(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, 
	ID3D11Debug* p_debug, ID3D11SamplerState* p_sampler)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug), p_sampler_(p_sampler)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.
	assert(p_sampler);

	const hlsl_compute_desc hlsl_equirect_to_skybox("../../data/shaders/equirect_to_skybox.compute.hlsl");
	equirect_to_skybox_compute_ = hlsl_compute(p_device, hlsl_equirect_to_skybox);
	
	const hlsl_compute_desc hlsl_diffuse_envmap("../../data/shaders/diffuse_envmap.compute.hlsl");
	diffuse_envmap_compute_ = hlsl_compute(p_device, hlsl_diffuse_envmap);

	const hlsl_compute_desc hlsl_specular_envmap("../../data/shaders/specular_envmap.compute.hlsl");
	specular_envmap_compute_ = hlsl_compute(p_device, hlsl_specular_envmap);

	p_cb_prefilter_envmap_ = make_constant_buffer(p_device, sizeof(float4));
}

com_ptr<ID3D11Texture2D> envmap_texture_builder::make_skybox(const char* p_hdr_filename)
{
	// equirect texture
	const texture_data td = load_from_image_file(p_hdr_filename, 4);
	com_ptr<ID3D11Texture2D> p_tex_equirect = make_texture_2d(p_device_, td, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	com_ptr<ID3D11ShaderResourceView> p_tex_equirect_srv;
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_equirect, nullptr, &p_tex_equirect_srv.ptr);
	assert(hr == S_OK);

	// skybox texture & uav
	com_ptr<ID3D11Texture2D> p_tex_skybox = make_texture_cube(p_device_,
		c_skybox_side_size, c_skybox_mipmap_count,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_USAGE_DEFAULT, 
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS,
		D3D11_RESOURCE_MISC_GENERATE_MIPS);
	
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uav_desc.Texture2DArray.MipSlice = 0;
	uav_desc.Texture2DArray.FirstArraySlice = 0;
	uav_desc.Texture2DArray.ArraySize = 6;
	com_ptr<ID3D11UnorderedAccessView> p_tex_skybox_uav;
	hr = p_device_->CreateUnorderedAccessView(p_tex_skybox, &uav_desc, &p_tex_skybox_uav.ptr);
	assert(hr == S_OK);

	// setup compute pipeline & dispatch work
	p_ctx_->CSSetShader(equirect_to_skybox_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_equirect_srv.ptr);
	p_ctx_->CSSetSamplers(0, 1, &p_sampler_);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_tex_skybox_uav.ptr, nullptr);

#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif

	p_ctx_->Dispatch(c_skybox_compute_gx, c_skybox_compute_gy, 6);

	// reset uav binding
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	return p_tex_skybox;
}

com_ptr<ID3D11Texture2D> envmap_texture_builder::make_diffuse_envmap(ID3D11ShaderResourceView* p_tex_skybox_srv)
{
	assert(p_tex_skybox_srv);

	com_ptr<ID3D11Texture2D> p_tex_envmap = make_texture_cube(p_device_,
		c_diffuse_envmap_side_size, 1,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_UNORDERED_ACCESS);
	com_ptr<ID3D11UnorderedAccessView> p_tex_envmap_uav;
	HRESULT hr = p_device_->CreateUnorderedAccessView(p_tex_envmap, nullptr, &p_tex_envmap_uav.ptr);
	assert(hr == S_OK);

	// setup compute pipeline & dispatch work
	p_ctx_->CSSetShader(diffuse_envmap_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_skybox_srv);
	p_ctx_->CSSetSamplers(0, 1, &p_sampler_);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_tex_envmap_uav.ptr, nullptr);

#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif

	p_ctx_->Dispatch(c_diffuse_compute_gx, c_diffuse_compute_gy, 6);

	// reset uav binding
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	return p_tex_envmap;
}

com_ptr<ID3D11Texture2D> envmap_texture_builder::make_specular_envmap(ID3D11Texture2D* p_tex_skybox,
	ID3D11ShaderResourceView* p_tex_skybox_srv)
{
	assert(p_tex_skybox);
	assert(p_tex_skybox_srv);

	com_ptr<ID3D11Texture2D> p_tex_envmap = make_texture_cube(p_device_,
		c_specular_envmap_side_size, c_specular_envmap_mipmap_count,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_USAGE_DEFAULT, 
		D3D11_BIND_UNORDERED_ACCESS);

	// Copy mipmap #skybox_lvl from p_tex_skybox to the first mip level of the p_tex_envmap.
	const UINT skybox_lvl = UINT(std::log2(c_skybox_side_size / c_specular_envmap_side_size));
	const D3D11_BOX box = { 0, 0, 0, c_specular_envmap_side_size, c_specular_envmap_side_size, 1 };
	for (UINT a = 0; a < 6; ++a) {
		const UINT src_index = D3D11CalcSubresource(skybox_lvl, a, c_skybox_mipmap_count);
		const UINT dest_index = D3D11CalcSubresource(0, a, c_specular_envmap_mipmap_count);
		p_ctx_->CopySubresourceRegion(p_tex_envmap, dest_index, 0, 0, 0, p_tex_skybox, src_index, &box);
	}

	// setup compute pipeline & dispatch work
	p_ctx_->CSSetShader(specular_envmap_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetConstantBuffers(0, 1, &p_cb_prefilter_envmap_.ptr);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_skybox_srv);
	p_ctx_->CSSetSamplers(0, 1, &p_sampler_);

	// for each mipmap level
	for (UINT m = 1; m < c_specular_envmap_mipmap_count; ++m) {
		const float cb_data[4] = {
			/* roughness */ float(m) / (c_specular_envmap_mipmap_count - 1),
			/* side_size */ float(c_specular_envmap_side_size >> m),
			0.0f,
			0.0f
		};
		p_ctx_->UpdateSubresource(p_cb_prefilter_envmap_, 0, nullptr, cb_data, 0, 0);

		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = m;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.ArraySize = 6;
		com_ptr<ID3D11UnorderedAccessView> p_uav;
		HRESULT hr = p_device_->CreateUnorderedAccessView(p_tex_envmap, &desc, &p_uav.ptr);
		assert(hr == S_OK);
		p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_uav.ptr, nullptr);

#ifdef SPARKI_DEBUG
		hr = p_debug_->ValidateContextForDispatch(p_ctx_);
		assert(hr == S_OK);
#endif
		const UINT mipmap_size = c_specular_envmap_side_size >> m;
		const UINT gx = (mipmap_size) / c_specular_envmap_compute_group_x_size;
		const UINT gy = (mipmap_size) / c_specular_envmap_compute_group_y_size;
		p_ctx_->Dispatch(gx, gy, 6);
	}

	// reset uav binding
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	return p_tex_envmap;
}

void envmap_texture_builder::perform(const char* p_hdr_filename, const char* p_skybox_filename,
	const char* p_diffuse_envmap_filename, const char* p_specular_envmap_filename)
{
	assert(p_hdr_filename);
	assert(p_diffuse_envmap_filename);
	assert(p_specular_envmap_filename);

	// make skybox texture & save it to a file
	com_ptr<ID3D11Texture2D> p_tex_skybox = make_skybox(p_hdr_filename);
	save_skybox_to_file(p_skybox_filename, p_tex_skybox);

	// generate skybox mipmaps
	com_ptr<ID3D11ShaderResourceView> p_tex_skybox_srv;
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_skybox, nullptr, &p_tex_skybox_srv.ptr);
	assert(hr == S_OK);
	p_ctx_->GenerateMips(p_tex_skybox_srv);

	// make diffuse envmap and save it to a file
	{
		com_ptr<ID3D11Texture2D> p_tex_diffuse_envmap = make_diffuse_envmap(p_tex_skybox_srv);
		const texture_data td = make_texture_data(p_device_, p_ctx_,
			texture_type::texture_cube, p_tex_diffuse_envmap);
		save_to_tex_file(p_diffuse_envmap_filename, td);
	}
	
	// make specular envmap and save it to a file
	{
		com_ptr<ID3D11Texture2D> p_tex_specular_envmap = make_specular_envmap(p_tex_skybox, p_tex_skybox_srv);
		const texture_data td = make_texture_data(p_device_, p_ctx_, 
			texture_type::texture_cube, p_tex_specular_envmap);
		save_to_tex_file(p_specular_envmap_filename, td);
	}
}

void envmap_texture_builder::save_skybox_to_file(const char* p_filename, ID3D11Texture2D* p_tex_skybox)
{
	assert(p_filename);
	assert(p_tex_skybox);

	// create skybox texture (single mip level) and 
	D3D11_TEXTURE2D_DESC tmp_desc;
	p_tex_skybox->GetDesc(&tmp_desc);
	tmp_desc.MipLevels = 1;
	tmp_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	com_ptr<ID3D11Texture2D> p_tex_skybox_tmp;
	HRESULT hr = p_device_->CreateTexture2D(&tmp_desc, nullptr, &p_tex_skybox_tmp.ptr);
	assert(hr == S_OK);

	// copy mip #0 from &p_tex_skybox to p_tex_skybox_tmp
	const D3D11_BOX box = { 0, 0, 0, tmp_desc.Width, tmp_desc.Height, 1 };
	for (UINT a = 0; a < tmp_desc.ArraySize; ++a) {
		const UINT index = D3D11CalcSubresource(0, a, c_skybox_mipmap_count);
		p_ctx_->CopySubresourceRegion(p_tex_skybox_tmp, a, 0, 0, 0, p_tex_skybox, index, &box);
	}

	// save p_tex_skybox_tmp to a file
	const texture_data td = make_texture_data(p_device_, p_ctx_, 
		texture_type::texture_cube, p_tex_skybox_tmp);
	save_to_tex_file(p_filename, td);
}

// ----- material_composer -----

material_composer::material_composer(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	const hlsl_compute_desc hc("../../data/shaders/material_composer.compute.hlsl");
	compute_shader_ = hlsl_compute(p_device_, hc);
}

// ----- unique_color_miner -----

unique_color_miner::unique_color_miner(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	const hlsl_compute_desc hc("../../data/shaders/unique_color_miner.compute.hlsl");
	compute_shader_ = hlsl_compute(p_device_, hc);

	// p_hash_buffer_ & uav
	const UINT bc = sizeof(uint32_t) * unique_color_miner::c_hash_buffer_count;
	p_hash_buffer_ = make_buffer(p_device_, bc, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS);
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.Format = DXGI_FORMAT_R32_UINT;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = 0;
	uav_desc.Buffer.NumElements = c_hash_buffer_count;
	HRESULT hr = p_device_->CreateUnorderedAccessView(p_hash_buffer_, &uav_desc, &p_hash_buffer_uav_.ptr);
	assert(hr == S_OK);

	// p_color_buffer & uav
	p_color_buffer_ = make_structured_buffer(p_device_, sizeof(uint32_t), c_color_buffer_count,
		D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS);
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = 0;
	uav_desc.Buffer.NumElements = c_color_buffer_count;
	uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
	hr = p_device_->CreateUnorderedAccessView(p_color_buffer_, &uav_desc, &p_color_buffer_uav_.ptr);
	assert(hr == S_OK);

	// p_result_buffer_
	p_result_buffer_ = make_buffer(p_device_, c_result_buffer_byte_count,
		D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ);
}

void unique_color_miner::perform(ID3D11ShaderResourceView* p_tex_image_srv, const uint2& image_size,
	std::vector<uint32_t>& out_colors)
{
	assert(p_tex_image_srv);
	assert(image_size > 0);

	// clear buffers ---
	p_ctx_->ClearUnorderedAccessViewUint(p_hash_buffer_uav_, &uint4::zero.x);
	p_ctx_->ClearUnorderedAccessViewUint(p_color_buffer_uav_, &uint4::zero.x);
	out_colors.clear();

	// setup compute pipeline ---
	p_ctx_->CSSetShader(compute_shader_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_image_srv);
	constexpr UINT c_uav_count = 2;
	ID3D11UnorderedAccessView* uav_list[c_uav_count] = { p_hash_buffer_uav_, p_color_buffer_uav_ };
	UINT uav_init_counters[c_uav_count] = { 0, 0 };
	p_ctx_->CSSetUnorderedAccessViews(0, c_uav_count, uav_list, uav_init_counters);

	// dispatch work ---
#ifdef SPARKI_DEBUG
	HRESULT hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif

	const UINT gx = UINT(std::ceil(float(image_size.x) / c_compute_group_x_size));
	const UINT gy = UINT(std::ceil(float(image_size.y) / c_compute_group_y_size));
	p_ctx_->Dispatch(gx, gy, 1);

	// reset uav binding ---
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	uav_list[0] = nullptr;
	uav_list[1] = nullptr;
	p_ctx_->CSSetUnorderedAccessViews(0, c_uav_count, uav_list, nullptr);


	// copy results to staging buffer ---
	// p_result_buffer_[0] - color count
	// p_result_buffer_[1, color count] - unique colors
	p_ctx_->CopyStructureCount(p_result_buffer_, 0, p_color_buffer_uav_);
	const D3D11_BOX box = { 0, 0, 0, c_color_buffer_byte_count, 1, 1 };
	p_ctx_->CopySubresourceRegion(p_result_buffer_, 0, sizeof(uint32_t), 0, 0, p_color_buffer_, 0, &box);

	// map the staging buffer and copy data ---
	D3D11_MAPPED_SUBRESOURCE map;
	hr = p_ctx_->Map(p_result_buffer_, 0, D3D11_MAP_READ, 0, &map);
	assert(hr == S_OK);

	const uint32_t* p_data = reinterpret_cast<const uint32_t*>(map.pData);
	const uint32_t color_count = (*p_data <= c_color_buffer_count) ? (*p_data) : (c_color_buffer_count);
	++p_data; // now it points to the first color if any.
	if (color_count > 0) {
		out_colors.resize(color_count);
		std::memcpy(out_colors.data(), p_data, color_count * sizeof(uint32_t));
		std::sort(out_colors.begin(), out_colors.end());
	}

	p_ctx_->Unmap(p_result_buffer_, 0);
}

// ----- material_editor_tool -----

const ubyte4 material_editor_tool::c_default_color_value		= ubyte4(0x7f, 0x7f, 0x7f, 0xff);
const ubyte4 material_editor_tool::c_default_texture_value		= ubyte4(0xff);
const ubyte4 material_editor_tool::c_default_param_mask_value	= ubyte4(0x00, 0x00, 0x00, 0xff);

material_editor_tool::material_editor_tool(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug), color_miner_(p_device, p_ctx, p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	D3D11_TEXTURE2D_DESC tex_desc = {};
	tex_desc.Width = 1;
	tex_desc.Height = 1;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA tex_data = {};
	// p_tex_base_color_output_color_ ---
	tex_data.pSysMem = &material_editor_tool::c_default_color_value.x;
	tex_data.SysMemPitch = vector_traits<ubyte4>::byte_count;
	HRESULT hr = p_device_->CreateTexture2D(&tex_desc, &tex_data, &p_tex_base_color_output_color_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_base_color_output_color_, nullptr, &p_tex_base_color_output_color_srv_.ptr);
	assert(hr == S_OK);
	// p_tex_reflect_color_output_color_ ---
	const ubyte4 reflect_color(0xff, 0xff, 0xff, uint8_t(0.23f * 0xff));
	tex_data.pSysMem = &reflect_color.x;
	hr = p_device_->CreateTexture2D(&tex_desc, &tex_data, &p_tex_reflect_color_output_color_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_reflect_color_output_color_, nullptr, &p_tex_reflect_color_output_color_srv_.ptr);
	assert(hr == S_OK);
	// p_tex_base_color_input_texture_ ---
	tex_desc.Usage = D3D11_USAGE_IMMUTABLE;
	tex_data.pSysMem = &material_editor_tool::c_default_texture_value;
	hr = p_device_->CreateTexture2D(&tex_desc, &tex_data, &p_tex_base_color_input_texture_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_base_color_input_texture_, nullptr, 
		&p_tex_base_color_input_texture_srv_.ptr);
	assert(hr == S_OK);
	
	// property mask stuff
	property_mask_colors_.reserve(material_composer::c_property_mapping_max_count);
	reset_property_mask_texutre();

	material_.p_tex_base_color_srv = p_tex_base_color_output_color_srv_;
	material_.p_tex_reflect_color_srv = p_tex_reflect_color_output_color_srv_;
}

void material_editor_tool::reset_property_mask_texutre()
{
	assert(property_mask_colors_.capacity() > 0);

	p_tex_property_mask_srv_.dispose();
	p_tex_property_mask_.dispose();
	property_mask_colors_.clear();

	D3D11_TEXTURE2D_DESC tex_desc = {};
	tex_desc.Width = 1;
	tex_desc.Height = 1;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_IMMUTABLE;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA tex_data = {};
	tex_data.pSysMem = &c_default_param_mask_value.x;
	tex_data.SysMemPitch = vector_traits<ubyte4>::byte_count;

	HRESULT hr = p_device_->CreateTexture2D(&tex_desc, &tex_data, &p_tex_property_mask_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_property_mask_, nullptr, &p_tex_property_mask_srv_.ptr);
	assert(hr == S_OK);
}

void material_editor_tool::reload_base_color_input_texture(const char* p_filename)
{
	assert(p_filename);

	p_tex_base_color_input_texture_srv_.dispose();
	p_tex_base_color_input_texture_.dispose();

	const texture_data td = load_from_image_file(p_filename, 4);
	p_tex_base_color_input_texture_ = make_texture_2d(p_device_, td, 
		D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_base_color_input_texture_, nullptr, 
		&p_tex_base_color_input_texture_srv_.ptr);
	assert(hr == S_OK);
}

void material_editor_tool::reload_property_mask_texture(const char* p_filename)
{
	assert(p_filename);

	p_tex_property_mask_srv_.dispose();
	p_tex_property_mask_.dispose();

	const texture_data td = load_from_image_file(p_filename, 4);
	p_tex_property_mask_ = make_texture_2d(p_device_, td, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_property_mask_, nullptr, &p_tex_property_mask_srv_.ptr);
	assert(hr == S_OK);

	color_miner_.perform(p_tex_property_mask_srv_, xy(td.size), property_mask_colors_);
}

void material_editor_tool::update_base_color_color(const ubyte4& value)
{
	p_ctx_->UpdateSubresource(p_tex_base_color_output_color_, 0, nullptr,
		&value.x, vector_traits<ubyte4>::byte_count, 0);
}


} // namespace core
} // namespace sparki
