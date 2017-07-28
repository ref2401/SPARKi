#include "sparki/rnd/rnd_base.h"

#include <cassert>
#include <algorithm>


namespace sparki {
namespace rnd {

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

// ----- funcs -----

DXGI_FORMAT dxgi_format(sparki::pixel_format fmt) noexcept
{
	assert(fmt != sparki::pixel_format::rgb_8);

	switch (fmt) {
		default: 
		case sparki::pixel_format::none:		return DXGI_FORMAT_UNKNOWN;
		case sparki::pixel_format::rg_32f:		return DXGI_FORMAT_R32G32_FLOAT;
		case sparki::pixel_format::rgb_32f:		return DXGI_FORMAT_R32G32B32_FLOAT;
		case sparki::pixel_format::rgba_32f:	return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case sparki::pixel_format::red_8:		return DXGI_FORMAT_R8_UNORM;
		case sparki::pixel_format::rg_8:		return DXGI_FORMAT_R8G8_UNORM;
		case sparki::pixel_format::rgba_8:		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

com_ptr<ID3DBlob> compile_shader(const std::string& source_code, const std::string& source_filename,
	uint32_t compile_flags, const char* p_entry_point_name, const char* p_shader_model)
{
	com_ptr<ID3DBlob> p_bytecode;
	com_ptr<ID3DBlob> p_error_blob;

	HRESULT hr = D3DCompile(
		source_code.c_str(),
		source_code.size(),
		source_filename.c_str(),
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

com_ptr<ID3D11Buffer> constant_buffer(ID3D11Device* p_device, size_t byte_count)
{
	assert(p_device);
	assert(byte_count > 0);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = UINT(byte_count);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	com_ptr<ID3D11Buffer> buffer;
	HRESULT hr = p_device->CreateBuffer(&desc, nullptr, &buffer.ptr);
	assert(hr == S_OK);

	return buffer;
}

sparki::pixel_format pixel_format(DXGI_FORMAT fmt) noexcept
{
	switch (fmt) {
		default:								return sparki::pixel_format::none;
		
		case DXGI_FORMAT_R32G32_FLOAT:			return sparki::pixel_format::rg_32f;
		case DXGI_FORMAT_R32G32B32_FLOAT:		return sparki::pixel_format::rgb_32f;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return sparki::pixel_format::rgba_32f;
		case DXGI_FORMAT_R8_UNORM:				return sparki::pixel_format::red_8;
		case DXGI_FORMAT_R8G8_UNORM:			return sparki::pixel_format::rg_8;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return sparki::pixel_format::rgba_8;
	}
}

com_ptr<ID3D11Texture2D> texture2d(ID3D11Device* p_device, const texture_data& td, 
	D3D11_USAGE usage, UINT bind_flags)
{
	assert(p_device);
	assert(is_valid_texture_data(td));

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = td.size.x;
	desc.Height = td.size.y;
	desc.MipLevels = td.mipmap_level_count;
	desc.ArraySize = td.size.z;
	desc.Format = dxgi_format(td.format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bind_flags;
	if (td.size.z == 6) desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	assert(td.size.z == 6); // the other cases have not been implemented yet and tested.

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

texture_data make_texture_data(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Texture2D* p_tex)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_tex);

	// create a staging texture & fill it with p_tex data.
	D3D11_TEXTURE2D_DESC desc;
	p_tex->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	com_ptr<ID3D11Texture2D> p_tex_staging;
	HRESULT hr = p_device->CreateTexture2D(&desc, nullptr, &p_tex_staging.ptr);
	assert(hr == S_OK);
	p_ctx->CopyResource(p_tex_staging, p_tex);

	// create a texture_data object
	texture_data td(uint3(desc.Width, desc.Height, 6), desc.MipLevels, rnd::pixel_format(desc.Format));
	uint8_t* ptr = td.buffer.data();

	// for each array slice 
	// for each mipmap level of the slice
	// map p_tex_tmp[array_slice][mipmap_level] and copy its' contents into the texture_data object.
	for (UINT a = 0; a < desc.ArraySize; ++a) {
		for (UINT m = 0; m < desc.MipLevels; ++m) {
			const UINT index = a * desc.MipLevels + m;
			
			D3D11_MAPPED_SUBRESOURCE map;
			HRESULT hr = p_ctx->Map(p_tex_staging, index, D3D11_MAP_READ, 0, &map);
			assert(hr == S_OK);

			// Note(MSDN): The runtime might assign values to RowPitch and DepthPitch 
			// that are larger than anticipated because there might be padding between rows and depth.
			// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476182(v=vs.85).aspx
			const UINT w = desc.Width >> m;
			const UINT h = desc.Height >> m;
			const size_t bc = w * h * byte_count(td.format);
			assert(bc > 0);
			assert(map.DepthPitch >= bc);

			std::memcpy(ptr, map.pData, bc);
			ptr += bc;
			p_ctx->Unmap(p_tex_staging, index);
		}
	}

	return td;
}

} // namespace rnd
} // namespace sparki
