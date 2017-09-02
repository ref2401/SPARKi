#include "sparki/core/rnd.h"

#include <cassert>
#include "sparki/core/utility.h"
#include "ts/task_system.h"


namespace sparki {
namespace core {

// ----- gbuffer -----

gbuffer::gbuffer(ID3D11Device* p_device)
{
	assert(p_device);

	D3D11_RASTERIZER_DESC rastr_desc = {};
	rastr_desc.FillMode = D3D11_FILL_SOLID;
	rastr_desc.CullMode = D3D11_CULL_BACK;
	rastr_desc.FrontCounterClockwise = true;
	HRESULT hr = p_device->CreateRasterizerState(&rastr_desc, &p_rasterizer_state.ptr);
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
	color_desc.Width = width;
	color_desc.Height = height;
	color_desc.MipLevels = 1;
	color_desc.ArraySize = 1;
	color_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	color_desc.SampleDesc.Count = 1;
	color_desc.SampleDesc.Quality = 0;
	color_desc.Usage = D3D11_USAGE_DEFAULT;
	color_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	HRESULT hr = p_device->CreateTexture2D(&color_desc, nullptr, &p_tex_color.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateShaderResourceView(p_tex_color, nullptr, &p_tex_color_srv.ptr);
	assert(hr == S_OK);
	hr = p_device->CreateRenderTargetView(p_tex_color, nullptr, &p_tex_color_rtv.ptr);
	assert(hr == S_OK);

	// update postproc textures & its view
	D3D11_TEXTURE2D_DESC pp_desc = {};
	pp_desc.Width = width;
	pp_desc.Height = height;
	pp_desc.MipLevels = 1;
	pp_desc.ArraySize = 1;
	pp_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	pp_desc.SampleDesc.Count = 1;
	pp_desc.SampleDesc.Quality = 0;
	pp_desc.Usage = D3D11_USAGE_DEFAULT;
	pp_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
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
	ds_desc.Width = width;
	ds_desc.Height = height;
	ds_desc.MipLevels = 1;
	ds_desc.ArraySize = 1;
	ds_desc.Format = DXGI_FORMAT_D32_FLOAT;
	ds_desc.SampleDesc.Count = 1;
	ds_desc.SampleDesc.Quality = 0;
	ds_desc.Usage = D3D11_USAGE_DEFAULT;
	ds_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
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

// ----- shading_pass -----

shading_pass::shading_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	hlsl_shader_desc shader_desc("../../data/shaders/shading_pass.hlsl");
	shader_ = hlsl_shader(p_device_, shader_desc);

	p_cb_vertex_shader_ = make_constant_buffer(p_device_, shading_pass::cb_byte_count);
	init_pipeline_state();
	init_textures();
	init_geometry(); // shader_ must be initialized
}

void shading_pass::init_geometry()
{
	using geometry_t	= mesh_geometry<vertex_attribs::p_n_uv_ts>;
	using fmt_t			= geometry_t::format;

	D3D11_INPUT_ELEMENT_DESC attrib_layout[fmt_t::attrib_count] = {
		{ "VERT_POSITION_MS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, fmt_t::position_byte_offset, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VERT_NORMAL_MS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, fmt_t::normal_byte_offset, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VERT_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, fmt_t::uv_byte_offset, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VERT_TANGENT_SPACE_MS", 0, DXGI_FORMAT_R10G10B10A2_UNORM, 0, fmt_t::tangent_space_byte_offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HRESULT hr = p_device_->CreateInputLayout(attrib_layout, fmt_t::attrib_count,
		shader_.p_vertex_shader_bytecode->GetBufferPointer(),
		shader_.p_vertex_shader_bytecode->GetBufferSize(),
		&p_input_layout_.ptr);
	assert(hr == S_OK);

	//geometry_t geometry = read_geo("../../data/geometry/plane.geo");
	geometry_t geometry = read_from_geo_file("../../data/geometry/sphere.geo");
	//geometry_t geometry = read_geo("../../data/geometry/suzanne.geo");
	D3D11_BUFFER_DESC vb_desc = {};
	vb_desc.ByteWidth = UINT(byte_count(geometry.vertices));
	vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
	vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	const D3D11_SUBRESOURCE_DATA vb_data = { geometry.vertices.data(), 0, 0 };
	hr = p_device_->CreateBuffer(&vb_desc, &vb_data, &p_vertex_buffer_.ptr);
	assert(hr == S_OK);

	D3D11_BUFFER_DESC ib_desc = {};
	ib_desc.ByteWidth = UINT(byte_count(geometry.indices));
	ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
	ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	const D3D11_SUBRESOURCE_DATA ib_data = { geometry.indices.data(), 0, 0 };
	hr = p_device_->CreateBuffer(&ib_desc, &ib_data, &p_index_buffer_.ptr);
	assert(hr == S_OK);

	vertex_stride_ = UINT(fmt_t::vertex_byte_count);
	index_count_ = UINT(geometry.indices.size());
}

void shading_pass::init_pipeline_state()
{
	D3D11_DEPTH_STENCIL_DESC ds_desc = {};
	ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	ds_desc.DepthFunc = D3D11_COMPARISON_LESS;
	ds_desc.DepthEnable = true;
	HRESULT hr = p_device_->CreateDepthStencilState(&ds_desc, &p_depth_stencil_state_.ptr);
	assert(hr == S_OK);
}

void shading_pass::init_textures()
{
	texture_data td_diffuse_envmap = load_from_tex_file("../../data/pisa_diffuse_envmap.tex");
	texture_data td_specular_envmap = load_from_tex_file("../../data/pisa_specular_envmap.tex");
	texture_data td_specular_brdf = load_from_tex_file("../../data/specular_brdf.tex");

	p_tex_diffuse_envmap_ = make_texture_cube(p_device_, td_diffuse_envmap, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_diffuse_envmap_, nullptr, &p_tex_diffuse_envmap_srv_.ptr);
	assert(hr == S_OK);

	p_tex_specular_envmap_ = make_texture_cube(p_device_, td_specular_envmap, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	hr = p_device_->CreateShaderResourceView(p_tex_specular_envmap_, nullptr, &p_tex_specular_envmap_srv_.ptr);
	assert(hr == S_OK);

	p_tex_specular_brdf_ = make_texture_2d(p_device_, td_specular_brdf, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	hr = p_device_->CreateShaderResourceView(p_tex_specular_brdf_, nullptr, &p_tex_specular_brdf_srv_.ptr);
	assert(hr == S_OK);
}

void shading_pass::perform(const gbuffer& gbuffer, const float4x4& pv_matrix, const float3& camera_position)
{
	const float4x4 model_matrix = float4x4::identity;
	const float4x4 normal_matrix = float4x4::identity;
	const float4x4 pvm_matrix = pv_matrix * model_matrix;
	const float4 camera_position_ms = mul(inverse(model_matrix), camera_position);

	float cb_data[shading_pass::cb_component_count];
	to_array_column_major_order(pv_matrix, cb_data);
	to_array_column_major_order(model_matrix, cb_data + 16);
	to_array_column_major_order(normal_matrix, cb_data + 32);
	std::memcpy(cb_data + 48, &camera_position.x, sizeof(float3));
	std::memcpy(cb_data + 52, &camera_position_ms.x, sizeof(float3));
	p_ctx_->UpdateSubresource(p_cb_vertex_shader_, 0, nullptr, cb_data, 0, 0);

	// input layout
	const UINT offset = 0;
	p_ctx_->IASetInputLayout(p_input_layout_);
	p_ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	p_ctx_->IASetVertexBuffers(0, 1, &p_vertex_buffer_.ptr, &vertex_stride_, &offset);
	p_ctx_->IASetIndexBuffer(p_index_buffer_, DXGI_FORMAT_R32_UINT, 0);
	// rasterizer & output merger
	p_ctx_->RSSetState(gbuffer.p_rasterizer_state);
	p_ctx_->OMSetDepthStencilState(p_depth_stencil_state_, 0);
	// shaders
	p_ctx_->VSSetShader(shader_.p_vertex_shader, nullptr, 0);
	p_ctx_->VSSetConstantBuffers(0, 1, &p_cb_vertex_shader_.ptr);
	p_ctx_->PSSetShader(shader_.p_pixel_shader, nullptr, 0);
	constexpr UINT srv_count = 3;
	ID3D11ShaderResourceView* srv_list[srv_count] = { 
		p_tex_diffuse_envmap_srv_, 
		p_tex_specular_envmap_srv_, 
		p_tex_specular_brdf_srv_ 
	};
	p_ctx_->PSSetShaderResources(0, srv_count, srv_list);
	p_ctx_->PSSetSamplers(0, 1, &gbuffer.p_sampler_linear.ptr);

#ifdef SPARKI_DEBUG
	HRESULT hr = p_debug_->ValidateContext(p_ctx_);
	assert(hr == S_OK);
#endif

	p_ctx_->DrawIndexed(index_count_, 0, 0);
}

// ----- skybox_pass -----

skybox_pass::skybox_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	hlsl_shader_desc shader_desc("../../data/shaders/skybox_pass.hlsl");
	shader_ = hlsl_shader(p_device_, shader_desc);

	p_cb_vertex_shader_ = make_constant_buffer(p_device_, sizeof(float4x4));
	init_pipeline_state();
	init_skybox_texture();
}

void skybox_pass::init_pipeline_state()
{
	D3D11_RASTERIZER_DESC rastr_desc = {};
	rastr_desc.FillMode = D3D11_FILL_SOLID;
	rastr_desc.CullMode = D3D11_CULL_FRONT;
	rastr_desc.FrontCounterClockwise = true;
	HRESULT hr = p_device_->CreateRasterizerState(&rastr_desc, &p_rasterizer_state_.ptr);
	assert(hr == S_OK);

	D3D11_DEPTH_STENCIL_DESC ds_desc = {};
	ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	ds_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	ds_desc.DepthEnable = true;
	hr = p_device_->CreateDepthStencilState(&ds_desc, &p_depth_stencil_state_.ptr);
	assert(hr == S_OK);
}

void skybox_pass::init_skybox_texture()
{
	const texture_data td = load_from_tex_file("../../data/pisa_skybox.tex");
	p_tex_skybox_ = make_texture_cube(p_device_, td, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE);
	HRESULT hr = p_device_->CreateShaderResourceView(p_tex_skybox_, nullptr, &p_tex_skybox_srv_.ptr);
	assert(hr == S_OK);
}

void skybox_pass::perform(const gbuffer& gbuffer, const float4x4& pv_matrix, const float3& position)
{
	// update pvm matrix
	const float4x4 pvm_matrix = pv_matrix * translation_matrix(position);
	float data[16];
	to_array_column_major_order(pvm_matrix, data);
	p_ctx_->UpdateSubresource(p_cb_vertex_shader_, 0, nullptr, data, 0, 0);

	// input layout
	p_ctx_->IASetInputLayout(nullptr);
	p_ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	p_ctx_->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	p_ctx_->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	// rasterizer & output merger
	p_ctx_->RSSetState(p_rasterizer_state_);
	p_ctx_->OMSetDepthStencilState(p_depth_stencil_state_, 0);
	// shaders
	p_ctx_->VSSetShader(shader_.p_vertex_shader, nullptr, 0);
	p_ctx_->VSSetConstantBuffers(0, 1, &p_cb_vertex_shader_.ptr);
	p_ctx_->PSSetShader(shader_.p_pixel_shader, nullptr, 0);
	p_ctx_->PSSetShaderResources(0, 1, &p_tex_skybox_srv_.ptr);
	p_ctx_->PSSetSamplers(0, 1, &gbuffer.p_sampler_linear.ptr);

#ifdef SPARKI_DEBUG
	HRESULT hr = p_debug_->ValidateContext(p_ctx_);
	assert(hr == S_OK);
#endif

	p_ctx_->Draw(14, 0); // 14 - number of indices in a cube represented by a triangle strip
}

// ----- postproc_pass -----

postproc_pass::postproc_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device),
	p_ctx_(p_ctx),
	p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	const hlsl_compute_desc tn_desc("../../data/shaders/tone_mapping_pass.compute.hlsl");
	tone_mapping_compute_ = hlsl_compute(p_device, tn_desc);
	const hlsl_compute_desc fxaa_desc("../../data/shaders/fxaa_pass.compute.hlsl");
	fxaa_compute_ = hlsl_compute(p_device, fxaa_desc);
	const hlsl_compute_desc downsample_desc("../../data/shaders/downsample.compute.hlsl");
	downsample_compute_ = hlsl_compute(p_device, downsample_desc);
}

void postproc_pass::perform(const gbuffer& gbuffer, ID3D11UnorderedAccessView* p_tex_window_uav)
{
	assert(p_tex_window_uav);

	HRESULT hr;
	const UINT rnd_gx = UINT(gbuffer.rnd_viewport.Width) / postproc_pass::postproc_compute_group_x_size
		+ ((std::fmod(gbuffer.rnd_viewport.Width, postproc_pass::postproc_compute_group_x_size) > 0) ? 1 : 0);
	const UINT rnd_gy = UINT(gbuffer.rnd_viewport.Height) / postproc_pass::postproc_compute_group_y_size
		+ ((std::fmod(gbuffer.rnd_viewport.Height, postproc_pass::postproc_compute_group_y_size) > 0) ? 1 : 0);

	// ---- tone mapping pass -----
	p_ctx_->CSSetShader(tone_mapping_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &gbuffer.p_tex_color_srv.ptr);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &gbuffer.p_tex_tone_mapping_uav.ptr, nullptr);
#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif
	p_ctx_->Dispatch(rnd_gx, rnd_gy, 1);

	// ---- anti-aliasing pass -----
	p_ctx_->CSSetShader(fxaa_compute_.p_compute_shader, nullptr, 0);
	ID3D11SamplerState* sampler_list[2] = { gbuffer.p_sampler_linear, gbuffer.p_sampler_point };
	p_ctx_->CSSetSamplers(0, 2, sampler_list);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &gbuffer.p_tex_aa_uav.ptr, nullptr);
	p_ctx_->CSSetShaderResources(0, 1, &gbuffer.p_tex_tone_mapping_srv.ptr);
#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif
	p_ctx_->Dispatch(rnd_gx, rnd_gy, 1);

	// ---- downsample pass -----
	const UINT wnd_gx = UINT(gbuffer.window_viewport.Width) / postproc_pass::downsample_compute_group_x_size
		+ ((std::fmod(gbuffer.window_viewport.Width, postproc_pass::downsample_compute_group_x_size) > 0) ? 1 : 0);
	const UINT wnd_gy = UINT(gbuffer.window_viewport.Height) / postproc_pass::downsample_compute_group_y_size
		+ ((std::fmod(gbuffer.window_viewport.Height, postproc_pass::downsample_compute_group_y_size) > 0) ? 1 : 0);

	p_ctx_->CSSetShader(downsample_compute_.p_compute_shader, nullptr, 0);
	p_ctx_->CSSetSamplers(0, 1, &gbuffer.p_sampler_linear.ptr);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_tex_window_uav, nullptr);
	p_ctx_->CSSetShaderResources(0, 1, &gbuffer.p_tex_aa_srv.ptr);
#ifdef SPARKI_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif
	p_ctx_->Dispatch(wnd_gx, wnd_gy, 1);

	// clear pipeline state
	p_ctx_->CSSetShader(nullptr, nullptr, 0);
	// reset sampler
	sampler_list[0] = { nullptr };
	sampler_list[1] = { nullptr };
	p_ctx_->CSSetSamplers(0, 2, sampler_list);
	// reset srv binding
	ID3D11ShaderResourceView* srv_list[1] = { nullptr };
	p_ctx_->CSSetShaderResources(0, 1, srv_list);
	// reset uav binding
	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);
}

// ----- renderer -----

renderer::renderer(HWND p_hwnd, const uint2& viewport_size)
{
	assert(p_hwnd);
	assert(viewport_size > 0);

	init_dx_device(p_hwnd, viewport_size);
	p_gbuffer_ = std::make_unique<gbuffer>(p_device_);

	std::atomic_size_t wc;
	ts::run([this] { init_passes_and_tools(); }, wc);

	resize_viewport(viewport_size);
	
	ts::wait_for(wc);
	//envmap_builder.perform("../../data/pisa.hdr", "../../data/pisa_skybox.tex",
	//	"../../data/pisa_diffuse_envmap.tex", "../../data/pisa_specular_envmap.tex");
	//bi.perform("../../data/specular_brdf.tex");
}

renderer::~renderer() noexcept
{
#ifdef SPARKI_DEBUG
	p_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

void renderer::init_dx_device(HWND p_hwnd, const uint2& viewport_size)
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

void renderer::init_passes_and_tools()
{
	assert(p_gbuffer_);

	// rnd tools
	p_envmap_builder_ = std::make_unique<envmap_texture_builder>(p_device_, p_ctx_, p_debug_, p_gbuffer_->p_sampler_linear);
	p_brdf_integrator_ = std::make_unique<brdf_integrator>(p_device_, p_ctx_, p_debug_);
	// rnd passes
	p_skybox_pass_ = std::make_unique<skybox_pass>(p_device_, p_ctx_, p_debug_);
	p_light_pass_ = std::make_unique<shading_pass>(p_device_, p_ctx_, p_debug_);
	p_postproc_pass_ = std::make_unique<postproc_pass>(p_device_, p_ctx_, p_debug_);
}

void renderer::draw_frame(frame& frame)
{
	const float4x4 view_matrix = math::view_matrix(frame.camera_position, frame.camera_target, frame.camera_up);
	const float4x4 pv_matrix = frame.projection_matrix * view_matrix;

	// ----- rnd passes -----
	//
	p_ctx_->RSSetViewports(1, &p_gbuffer_->rnd_viewport);
	p_ctx_->OMSetRenderTargets(1, &p_gbuffer_->p_tex_color_rtv.ptr, p_gbuffer_->p_tex_depth_dsv);
	p_ctx_->ClearRenderTargetView(p_gbuffer_->p_tex_color_rtv, &float4::zero.x);
	p_ctx_->ClearDepthStencilView(p_gbuffer_->p_tex_depth_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	p_light_pass_->perform(*p_gbuffer_, pv_matrix, frame.camera_position);
	p_skybox_pass_->perform(*p_gbuffer_, pv_matrix, frame.camera_position);

	// reset rtv bindings
	ID3D11RenderTargetView* rtv_list[1] = { nullptr };
	p_ctx_->OMSetRenderTargets(1, rtv_list, nullptr);

	// ----- post processing -----
	//
	p_postproc_pass_->perform(*p_gbuffer_, p_tex_window_uav_);

	// ----- present frame -----
	//
	p_swap_chain_->Present(0, 0);
}

void renderer::resize_viewport(const uint2& size)
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