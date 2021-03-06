#include "sparki/core/rnd_base.h"

#include <cassert>
#include <algorithm>


namespace sparki {
namespace core {

// ----- hlsl_compute -----

hlsl_compute::hlsl_compute(ID3D11Device* p_device, const hlsl_compute_desc& desc)
{
	assert(p_device);
	assert(desc.source_code.length() > 0);

	try {
		p_compute_shader_bytecode = compile_shader(desc.source_code, desc.source_filename,
			desc.compile_flags,
			hlsl_compute_desc::compute_shader_entry_point,
			hlsl_compute_desc::compute_shader_model);

		HRESULT hr = p_device->CreateComputeShader(
			p_compute_shader_bytecode->GetBufferPointer(),
			p_compute_shader_bytecode->GetBufferSize(),
			nullptr, &p_compute_shader.ptr);

		ENFORCE(hr == S_OK, std::to_string(hr));
	}
	catch (...) {
		const std::string exc_msg = EXCEPTION_MSG("Compute shader creation error.");
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

// ----- hlsl_shader -----

hlsl_shader::hlsl_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc)
{
	assert(p_device);
	assert(desc.source_code.length() > 0);

	init_vertex_shader(p_device, desc);
	init_pixel_shader(p_device, desc);

	if (desc.tesselation_stage) {
		init_hull_shader(p_device, desc);
		init_domain_shader(p_device, desc);
	}
}

void hlsl_shader::init_vertex_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc)
{
	try {
		p_vertex_shader_bytecode = compile_shader(desc.source_code, desc.source_filename,
			desc.compile_flags,
			hlsl_shader_desc::vertex_shader_entry_point,
			hlsl_shader_desc::vertex_shader_model);

		HRESULT hr = p_device->CreateVertexShader(
			p_vertex_shader_bytecode->GetBufferPointer(),
			p_vertex_shader_bytecode->GetBufferSize(),
			nullptr, &p_vertex_shader.ptr);

		ENFORCE(hr == S_OK, std::to_string(hr));
	}
	catch (...) {
		const std::string exc_msg = EXCEPTION_MSG("Vertex shader creation error.");
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

void hlsl_shader::init_hull_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc)
{
	try {
		p_hull_shader_bytecode = compile_shader(desc.source_code, desc.source_filename,
			desc.compile_flags,
			hlsl_shader_desc::hull_shader_entry_point,
			hlsl_shader_desc::hull_shader_model);

		HRESULT hr = p_device->CreateHullShader(
			p_hull_shader_bytecode->GetBufferPointer(),
			p_hull_shader_bytecode->GetBufferSize(),
			nullptr, &p_hull_shader.ptr);

		ENFORCE(hr == S_OK, std::to_string(hr));
	}
	catch (...) {
		const std::string exc_msg = EXCEPTION_MSG("Hull shader creation error.");
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

void hlsl_shader::init_domain_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc)
{
	try {
		p_domain_shader_bytecode = compile_shader(desc.source_code, desc.source_filename,
			desc.compile_flags,
			hlsl_shader_desc::domain_shader_entry_point,
			hlsl_shader_desc::domain_shader_model);

		HRESULT hr = p_device->CreateDomainShader(
			p_domain_shader_bytecode->GetBufferPointer(),
			p_domain_shader_bytecode->GetBufferSize(),
			nullptr, &p_domain_shader.ptr);

		ENFORCE(hr == S_OK, std::to_string(hr));
	}
	catch (...) {
		const std::string exc_msg = EXCEPTION_MSG("Domain shader creation error.");
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

void hlsl_shader::init_pixel_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc)
{
	try {
		p_pixel_shader_bytecode = compile_shader(desc.source_code, desc.source_filename,
			desc.compile_flags,
			hlsl_shader_desc::pixel_shader_entry_point,
			hlsl_shader_desc::pixel_shader_model);

		HRESULT hr = p_device->CreatePixelShader(
			p_pixel_shader_bytecode->GetBufferPointer(),
			p_pixel_shader_bytecode->GetBufferSize(),
			nullptr, &p_pixel_shader.ptr);

		ENFORCE(hr == S_OK, std::to_string(hr));
	}
	catch (...) {
		const std::string exc_msg = EXCEPTION_MSG("Pixel shader creation error.");
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

// ----- gbuffer -----

gbuffer::gbuffer(ID3D11Device* p_device)
{
	assert(p_device);

	D3D11_BLEND_DESC blend_desc = {};
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HRESULT hr = p_device->CreateBlendState(&blend_desc, &p_blend_state_no_blend.ptr);
	assert(hr == S_OK);

	D3D11_RASTERIZER_DESC rastr_desc = {};
	rastr_desc.FillMode = D3D11_FILL_SOLID;
	rastr_desc.CullMode = D3D11_CULL_BACK;
	rastr_desc.FrontCounterClockwise = true;
	hr = p_device->CreateRasterizerState(&rastr_desc, &p_rasterizer_state.ptr);
	assert(hr == S_OK);

	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = p_device->CreateSamplerState(&sampler_desc, &p_sampler_linear.ptr);
	assert(hr == S_OK);

	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = p_device->CreateSamplerState(&sampler_desc, &p_sampler_point.ptr);
	assert(hr == S_OK);
}

void gbuffer::resize(ID3D11Device* p_device, const uint2 size)
{
	assert(size > 0);

	const UINT width = UINT(size.x * gbuffer::viewport_factor);
	const UINT height = UINT(size.y * gbuffer::viewport_factor);

	p_tex_color.dispose();
	p_tex_color_srv.dispose();
	p_tex_color_rtv.dispose();
	p_tex_tone_mapping.dispose();
	p_tex_tone_mapping_srv.dispose();
	p_tex_tone_mapping_uav.dispose();
	p_tex_aa.dispose();
	p_tex_aa_srv.dispose();
	p_tex_aa_uav.dispose();
	p_tex_depth.dispose();
	p_tex_depth_dsv.dispose();

	// update color texture & its views
	D3D11_TEXTURE2D_DESC color_desc = {};
	color_desc.Width				= width;
	color_desc.Height				= height;
	color_desc.MipLevels			= 1;
	color_desc.ArraySize			= 1;
	color_desc.Format				= DXGI_FORMAT_R32G32B32A32_FLOAT;
	color_desc.SampleDesc.Count		= 1;
	color_desc.SampleDesc.Quality	= 0;
	color_desc.Usage				= D3D11_USAGE_DEFAULT;
	color_desc.BindFlags			= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	HRESULT hr = p_device->CreateTexture2D(&color_desc, nullptr, &p_tex_color.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateShaderResourceView(p_tex_color, nullptr, &p_tex_color_srv.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateRenderTargetView(p_tex_color, nullptr, &p_tex_color_rtv.ptr);
	assert(hr == S_OK);

	// update postproc textures & its view
	D3D11_TEXTURE2D_DESC pp_desc = {};
	pp_desc.Width				= width;
	pp_desc.Height				= height;
	pp_desc.MipLevels			= 1;
	pp_desc.ArraySize			= 1;
	pp_desc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	pp_desc.SampleDesc.Count	= 1;
	pp_desc.SampleDesc.Quality	= 0;
	pp_desc.Usage				= D3D11_USAGE_DEFAULT;
	pp_desc.BindFlags			= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	hr = p_device->CreateTexture2D(&pp_desc, nullptr, &p_tex_tone_mapping.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateShaderResourceView(p_tex_tone_mapping, nullptr, &p_tex_tone_mapping_srv.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateUnorderedAccessView(p_tex_tone_mapping, nullptr, &p_tex_tone_mapping_uav.ptr);
	assert(hr == S_OK);

	// update aa texture & its view
	hr = p_device->CreateTexture2D(&pp_desc, nullptr, &p_tex_aa.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateShaderResourceView(p_tex_aa, nullptr, &p_tex_aa_srv.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateUnorderedAccessView(p_tex_aa, nullptr, &p_tex_aa_uav.ptr);
	assert(hr == S_OK);

	// update depth texture & its views
	D3D11_TEXTURE2D_DESC ds_desc = {};
	ds_desc.Width				= width;
	ds_desc.Height				= height;
	ds_desc.MipLevels			= 1;
	ds_desc.ArraySize			= 1;
	ds_desc.Format				= DXGI_FORMAT_D32_FLOAT;
	ds_desc.SampleDesc.Count	= 1;
	ds_desc.SampleDesc.Quality	= 0;
	ds_desc.Usage				= D3D11_USAGE_DEFAULT;
	ds_desc.BindFlags			= D3D11_BIND_DEPTH_STENCIL;
	hr = p_device->CreateTexture2D(&ds_desc, nullptr, &p_tex_depth.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateDepthStencilView(p_tex_depth, nullptr, &p_tex_depth_dsv.ptr);
	assert(hr == S_OK);

	// rnd viewport
	rnd_viewport.TopLeftX = 0;
	rnd_viewport.TopLeftY = 0;
	rnd_viewport.Width = float(width);
	rnd_viewport.Height = float(height);
	rnd_viewport.MinDepth = 0.0;
	rnd_viewport.MaxDepth = 1.0f;

	// update viewport
	window_viewport.TopLeftX = 0;
	window_viewport.TopLeftY = 0;
	window_viewport.Width = float(size.x);
	window_viewport.Height = float(size.y);
	window_viewport.MinDepth = 0.0;
	window_viewport.MaxDepth = 1.0f;
}

// ----- funcs -----

com_ptr<ID3DBlob> compile_shader(const std::string& source_code, const std::string& source_filename,
	uint32_t compile_flags, const char* p_entry_point_name, const char* p_shader_model)
{
	com_ptr<ID3DBlob> p_bytecode;
	com_ptr<ID3DBlob> p_error_blob;

	const char* p_filename = (source_filename.empty()) ? nullptr : source_filename.c_str();
	HRESULT hr = D3DCompile(
		source_code.c_str(),
		source_code.size(),
		p_filename,
		nullptr,							// defines
		D3D_COMPILE_STANDARD_FILE_INCLUDE,	// includes
		p_entry_point_name,
		p_shader_model,
		compile_flags,
		0,									// effect compilation flags
		&p_bytecode.ptr,
		&p_error_blob.ptr
	);

	if (hr != S_OK) {
		std::string error(static_cast<char*>(p_error_blob->GetBufferPointer()), p_error_blob->GetBufferSize());
		throw std::runtime_error(error);
	}

	return p_bytecode;
}

com_ptr<ID3D11Buffer> make_buffer(ID3D11Device* p_device, UINT byte_count, 
	D3D11_USAGE usage, UINT bing_flags, UINT cpu_access_flags)
{
	assert(p_device);
	assert(byte_count > 0);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth		= byte_count;
	desc.Usage			= usage;
	desc.BindFlags		= bing_flags;
	desc.CPUAccessFlags	= cpu_access_flags;

	com_ptr<ID3D11Buffer> buffer;
	HRESULT hr = p_device->CreateBuffer(&desc, nullptr, &buffer.ptr);
	assert(hr == S_OK);

	return buffer;
}

com_ptr<ID3D11Buffer> make_constant_buffer(ID3D11Device* p_device, UINT byte_count)
{
	assert(p_device);
	assert(byte_count > 0);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = byte_count;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	com_ptr<ID3D11Buffer> buffer;
	HRESULT hr = p_device->CreateBuffer(&desc, nullptr, &buffer.ptr);
	assert(hr == S_OK);

	return buffer;
}

com_ptr<ID3D11Buffer> make_structured_buffer(ID3D11Device* p_device, UINT item_byte_count, UINT item_count,
	D3D11_USAGE usage, UINT bing_flags)
{
	assert(p_device);
	assert(item_byte_count > 0);
	assert(item_count > 0);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = item_byte_count * item_count;
	desc.Usage = usage;
	desc.BindFlags = bing_flags;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = item_byte_count;

	com_ptr<ID3D11Buffer> buffer;
	HRESULT hr = p_device->CreateBuffer(&desc, nullptr, &buffer.ptr);
	assert(hr == S_OK);

	return buffer;
}

DXGI_FORMAT make_dxgi_format(sparki::core::pixel_format fmt) noexcept
{
	assert(fmt != pixel_format::rgb_8);

	switch (fmt) {
		default:
		case pixel_format::none:		return DXGI_FORMAT_UNKNOWN;
		case pixel_format::rg_16f:		return DXGI_FORMAT_R16G16_FLOAT;
		case pixel_format::rgba_16f:	return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case pixel_format::rg_32f:		return DXGI_FORMAT_R32G32_FLOAT;
		case pixel_format::rgb_32f:		return DXGI_FORMAT_R32G32B32_FLOAT;
		case pixel_format::rgba_32f:	return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case pixel_format::red_8:		return DXGI_FORMAT_R8_UNORM;
		case pixel_format::rg_8:		return DXGI_FORMAT_R8G8_UNORM;
		case pixel_format::rgba_8:		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

pixel_format make_pixel_format(DXGI_FORMAT fmt) noexcept
{
	switch (fmt) {
		default:								return pixel_format::none;
		
		case DXGI_FORMAT_R16G16_FLOAT:			return pixel_format::rg_16f;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:	return pixel_format::rgba_16f;
		case DXGI_FORMAT_R32G32_FLOAT:			return pixel_format::rg_32f;
		case DXGI_FORMAT_R32G32B32_FLOAT:		return pixel_format::rgb_32f;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return pixel_format::rgba_32f;
		case DXGI_FORMAT_R8_UNORM:				return pixel_format::red_8;
		case DXGI_FORMAT_R8G8_UNORM:			return pixel_format::rg_8;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return pixel_format::rgba_8;
	}
}

com_ptr<ID3D11Texture2D> make_texture_2d(ID3D11Device* p_device, const texture_data& td,
	D3D11_USAGE usage, UINT bind_flags)
{
	assert(p_device);
	assert(td.type == texture_type::texture_2d);
	assert(is_valid_texture_data(td));

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = td.size.x;
	desc.Height = td.size.y;
	desc.MipLevels = td.mipmap_count;
	desc.ArraySize = td.array_size;
	desc.Format = make_dxgi_format(td.format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bind_flags;

	const size_t fmt_byte_count = byte_count(td.format);
	const uint8_t* ptr = td.buffer.data();
	std::vector<D3D11_SUBRESOURCE_DATA> data_list(desc.ArraySize * desc.MipLevels);

	for (UINT a = 0; a < desc.ArraySize; ++a) {
		for (UINT m = 0; m < desc.MipLevels; ++m) {
			const UINT w = desc.Width >> m;
			const UINT h = desc.Height >> m;
			const UINT index = a * desc.MipLevels + m;

			data_list[index].pSysMem = ptr;
			data_list[index].SysMemPitch = UINT(w * fmt_byte_count);
			data_list[index].SysMemSlicePitch = 0;
			ptr += w * h * fmt_byte_count;
		}
	}

	com_ptr<ID3D11Texture2D> p_tex;
	HRESULT hr = p_device->CreateTexture2D(&desc, data_list.data(), &p_tex.ptr);
	assert(hr == S_OK);
	return p_tex;
}

com_ptr<ID3D11Texture2D> make_texture_cube(ID3D11Device* p_device, const texture_data& td,
	D3D11_USAGE usage, UINT bind_flags, UINT misc_flags)
{
	assert(p_device);
	assert(td.type == texture_type::texture_cube);
	assert(td.array_size == 6);
	assert(is_valid_texture_data(td));

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = td.size.x;
	desc.Height = td.size.y;
	desc.MipLevels = td.mipmap_count;
	desc.ArraySize = td.array_size;
	desc.Format = make_dxgi_format(td.format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bind_flags;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	if (misc_flags != 0)
		desc.MiscFlags |= misc_flags;

	const size_t fmt_byte_count = byte_count(td.format);
	const uint8_t* ptr = td.buffer.data();
	std::vector<D3D11_SUBRESOURCE_DATA> data_list(desc.ArraySize * desc.MipLevels);

	for (UINT a = 0; a < desc.ArraySize; ++a) {
		for (UINT m = 0; m < desc.MipLevels; ++m) {
			const UINT w = desc.Width >> m;
			const UINT h = desc.Height >> m;
			const UINT index = a * desc.MipLevels + m;

			data_list[index].pSysMem = ptr;
			data_list[index].SysMemPitch = UINT(w * fmt_byte_count);
			data_list[index].SysMemSlicePitch = 0;
			ptr += w * h * fmt_byte_count;
		}
	}

	com_ptr<ID3D11Texture2D> p_tex;
	HRESULT hr = p_device->CreateTexture2D(&desc, data_list.data(), &p_tex.ptr);
	assert(hr == S_OK);
	return p_tex;
}

com_ptr<ID3D11Texture2D> make_texture_cube(ID3D11Device* p_device, UINT side_size, UINT mipmap_count,
	DXGI_FORMAT format, D3D11_USAGE usage, UINT bing_flags, UINT misc_flags)
{
	assert(p_device);
	assert(side_size > 0);
	assert(mipmap_count > 0);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = side_size;
	desc.MipLevels = mipmap_count;
	desc.ArraySize = 6;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bing_flags;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	if (misc_flags != 0)
		desc.MiscFlags |= misc_flags;

	com_ptr<ID3D11Texture2D> p_tex;
	HRESULT hr = p_device->CreateTexture2D(&desc, nullptr, &p_tex.ptr);
	assert(hr == S_OK);

	return p_tex;
}

texture_data make_texture_data(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, 
	texture_type type, ID3D11Texture2D* p_tex)
{
	assert(p_device);
	assert(p_ctx);
	assert(type != texture_type::unknown);
	assert(p_tex);

	// create a staging texture & fill it with p_tex data.
	D3D11_TEXTURE2D_DESC desc;
	p_tex->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = desc.MiscFlags & (~D3D11_RESOURCE_MISC_GENERATE_MIPS);

	com_ptr<ID3D11Texture2D> p_tex_staging;
	HRESULT hr = p_device->CreateTexture2D(&desc, nullptr, &p_tex_staging.ptr);
	assert(hr == S_OK);
	p_ctx->CopyResource(p_tex_staging, p_tex);


#ifdef SPARKI_DEBUG
	if (type == texture_type::texture_cube) assert(desc.ArraySize == 6);
#endif // SPAKIR_DEBUG

	// create a texture_data object
	texture_data td(type, uint3(desc.Width, desc.Height, 1), 
		desc.MipLevels, desc.ArraySize, make_pixel_format(desc.Format));
	
	const size_t fmt_byte_count = byte_count(td.format);
	uint8_t* ptr = td.buffer.data();

	// for each array slice 
	// for each mipmap level of the slice
	// map p_tex_tmp[array_slice][mipmap_level] and copy its' contents into the texture_data object.
	for (UINT a = 0; a < desc.ArraySize; ++a) {
		for (UINT m = 0; m < desc.MipLevels; ++m) {
			const UINT index = D3D11CalcSubresource(m, a, desc.MipLevels);

			D3D11_MAPPED_SUBRESOURCE map;
			hr = p_ctx->Map(p_tex_staging, index, D3D11_MAP_READ, 0, &map);
			assert(hr == S_OK);

			const UINT w = desc.Width >> m;
			const UINT h = desc.Height >> m;
			const size_t mip_bc = w * h * fmt_byte_count;
			assert(mip_bc > 0);
			assert(mip_bc <= map.DepthPitch);

			if (mip_bc == map.DepthPitch) {
				std::memcpy(ptr, map.pData, mip_bc);
				ptr += mip_bc;
			}
			else {
				uint8_t* p_src = reinterpret_cast<uint8_t*>(map.pData);
				const size_t row_bc = w * fmt_byte_count;
				// Note(MSDN): The runtime might assign values to RowPitch and DepthPitch 
				// that are larger than anticipated because there might be padding between rows and depth.
				// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476182(v=vs.85).aspx
				for (size_t row = 0; row < h; ++row) {
					std::memcpy(ptr, p_src, row_bc);
					p_src += map.RowPitch;
					ptr += row_bc;
				}
			}

			p_ctx->Unmap(p_tex_staging, index);
		}
	}

	return td;
}

} // namespace core
} // namespace sparki
