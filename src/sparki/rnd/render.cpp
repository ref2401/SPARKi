#include "sparki/rnd/render.h"

#include <cassert>
#include "sparki/utility.h"
#include "ts/task_system.h"


namespace sparki {

// ----- hlsl_shader -----

hlsl_shader::hlsl_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc)
{
	assert(p_device);

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
		std::string exc_msg = EXCEPTION_MSG("Vertex shader creation error.");
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
		std::string exc_msg = EXCEPTION_MSG("Hull shader creation error.");
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
		std::string exc_msg = EXCEPTION_MSG("Domain shader creation error.");
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
		std::string exc_msg = EXCEPTION_MSG("Pixel shader creation error.");
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

// ----- renderer -----

renderer::renderer(HWND p_hwnd, const uint2& viewport_size)
{
	assert(p_hwnd);
	assert(viewport_size > 0);

	init_device(p_hwnd, viewport_size);
	resize_viewport(viewport_size);
	load_assets();
}

renderer::~renderer() noexcept
{
#ifdef SPARKI_DEBUG
	p_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

void renderer::init_device(HWND p_hwnd, const uint2& viewport_size)
{
	DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferDesc.Width = viewport_size.x;
	swap_chain_desc.BufferDesc.Height = viewport_size.y;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_chain_desc.Windowed = true;
	swap_chain_desc.OutputWindow = p_hwnd;

	const D3D_FEATURE_LEVEL expected_feature_level = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL actual_feature_level;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		D3D11_CREATE_DEVICE_DEBUG, &expected_feature_level, 1, D3D11_SDK_VERSION, &swap_chain_desc,
		&p_swap_chain_.ptr, 
		&p_device_.ptr, 
		&actual_feature_level, 
		&p_ctx_.ptr);
	assert(hr == S_OK);
	ENFORCE(actual_feature_level == expected_feature_level, "Failed to create a device with feature level: D3D_FEATURE_LEVEL_11_0");

#ifdef SPARKI_DEBUG
	hr = p_device_->QueryInterface<ID3D11Debug>(&p_debug_.ptr);
	assert(hr == S_OK);
#endif
}

void renderer::draw_frame()
{
	p_ctx_->RSSetViewports(1, &viewport_);
	p_ctx_->OMSetRenderTargets(1, &p_tex_window_rtv_.ptr, nullptr);
	p_ctx_->ClearRenderTargetView(p_tex_window_rtv_, &float4::unit_xyzw.x);

	p_swap_chain_->Present(0, 0);
}

void renderer::load_assets()
{
	hlsl_shader_desc simple_shader_desc;

	std::atomic_size_t wait_counter;
	ts::task_desc task_load_hlsl = ts::task_desc([&simple_shader_desc] {
		simple_shader_desc = hlsl_shader_desc("../../data/gen_cube_envmap.hlsl");
	});

	ts::run(task_load_hlsl, wait_counter);

	// run images & geometry loading tasks

	ts::wait_for(wait_counter);
	hlsl_shader s(p_device_, simple_shader_desc);
}

void renderer::resize_viewport(const uint2& size)
{
	assert(size > 0);

	const float w = float(size.x);
	const float h = float(size.y);

	if (approx_equal(w, viewport_.Width) && approx_equal(h, viewport_.Height)) return;

	// resize swap chain's buffers
	if (p_tex_window_rtv_) {
		p_ctx_->OMSetRenderTargets(0, nullptr, nullptr);
		p_tex_window_rtv_.dispose();

		//DXGI_SWAP_CHAIN_DESC d;
		//HRESULT hr = p_swap_chain_->GetDesc(&d);
		//assert(hr == S_OK);
		//hr = p_swap_chain_->ResizeBuffers(d.BufferCount, size.x, size.y, d.BufferDesc.Format, d.Flags);
		//assert(hr == S_OK);
	}

	// update window rtv
	com_ptr<ID3D11Texture2D> tex_back_buffer;
	HRESULT hr = p_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&tex_back_buffer.ptr));
	assert(hr == S_OK);

	hr = p_device_->CreateRenderTargetView(tex_back_buffer.ptr, nullptr, &p_tex_window_rtv_.ptr);
	assert(hr == S_OK);

	// update viewport
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.Width = float(size.x);
	viewport_.Height = float(size.y);
	viewport_.MinDepth = 0.0;
	viewport_.MaxDepth = 1.0f;
}

} // namespace sparki
