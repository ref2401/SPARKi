#include "sparki/rnd/rnd.h"

#include <cassert>
#include "sparki/utility.h"
#include "ts/task_system.h"


namespace sparki {
namespace rnd {

// ----- skybox_pass -----

skybox_pass::skybox_pass(ID3D11Device* p_device)
{
	assert(p_device);

	hlsl_shader_desc shader_desc;
	auto load_assets = [&shader_desc] {
		shader_desc = hlsl_shader_desc("../../data/shaders/rnd_skybox.hlsl");
	};

	std::atomic_size_t wc;
	ts::run(load_assets, wc);

	init_pipeline_state(p_device);
	p_cb_vertex_shader_ = constant_buffer(p_device, sizeof(float4x4));

	ts::wait_for(wc);
	shader_ = hlsl_shader(p_device, shader_desc);
	init_skybox_texture(p_device);
}

void skybox_pass::init_pipeline_state(ID3D11Device* p_device)
{
	assert(p_device);

	D3D11_RASTERIZER_DESC rastr_desc = {};
	rastr_desc.FillMode = D3D11_FILL_SOLID;
	rastr_desc.CullMode = D3D11_CULL_FRONT;
	rastr_desc.FrontCounterClockwise = true;
	HRESULT hr = p_device->CreateRasterizerState(&rastr_desc, &p_rasterizer_state_.ptr);
	assert(hr == S_OK);

	D3D11_DEPTH_STENCIL_DESC ds_desc = {};
	ds_desc.DepthEnable = false;
	hr = p_device->CreateDepthStencilState(&ds_desc, &p_depth_stencil_state_.ptr);
	assert(hr == S_OK);

	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = p_device->CreateSamplerState(&sampler_desc, &p_sampler_.ptr);
	assert(hr == S_OK);
}

void skybox_pass::init_skybox_texture(ID3D11Device* p_device)
{
	assert(p_device);

	const texture_data td = read_tex("../../data/winter_forest_envmap.tex");
	p_tex_skybox_ = texture2d(p_device, td, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	HRESULT hr = p_device->CreateShaderResourceView(p_tex_skybox_, nullptr, &p_tex_skybox_srv_.ptr);
	assert(hr == S_OK);
}

void skybox_pass::perform(ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug,
	const float4x4& pv_matrix, const float3& position)
{
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	// update pvm matrix
	const float4x4 pvm_matrix = pv_matrix * translation_matrix(position);
	p_ctx->UpdateSubresource(p_cb_vertex_shader_, 0, nullptr, &pvm_matrix.m00, 0, 0);

	// input layout
	p_ctx->IASetInputLayout(nullptr);
	p_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	p_ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	p_ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	// rasterizer & output merger
	p_ctx->RSSetState(p_rasterizer_state_);
	p_ctx->OMSetDepthStencilState(p_depth_stencil_state_, 0);
	// shaders
	p_ctx->VSSetShader(shader_.p_vertex_shader, nullptr, 0);
	p_ctx->VSSetConstantBuffers(0, 1, &p_cb_vertex_shader_.ptr);
	p_ctx->PSSetShader(shader_.p_pixel_shader, nullptr, 0);
	p_ctx->PSSetShaderResources(0, 1, &p_tex_skybox_srv_.ptr);
	p_ctx->PSSetSamplers(0, 1, &p_sampler_.ptr);

#ifdef SPARKI_DEBUG
	HRESULT hr = p_debug->ValidateContext(p_ctx);
	assert(hr == S_OK);
#endif

	p_ctx->Draw(14, 0); // 14 - number of indices in a cube represented by a triangle strip
}

// ----- renderer -----

renderer::renderer(HWND p_hwnd, const uint2& viewport_size)
{
	assert(p_hwnd);
	assert(viewport_size > 0);

	init_device(p_hwnd, viewport_size);
	init_assets();
	resize_viewport(viewport_size);
}

renderer::~renderer() noexcept
{
#ifdef SPARKI_DEBUG
	p_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

std::unique_ptr<ibl_texture_builder> p_bbb;

void renderer::init_assets()
{
	// ts:: load cubemap pass assets: hlsl, tex
	// ts:: load equirectangular to cubemap pass stuff.
	// create cubemap pass
	// create equrectangular to cubemap pass
	const hlsl_compute_desc hlsl_equirect_to_skybox("../../data/shaders/equirect_to_skybox.compute.hlsl");
	const hlsl_compute_desc hlsl_filter_envmap("../../data/shaders/prefilter_envmap.compute.hlsl");

	p_bbb = std::make_unique<ibl_texture_builder>(p_device_, p_ctx_, p_debug_, hlsl_equirect_to_skybox, hlsl_filter_envmap);
	p_bbb->perform("../../data/WinterForest_Ref.hdr",
		"../../data/winter_forest_skybox.tex", 1024,
		"../../data/winter_forest_envmap.tex", 256);

	p_skybox_pass_ = std::make_unique<skybox_pass>(p_device_);
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

void renderer::draw_frame(frame& frame)
{
	//p_bbb->perform("../../data/WinterForest_Ref.hdr",
	//	"../../data/winter_forest_skybox.tex", 1024,
	//	"../../data/winter_forest_envmap.tex", 256);

	const float4x4 view_matrix = math::view_matrix(frame.camera_position, frame.camera_target, frame.camera_up);
	const float4x4 pv_matrix = frame.projection_matrix * view_matrix;

	p_ctx_->RSSetViewports(1, &viewport_);
	p_ctx_->OMSetRenderTargets(1, &p_tex_window_rtv_.ptr, nullptr);
	p_ctx_->ClearRenderTargetView(p_tex_window_rtv_, &float4::unit_xyzw.x);

	p_skybox_pass_->perform(p_ctx_, p_debug_, pv_matrix, frame.camera_position);

	p_swap_chain_->Present(0, 0);
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

		DXGI_SWAP_CHAIN_DESC d;
		HRESULT hr = p_swap_chain_->GetDesc(&d);
		assert(hr == S_OK);
		hr = p_swap_chain_->ResizeBuffers(d.BufferCount, size.x, size.y, d.BufferDesc.Format, d.Flags);
		assert(hr == S_OK);
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

} // namespace rnd
} // namespace sparki
