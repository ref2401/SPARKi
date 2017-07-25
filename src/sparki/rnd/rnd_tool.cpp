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

com_ptr<ID3D11Texture2D> make_skybox_texture(ID3D11Device* p_device, UINT side_size)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = side_size;
	desc.MipLevels = 1;
	desc.ArraySize = 6;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	com_ptr<ID3D11Texture2D> p_tex;
	HRESULT hr = p_device->CreateTexture2D(&desc, nullptr, &p_tex.ptr);
	assert(hr == S_OK);

	return p_tex;
}

} // namespace


namespace sparki {
namespace rnd {

// ----- ibl_texture_builder -----

ibl_texture_builder::ibl_texture_builder(ID3D11Device* p_device,
	ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug,
	const hlsl_compute_desc& hlsl_equirect_to_skybox, const hlsl_compute_desc& hlsl_prefilter_envmap)
	: p_device_(p_device),
	p_ctx_(p_ctx),
	p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	equirect_to_skybox_compute_shader_ = hlsl_compute(p_device, hlsl_equirect_to_skybox);
	prefilter_envmap_compute_shader_ = hlsl_compute(p_device, hlsl_prefilter_envmap);

	D3D11_SAMPLER_DESC smp = {};
	smp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	smp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	smp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	smp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	HRESULT hr = p_device->CreateSamplerState(&smp, &p_sampler_.ptr);
	assert(hr == S_OK);
}

void ibl_texture_builder::convert_equirect_to_skybox(ID3D11ShaderResourceView* p_tex_equirect_srv,
	ID3D11UnorderedAccessView* p_tex_skybox_uav, UINT skybox_side_size)
{
	HRESULT hr;

	// set compute pipeline & dispatch work
	p_ctx_->CSSetShader(equirect_to_skybox_compute_shader_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_equirect_srv);
	p_ctx_->CSSetSamplers(0, 1, &p_sampler_.ptr);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_tex_skybox_uav, nullptr);

#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif

	const UINT gx = skybox_side_size / 512;
	const UINT gy = skybox_side_size / 2;
	p_ctx_->Dispatch(gx, gy, 6);

	// reset pipeline state
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);
}

void ibl_texture_builder::perform(const char* p_hdr_filename,
	const char* p_skybox_filename, UINT skybox_side_size,
	const char* p_envmap_filename, UINT envmap_side_size)
{
	assert(p_hdr_filename);
	assert(p_skybox_filename);
	assert(skybox_side_size >= 512);
	assert(p_envmap_filename);
	assert(envmap_side_size >= 128);

	// load equirectangular texture
	com_ptr<ID3D11Texture2D> p_tex_equirect = load_equirect_texture(p_device_, p_hdr_filename);
	com_ptr<ID3D11ShaderResourceView> p_tex_equirect_srv;
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_equirect, nullptr, &p_tex_equirect_srv.ptr);
	assert(hr == S_OK);

	// create skybox texture
	com_ptr<ID3D11Texture2D> p_tex_skybox = make_skybox_texture(p_device_, skybox_side_size);
	com_ptr<ID3D11UnorderedAccessView> p_tex_skybox_uav;
	hr = p_device_->CreateUnorderedAccessView(p_tex_skybox, nullptr, &p_tex_skybox_uav.ptr);
	assert(hr == S_OK);

	convert_equirect_to_skybox(p_tex_equirect_srv, p_tex_skybox_uav, skybox_side_size);

	// create temporary texture (usage: staging)
	D3D11_TEXTURE2D_DESC tmp_desc;
	p_tex_skybox->GetDesc(&tmp_desc);
	tmp_desc.Usage = D3D11_USAGE_STAGING;
	tmp_desc.BindFlags = 0;
	tmp_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	com_ptr<ID3D11Texture2D> p_tex_tmp;
	hr = p_device_->CreateTexture2D(&tmp_desc, nullptr, &p_tex_tmp.ptr);
	assert(hr == S_OK);
	p_ctx_->CopyResource(p_tex_tmp, p_tex_skybox);

	const texture_data td = make_texture_data(p_ctx_, p_tex_tmp);
	write_tex(p_skybox_filename, td);
}

} // namespace rnd
} // namespace sparki
