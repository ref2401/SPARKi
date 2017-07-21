#include "sparki/rnd/render.h"

#include <cassert>
#include "sparki/utility.h"
#include "ts/task_system.h"


namespace sparki {

// ----- skybox_pass -----

skybox_pass::skybox_pass(ID3D11Device* p_device)
{
	assert(p_device);

	hlsl_shader_desc shader_desc;
	auto load_assets = [&shader_desc] {
		shader_desc = hlsl_shader_desc("../../data/shaders/rnd_cubemap.hlsl");
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
	hr = p_device->CreateSamplerState(&sampler_desc, &p_sampler_.ptr);
	assert(hr == S_OK);
}

void skybox_pass::init_skybox_texture(ID3D11Device* p_device)
{
	assert(p_device);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = 1024;
	desc.MipLevels = 1;
	desc.ArraySize = 6;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	HRESULT hr = p_device->CreateTexture2D(&desc, nullptr, &p_tex_skybox_.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateShaderResourceView(p_tex_skybox_, nullptr, &p_tex_skybox_srv_.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateUnorderedAccessView(p_tex_skybox_, nullptr, &p_tex_skybox_uav_.ptr);
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
	p_skybox_pass_ = std::make_unique<skybox_pass>(p_device_);
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
	image_2d img_equirect;

	auto load_assets = [&gen_cubemap_compute_desc, &img_equirect] {
		gen_cubemap_compute_desc = hlsl_compute_desc("../../data/shaders/equirectangular_to_cube.compute.hlsl");
		img_equirect = image_2d("../../data/WinterForest_Ref.hdr", 4);
	};
	std::atomic_size_t wc;
	ts::run(load_assets, wc);

	ts::wait_for(wc);
	gen_cubemap_compute_ = hlsl_compute(p_device_, gen_cubemap_compute_desc);

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
	HRESULT hr = p_device_->CreateTexture2D(&ed, &e_data, &p_tex_equirect_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_equirect_, nullptr, &p_tex_equirect_srv_.ptr);
	assert(hr == S_OK);

	p_ctx_->CSSetShader(gen_cubemap_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &p_tex_equirect_srv_.ptr);
	ID3D11UnorderedAccessView* uav_list[1] = { p_skybox_pass_->p_tex_skybox_uav() };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);
#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif
	p_ctx_->Dispatch(2, 512, 6);
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	uav_list[0] = nullptr;
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

void renderer::draw_frame(frame& frame)
{
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
