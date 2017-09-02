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

com_ptr<ID3D11Buffer> make_constant_buffer(ID3D11Device* p_device, size_t byte_count)
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

DXGI_FORMAT make_dxgi_format(sparki::pixel_format fmt) noexcept
{
	assert(fmt != sparki::pixel_format::rgb_8);

	switch (fmt) {
		default:
		case sparki::pixel_format::none:		return DXGI_FORMAT_UNKNOWN;
		case sparki::pixel_format::rg_16f:		return DXGI_FORMAT_R16G16_FLOAT;
		case sparki::pixel_format::rgba_16f:	return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case sparki::pixel_format::rg_32f:		return DXGI_FORMAT_R32G32_FLOAT;
		case sparki::pixel_format::rgb_32f:		return DXGI_FORMAT_R32G32B32_FLOAT;
		case sparki::pixel_format::rgba_32f:	return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case sparki::pixel_format::red_8:		return DXGI_FORMAT_R8_UNORM;
		case sparki::pixel_format::rg_8:		return DXGI_FORMAT_R8G8_UNORM;
		case sparki::pixel_format::rgba_8:		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

sparki::pixel_format make_pixel_format(DXGI_FORMAT fmt) noexcept
{
	switch (fmt) {
		default:								return sparki::pixel_format::none;
		
		case DXGI_FORMAT_R16G16_FLOAT:			return sparki::pixel_format::rg_16f;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:	return sparki::pixel_format::rgba_16f;
		case DXGI_FORMAT_R32G32_FLOAT:			return sparki::pixel_format::rg_32f;
		case DXGI_FORMAT_R32G32B32_FLOAT:		return sparki::pixel_format::rgb_32f;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return sparki::pixel_format::rgba_32f;
		case DXGI_FORMAT_R8_UNORM:				return sparki::pixel_format::red_8;
		case DXGI_FORMAT_R8G8_UNORM:			return sparki::pixel_format::rg_8;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return sparki::pixel_format::rgba_8;
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

texture_data make_texture_data_new(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, 
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

} // namespace rnd
} // namespace sparki
