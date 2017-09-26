#include "sparki/core/rnd.h"

#include <cassert>
#include "sparki/core/utility.h"
#include "ts/task_system.h"


namespace sparki {
namespace core {

// ----- renderer -----

render_system::render_system(HWND p_hwnd, const uint2& viewport_size)
{
	assert(p_hwnd);
	assert(viewport_size > 0);

	init_dx_device(p_hwnd, viewport_size);
	p_gbuffer_ = std::make_unique<gbuffer>(p_device_);

	std::atomic_size_t wc;
	ts::run([this] { init_passes_and_tools(); }, wc);

	resize_viewport(viewport_size);
	
	ts::wait_for(wc);
}

render_system::~render_system() noexcept
{
#ifdef SPARKI_DEBUG
	p_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

void render_system::init_dx_device(HWND p_hwnd, const uint2& viewport_size)
{
	DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
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
		D3D11_CREATE_DEVICE_DEBUG,  &expected_feature_level, 1, D3D11_SDK_VERSION, &swap_chain_desc,
		&p_swap_chain_.ptr,
		&p_device_.ptr,
		&actual_feature_level,
		&p_ctx_.ptr);
	assert(hr == S_OK);
	ENFORCE(actual_feature_level == expected_feature_level, "Failed to create a device with feature level: D3D_FEATURE_LEVEL_11_0");

	D3D11_FEATURE_DATA_THREADING threading_feature;
	p_device_->CheckFeatureSupport(D3D11_FEATURE_THREADING, 
		&threading_feature, 
		sizeof(D3D11_FEATURE_DATA_THREADING));
	ENFORCE(threading_feature.DriverConcurrentCreates, "Your GPU must support D3D11_FEATURE_DATA_THREADING feature.");

#ifdef SPARKI_DEBUG
	hr = p_device_->QueryInterface<ID3D11Debug>(&p_debug_.ptr);
	assert(hr == S_OK);
#endif
}

void render_system::init_passes_and_tools()
{
	assert(p_gbuffer_);

	// rnd tools
	p_brdf_integrator_		= std::make_unique<brdf_integrator>(p_device_, p_ctx_, p_debug_);
	p_envmap_builder_		= std::make_unique<envmap_texture_builder>(p_device_, p_ctx_, 
		p_debug_, p_gbuffer_->p_sampler_linear);
	p_material_editor_tool_ = std::make_unique<core::material_editor_tool>(p_device_, p_ctx_, 
		p_debug_, p_gbuffer_->p_sampler_point);
	// rnd passes
	p_skybox_pass_		= std::make_unique<skybox_pass>(p_device_, p_ctx_, p_debug_);
	p_light_pass_		= std::make_unique<shading_pass>(p_device_, p_ctx_, p_debug_);
	p_postproc_pass_	= std::make_unique<postproc_pass>(p_device_, p_ctx_, p_debug_);
	p_imgui_pass_		= std::make_unique<imgui_pass>(p_device_, p_ctx_, p_debug_);
}

void render_system::draw_frame(frame& frame)
{
	const float4x4 view_matrix = math::view_matrix(frame.camera_position, frame.camera_target, frame.camera_up);
	const float4x4 pv_matrix = frame.projection_matrix * view_matrix;

	// rnd passes ---
	p_ctx_->RSSetViewports(1, &p_gbuffer_->rnd_viewport);
	p_ctx_->OMSetBlendState(p_gbuffer_->p_blend_state_no_blend, &float4::zero.x, 0xffffffff);
	p_ctx_->OMSetRenderTargets(1, &p_gbuffer_->p_tex_color_rtv.ptr, p_gbuffer_->p_tex_depth_dsv);
	p_ctx_->ClearRenderTargetView(p_gbuffer_->p_tex_color_rtv, &float4::zero.x);
	p_ctx_->ClearDepthStencilView(p_gbuffer_->p_tex_depth_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	p_light_pass_->perform(*p_gbuffer_, pv_matrix, frame.material, frame.camera_position);
	p_skybox_pass_->perform(*p_gbuffer_, pv_matrix, frame.camera_position);

	// reset rtv bindings
	ID3D11RenderTargetView* rtv_list[1] = { nullptr };
	p_ctx_->OMSetRenderTargets(1, rtv_list, nullptr);

	// post processing ---
	p_postproc_pass_->perform(*p_gbuffer_, p_tex_window_uav_);

	// imgui rendering ---
	if (frame.p_imgui_draw_data) {
		p_ctx_->OMSetRenderTargets(1, &p_tex_window_rtv_.ptr, nullptr);
		p_imgui_pass_->perform(frame.p_imgui_draw_data, p_gbuffer_->p_sampler_linear);
	}

	// present frame ---
	p_swap_chain_->Present(1, 0);

	constexpr UINT c_srv_count = 7;
	ID3D11ShaderResourceView* srv_list[c_srv_count] = { nullptr, nullptr, nullptr, 
		nullptr, nullptr, nullptr, nullptr };
	p_ctx_->VSSetShaderResources(0, c_srv_count, srv_list);
	p_ctx_->PSSetShaderResources(0, c_srv_count, srv_list);
}

void render_system::resize_viewport(const uint2& size)
{
	assert(size > 0);

	const float w = float(size.x);
	const float h = float(size.y);
	if (approx_equal(w, p_gbuffer_->window_viewport.Width) 
		&& approx_equal(h, p_gbuffer_->window_viewport.Height)) return;

	// resize swap chain's buffers
	if (p_tex_window_rtv_) {
		p_ctx_->OMSetRenderTargets(0, nullptr, nullptr);
		p_tex_window_.dispose();
		p_tex_window_rtv_.dispose();
		p_tex_window_uav_.dispose();

		DXGI_SWAP_CHAIN_DESC d;
		HRESULT hr = p_swap_chain_->GetDesc(&d);
		assert(hr == S_OK);
		hr = p_swap_chain_->ResizeBuffers(d.BufferCount, size.x, size.y, d.BufferDesc.Format, d.Flags);
		assert(hr == S_OK);
	}

	// update window rtv
	HRESULT hr = p_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&p_tex_window_.ptr));
	assert(hr == S_OK);
	D3D11_TEXTURE2D_DESC d;
	p_tex_window_->GetDesc(&d);
	hr = p_device_->CreateRenderTargetView(p_tex_window_.ptr, nullptr, &p_tex_window_rtv_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateUnorderedAccessView(p_tex_window_.ptr, nullptr, &p_tex_window_uav_.ptr);
	assert(hr == S_OK);
	p_gbuffer_->resize(p_device_, size);
}

} // namespace core
} // namespace sparki
