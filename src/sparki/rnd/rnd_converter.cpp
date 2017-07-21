#include "sparki/rnd/rnd_converter.h"

#include <cassert>


namespace sparki {

// ----- equirect_to_skybox_converter -----

equirect_to_skybox_converter::equirect_to_skybox_converter(ID3D11Device* p_device)
{
	assert(p_device);

	hlsl_compute_desc compute_desc("../../data/shaders/equirectangular_to_cube.compute.hlsl");
	compute_shader_ = hlsl_compute(p_device, compute_desc);
}

void equirect_to_skybox_converter::convert(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, 
	ID3D11Debug* p_debug, const char* p_source_filename, ID3D11UnorderedAccessView* p_uav)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.
	assert(p_source_filename);
	assert(p_uav);

	// load equirectangular texture
	com_ptr<ID3D11Texture2D> p_tex_equirect;
	com_ptr<ID3D11ShaderResourceView> p_tex_equirect_srv;
	const image_2d img = image_2d(p_source_filename, 4);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = UINT(img.size.x);
	desc.Height = UINT(img.size.y);
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = img.p_data;
	data.SysMemPitch = UINT(img.size.x * byte_count(img.pixel_format));
	HRESULT hr = p_device->CreateTexture2D(&desc, &data, &p_tex_equirect.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateShaderResourceView(p_tex_equirect, nullptr, &p_tex_equirect_srv.ptr);
	assert(hr == S_OK);

	// set compute pipeline & dispatch work
	p_ctx->CSSetShader(compute_shader_.p_compute_shader, nullptr, 0);
	p_ctx->CSSetShaderResources(0, 1, &p_tex_equirect_srv.ptr);
	p_ctx->CSSetUnorderedAccessViews(0, 1, &p_uav, nullptr);

#ifdef SPARKI_DEBUG
	hr = p_debug->ValidateContextForDispatch(p_ctx);
	assert(hr == S_OK);
#endif
	p_ctx->Dispatch(2, 512, 6);

	// reset pipeline state
	p_ctx->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);
}

} // namespace sparki
