#include "sparki/rnd/rnd_base.h"

#include <cassert>


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

DXGI_FORMAT dxgi_format(sparki::pixel_format& fmt) noexcept
{
	assert(fmt != sparki::pixel_format::rgb_8);

	switch (fmt) {
		default: 
		case sparki::pixel_format::none:		return DXGI_FORMAT_UNKNOWN;
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
		nullptr,					// defines
		nullptr,					// includes
		p_entry_point_name,
		p_shader_model,
		compile_flags,
		0,							// effect compilation flags
		&p_bytecode.ptr,
		&p_error_blob.ptr
	);

	if (hr != S_OK) {
		std::string error(static_cast<char*>(p_error_blob->GetBufferPointer()), p_error_blob->GetBufferSize());
		throw std::runtime_error(error);
	}

	return p_bytecode;
}

com_ptr<ID3D11Buffer> constant_buffer(ID3D11Device* device, size_t byte_count)
{
	assert(byte_count > 0);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = UINT(byte_count);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	com_ptr<ID3D11Buffer> buffer;
	HRESULT hr = device->CreateBuffer(&desc, nullptr, &buffer.ptr);
	assert(hr == S_OK);

	return buffer;
}

sparki::pixel_format pixel_format(DXGI_FORMAT fmt) noexcept
{
	switch (fmt) {
		default:								return sparki::pixel_format::none;
		
		case DXGI_FORMAT_R32G32B32_FLOAT:		return sparki::pixel_format::rgb_32f;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return sparki::pixel_format::rgba_32f;
		case DXGI_FORMAT_R8_UNORM:				return sparki::pixel_format::red_8;
		case DXGI_FORMAT_R8G8_UNORM:			return sparki::pixel_format::rg_8;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return sparki::pixel_format::rgba_8;
	}
}

texture_data make_texture_data(ID3D11DeviceContext* p_ctx, ID3D11Texture2D* p_tex)
{
	assert(p_ctx);
	assert(p_tex);

	D3D11_TEXTURE2D_DESC desc;
	p_tex->GetDesc(&desc);
	assert(desc.MipLevels == 1); // not implemented

	texture_data td(uint3(desc.Width, desc.Height, 6), rnd::pixel_format(desc.Format));

	for (UINT i = 0; i < desc.ArraySize; ++i) {
		D3D11_MAPPED_SUBRESOURCE map;
		HRESULT hr = p_ctx->Map(p_tex, i, D3D11_MAP_READ, 0, &map);
		assert(hr == S_OK);
		assert(size_t(map.DepthPitch) == square(td.size) * byte_count(td.format));

		void* p = td.buffer.data() + i * map.DepthPitch;
		std::memcpy(p, map.pData, map.DepthPitch);
		p_ctx->Unmap(p_tex, i);
	}

	return td;
}

} // namespace rnd
} // namespace sparki
