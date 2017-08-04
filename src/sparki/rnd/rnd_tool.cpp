#include "sparki/rnd/rnd_tool.h"

#include <cassert>


namespace {

using namespace sparki;
using namespace sparki::rnd;

com_ptr<ID3D11Texture2D> load_equirect_texture(ID3D11Device* p_device, const char* p_filename)
{
	assert(p_device);
	assert(p_filename);

	const image_2d img = image_2d(p_filename, 4);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = UINT(img.width());
	desc.Height = UINT(img.height());
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = img.p_data();
	data.SysMemPitch = UINT(img.width() * byte_count(img.pixel_format()));

	com_ptr<ID3D11Texture2D> p_tex;
	HRESULT hr = p_device->CreateTexture2D(&desc, &data, &p_tex.ptr);
	assert(hr == S_OK);

	return p_tex;
}

} // namespace


namespace sparki {
namespace rnd {

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
		const texture_data td = make_texture_data(p_device_, p_ctx_, p_tex);
		write_tex(p_specular_brdf_filename, td);
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
	
	const hlsl_compute_desc hlsl_specular_envmap("../../data/shaders/specular_envmap.compute.hlsl");
	specular_envmap_compute_ = hlsl_compute(p_device, hlsl_specular_envmap);

	p_cb_prefilter_envmap_ = constant_buffer(p_device, sizeof(float4));
}

com_ptr<ID3D11Texture2D> envmap_texture_builder::make_cube_texture(UINT side_size,
	UINT mipmap_level_count, D3D11_USAGE usage, UINT bing_flags, UINT misc_flags)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = side_size;
	desc.MipLevels = mipmap_level_count;
	desc.ArraySize = 6;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bing_flags;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	if (misc_flags != 0)
		desc.MiscFlags |= misc_flags;

	com_ptr<ID3D11Texture2D> p_tex;
	HRESULT hr = p_device_->CreateTexture2D(&desc, nullptr, &p_tex.ptr);
	assert(hr == S_OK);

	return p_tex;
}

com_ptr<ID3D11Texture2D> envmap_texture_builder::make_skybox(const char* p_hdr_filename)
{
	// equirect texture
	com_ptr<ID3D11Texture2D> p_tex_equirect = load_equirect_texture(p_device_, p_hdr_filename);
	com_ptr<ID3D11ShaderResourceView> p_tex_equirect_srv;
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_equirect, nullptr, &p_tex_equirect_srv.ptr);
	assert(hr == S_OK);

	// skybox texture & uav
	com_ptr<ID3D11Texture2D> p_tex_skybox = make_cube_texture(
		envmap_texture_builder::skybox_side_size, 
		envmap_texture_builder::envmap_mipmap_count,
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

	p_ctx_->Dispatch(envmap_texture_builder::skybox_compute_gx, envmap_texture_builder::skybox_compute_gy, 6);

	// reset uav binding
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	return p_tex_skybox;
}

com_ptr<ID3D11Texture2D> envmap_texture_builder::make_specular_envmap(
	ID3D11ShaderResourceView* p_tex_skybox_srv)
{
	com_ptr<ID3D11Texture2D> p_tex_envmap = make_cube_texture(
		envmap_texture_builder::envmap_side_size,
		envmap_texture_builder::envmap_mipmap_count,
		D3D11_USAGE_DEFAULT, 
		D3D11_BIND_UNORDERED_ACCESS);

	// setup compute pipeline & dispatch work
	p_ctx_->CSSetShader(specular_envmap_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetConstantBuffers(0, 1, &p_cb_prefilter_envmap_.ptr);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_skybox_srv);
	p_ctx_->CSSetSamplers(0, 1, &p_sampler_);

	// for each mipmap level
	for (UINT m = 0; m < envmap_texture_builder::envmap_mipmap_count; ++m) {
		const float cb_data[4] = {
			/* roughness */ float(m) / (envmap_texture_builder::envmap_mipmap_count - 1),
			/* side_size */ float(envmap_texture_builder::envmap_side_size >> m),
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
		const UINT mipmap_size = envmap_texture_builder::envmap_side_size >> m;
		const UINT gx = (mipmap_size) / envmap_texture_builder::envmap_compute_group_x_size;
		const UINT gy = (mipmap_size) / envmap_texture_builder::envmap_compute_group_y_size;
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

	// make skybox texture & generate its mips
	com_ptr<ID3D11Texture2D> p_tex_skybox = make_skybox(p_hdr_filename);
	com_ptr<ID3D11ShaderResourceView> p_tex_skybox_srv;
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_skybox, nullptr, &p_tex_skybox_srv.ptr);
	assert(hr == S_OK);
	p_ctx_->GenerateMips(p_tex_skybox_srv);

	com_ptr<ID3D11Texture2D> p_tex_specular_envmap = make_specular_envmap(p_tex_skybox_srv);

	// write skybox texture to a file
	{
		const texture_data td = make_texture_data(p_device_, p_ctx_, p_tex_skybox);
		write_tex(p_skybox_filename, td);
	}

	// write envmap texture to a file
	{
		const texture_data td = make_texture_data(p_device_, p_ctx_, p_tex_specular_envmap);
		write_tex(p_specular_envmap_filename, td);
	}
}

} // namespace rnd
} // namespace sparki
