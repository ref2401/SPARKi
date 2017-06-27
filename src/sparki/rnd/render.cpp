#include "sparki/rnd/render.h"

#include <cassert>
#include "sparki/utility.h"


namespace sparki {

// ----- renderer -----

renderer::renderer(HWND p_hwnd, const uint2& viewport_size)
{
	assert(p_hwnd);
	assert(viewport_size > 0);

	init_device(p_hwnd, viewport_size);
	resize_viewport(viewport_size);
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
		&p_swap_chain_.p_handle, 
		&p_device_.p_handle, 
		&actual_feature_level, 
		&p_ctx_.p_handle);
	assert(hr == S_OK);
	ENFORCE(actual_feature_level == expected_feature_level, "Failed to create a device with feature level: D3D_FEATURE_LEVEL_11_0");

#ifdef SPARKI_DEBUG
	hr = p_device_->QueryInterface<ID3D11Debug>(&p_debug_.p_handle);
	assert(hr == S_OK);
#endif
}

void renderer::draw_frame()
{
	p_ctx_->RSSetViewports(1, &viewport_);
	p_ctx_->OMSetRenderTargets(1, &p_tex_window_rtv_.p_handle, nullptr);
	p_ctx_->ClearRenderTargetView(p_tex_window_rtv_, &float4::unit_xyzw.x);

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

		//DXGI_SWAP_CHAIN_DESC d;
		//HRESULT hr = p_swap_chain_->GetDesc(&d);
		//assert(hr == S_OK);
		//hr = p_swap_chain_->ResizeBuffers(d.BufferCount, size.x, size.y, d.BufferDesc.Format, d.Flags);
		//assert(hr == S_OK);
	}

	// update window rtv
	com_ptr<ID3D11Texture2D> tex_back_buffer;
	HRESULT hr = p_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&tex_back_buffer.p_handle));
	assert(hr == S_OK);

	hr = p_device_->CreateRenderTargetView(tex_back_buffer.p_handle, nullptr, &p_tex_window_rtv_.p_handle);
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
