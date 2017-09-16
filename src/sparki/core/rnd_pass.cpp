#include "sparki/core/rnd_pass.h"


namespace sparki {
namespace core {

// ----- shading_pass -----

shading_pass::shading_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	hlsl_shader_desc shader_desc("../../data/shaders/shading_pass.hlsl");
	shader_ = hlsl_shader(p_device_, shader_desc);

	p_cb_vertex_shader_ = make_constant_buffer(p_device_, cb_vertex_shader_component_count * sizeof(float));
	init_pipeline_state();
	init_textures();
	init_geometry(); // shader_ must be initialized
}

void shading_pass::init_geometry()
{
	using geometry_t = mesh_geometry<vertex_attribs::p_n_uv_ts>;
	using fmt_t = geometry_t::format;

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

void shading_pass::perform(const gbuffer& gbuffer, const float4x4& pv_matrix,
	const material& material, const float3& camera_position)
{
	const float4x4 model_matrix = float4x4::identity;
	const float4x4 normal_matrix = float4x4::identity;
	const float4x4 pvm_matrix = pv_matrix * model_matrix;
	const float4 camera_position_ms = mul(inverse(model_matrix), camera_position);

	{ // update vertex shader constant buffer
		float cb_data[shading_pass::cb_vertex_shader_component_count];
		to_array_column_major_order(pv_matrix, cb_data);
		to_array_column_major_order(model_matrix, cb_data + 16);
		to_array_column_major_order(normal_matrix, cb_data + 32);
		std::memcpy(cb_data + 48, &camera_position.x, sizeof(float3));
		std::memcpy(cb_data + 52, &camera_position_ms.x, sizeof(float3));
		p_ctx_->UpdateSubresource(p_cb_vertex_shader_, 0, nullptr, cb_data, 0, 0);
	}

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
	constexpr UINT srv_count = 6;
	ID3D11ShaderResourceView* srv_list[srv_count] = {
		p_tex_diffuse_envmap_srv_,
		p_tex_specular_envmap_srv_,
		p_tex_specular_brdf_srv_,
		material.p_tex_base_color_srv,
		material.p_tex_reflect_color_srv,
		material.p_tex_properties_srv
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

} // namespace core
} // namespace sparki
