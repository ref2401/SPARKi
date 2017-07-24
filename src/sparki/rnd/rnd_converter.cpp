#include "sparki/rnd/rnd_converter.h"

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

// ----- equirect_to_skybox_converter -----

equirect_to_skybox_converter::equirect_to_skybox_converter(ID3D11Device* p_device)
{
	assert(p_device);

	hlsl_compute_desc compute_desc("../../data/shaders/equirectangular_to_cube.compute.hlsl");
	compute_shader_ = hlsl_compute(p_device, compute_desc);
}

void equirect_to_skybox_converter::convert(const char* p_source_filename, const char* p_dest_filename, 
	ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug, UINT side_size)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.
	assert(p_source_filename);
	assert(p_dest_filename);
	assert(side_size > 0);

	// load equirectangular texture
	com_ptr<ID3D11Texture2D> p_tex_equirect = load_equirect_texture(p_device, p_source_filename);
	com_ptr<ID3D11ShaderResourceView> p_tex_equirect_srv;
	HRESULT hr = p_device->CreateShaderResourceView(p_tex_equirect, nullptr, &p_tex_equirect_srv.ptr);
	assert(hr == S_OK);

	// dest skybox texture
	com_ptr<ID3D11Texture2D> p_tex_skybox = make_skybox_texture(p_device, side_size);
	com_ptr<ID3D11UnorderedAccessView> p_tex_skybox_uav;
	hr = p_device->CreateUnorderedAccessView(p_tex_skybox, nullptr, &p_tex_skybox_uav.ptr);
	assert(hr == S_OK);

	// set compute pipeline & dispatch work
	p_ctx->CSSetShader(compute_shader_.p_compute_shader, nullptr, 0);
	p_ctx->CSSetShaderResources(0, 1, &p_tex_equirect_srv.ptr);
	p_ctx->CSSetUnorderedAccessViews(0, 1, &p_tex_skybox_uav.ptr, nullptr);

#ifdef SPARKI_DEBUG
	hr = p_debug->ValidateContextForDispatch(p_ctx);
	assert(hr == S_OK);
#endif
	p_ctx->Dispatch(2, 512, 6);

	// reset pipeline state
	p_ctx->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	// create temporary texture (usage: staging)
	D3D11_TEXTURE2D_DESC tmp_desc;
	p_tex_skybox->GetDesc(&tmp_desc);
	tmp_desc.Usage = D3D11_USAGE_STAGING;
	tmp_desc.BindFlags = 0;
	tmp_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	com_ptr<ID3D11Texture2D> p_tex_tmp;
	hr = p_device->CreateTexture2D(&tmp_desc, nullptr, &p_tex_tmp.ptr);
	assert(hr == S_OK);
	p_ctx->CopyResource(p_tex_tmp, p_tex_skybox);

	const texture_data td = make_texture_data(p_ctx, p_tex_tmp);
	write_tex(p_dest_filename, td);
}

} // namespace rnd
} // namespace sparki
