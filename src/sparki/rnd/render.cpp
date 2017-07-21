#include "sparki/rnd/render.h"

#include <cassert>
#include "sparki/utility.h"
#include "ts/task_system.h"


namespace sparki {

// ----- renderer -----

renderer::renderer(HWND p_hwnd, const uint2& viewport_size)
{
	assert(p_hwnd);
	assert(viewport_size > 0);

	init_device(p_hwnd, viewport_size);
	resize_viewport(viewport_size);
	init_assets();
}

renderer::~renderer() noexcept
{
#ifdef SPARKI_DEBUG
	p_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

void renderer::init_assets()
{
	// ts:: load cubemap pass assets: hlsl, tex
	// ts:: load equirectangular to cubemap pass stuff.
	// create cubemap pass
	// create equrectangular to cubemap pass

	hlsl_compute_desc gen_cubemap_compute_desc;
	hlsl_shader_desc rnd_cubemap_shader_desc;
	image_2d img_equirect;

	auto load_assets = [&gen_cubemap_compute_desc, &rnd_cubemap_shader_desc, &img_equirect] {
		gen_cubemap_compute_desc = hlsl_compute_desc("../../data/shaders/equirectangular_to_cube.compute.hlsl");
		rnd_cubemap_shader_desc = hlsl_shader_desc("../../data/shaders/rnd_cubemap.hlsl");
		img_equirect = image_2d("../../data/WinterForest_Ref.hdr", 4);
	};
	std::atomic_size_t wait_counter;
	ts::run(load_assets, wait_counter);

	init_tex_cube();
	p_cb_vertex_shader_ = constant_buffer(p_device_, sizeof(float4x4));
	D3D11_RASTERIZER_DESC rs_desc = {};
	rs_desc.FillMode = D3D11_FILL_SOLID;
	rs_desc.CullMode = D3D11_CULL_FRONT;
	rs_desc.FrontCounterClockwise = true;
	HRESULT hr = p_device_->CreateRasterizerState(&rs_desc, &p_rastr_state_.ptr);
	assert(hr == S_OK);
	D3D11_DEPTH_STENCIL_DESC ds_desc = {};
	ds_desc.DepthEnable = false;
	hr = p_device_->CreateDepthStencilState(&ds_desc, &p_depth_stencil_state_.ptr);
	assert(hr == S_OK);
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	hr = p_device_->CreateSamplerState(&sampler_desc, &p_sampler_state_.ptr);
	assert(hr == S_OK);

	ts::wait_for(wait_counter);
	gen_cubemap_compute_ = hlsl_compute(p_device_, gen_cubemap_compute_desc);
	rnd_cubemap_shader_ = hlsl_shader(p_device_, rnd_cubemap_shader_desc);

	D3D11_TEXTURE2D_DESC ed = {};
	ed.Width = UINT(img_equirect.size.x);
	ed.Height = UINT(img_equirect.size.y);
	ed.MipLevels = 1;
	ed.ArraySize = 1;
	ed.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	ed.SampleDesc.Count = 1;
	ed.SampleDesc.Quality = 0;
	ed.Usage = D3D11_USAGE_IMMUTABLE;
	ed.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA e_data = {};
	e_data.pSysMem = img_equirect.p_data;
	e_data.SysMemPitch = UINT(img_equirect.size.x * byte_count(img_equirect.pixel_format));
	hr = p_device_->CreateTexture2D(&ed, &e_data, &p_tex_equirect_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_equirect_, nullptr, &p_tex_equirect_srv_.ptr);
	assert(hr == S_OK);

	p_ctx_->CSSetShader(gen_cubemap_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_equirect_srv_.ptr);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_tex_skybox_uav_.ptr, nullptr);
#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif
	p_ctx_->Dispatch(2, 512, 6);
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);
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

void renderer::init_tex_cube()
{
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = td.Height = 1024;
	td.MipLevels = 1;
	td.ArraySize = 6;
	td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	HRESULT hr = p_device_->CreateTexture2D(&td, nullptr, &p_tex_skybox_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_skybox_, nullptr, &p_tex_skybox_srv_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateUnorderedAccessView(p_tex_skybox_, nullptr, &p_tex_skybox_uav_.ptr);
	assert(hr == S_OK);
}

void renderer::draw_frame(frame& frame)
{
	const float4x4 view_matrix = math::view_matrix(frame.camera_position, frame.camera_target, frame.camera_up);
	const float4x4 proj_view_matrix = frame.projection_matrix * view_matrix * translation_matrix(frame.camera_position);

	p_ctx_->UpdateSubresource(p_cb_vertex_shader_, 0, nullptr, &proj_view_matrix.m00, 0, 0);

	p_ctx_->RSSetViewports(1, &viewport_);
	p_ctx_->OMSetRenderTargets(1, &p_tex_window_rtv_.ptr, nullptr);
	p_ctx_->ClearRenderTargetView(p_tex_window_rtv_, &float4::unit_xyzw.x);

	// input layout
	p_ctx_->IASetInputLayout(nullptr);
	p_ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	p_ctx_->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	p_ctx_->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	// rastr & output merger
	p_ctx_->RSSetState(p_rastr_state_);
	p_ctx_->OMSetDepthStencilState(p_depth_stencil_state_, 0);
	// shaders
	p_ctx_->VSSetShader(rnd_cubemap_shader_.p_vertex_shader, nullptr, 0);
	p_ctx_->VSSetConstantBuffers(0, 1, &p_cb_vertex_shader_.ptr);
	p_ctx_->PSSetShader(rnd_cubemap_shader_.p_pixel_shader, nullptr, 0);
	p_ctx_->PSSetShaderResources(0, 1, &p_tex_skybox_srv_.ptr);
	p_ctx_->PSSetSamplers(0, 1, &p_sampler_state_.ptr);

#ifdef SPARKI_DEBUG
	HRESULT hr = p_debug_->ValidateContext(p_ctx_);
	assert(hr == S_OK);
#endif

	p_ctx_->Draw(renderer::cube_index_count, 0);

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
