#pragma once

#include <memory>
#include "sparki/rnd/rnd_base.h"
#include "sparki/rnd/rnd_tool.h"


namespace sparki {
namespace rnd {

struct frame final {
	math::float4x4	projection_matrix;
	float3 camera_position;
	float3 camera_target;
	float3 camera_up;
};

struct gbuffer final {

	explicit gbuffer(ID3D11Device* p_device);

	gbuffer(gbuffer&&) = delete;
	gbuffer& operator=(gbuffer&&) = delete;


	void resize(ID3D11Device* p_device, const uint2 size);


	// color texture
	com_ptr<ID3D11Texture2D>			p_tex_color;
	com_ptr<ID3D11ShaderResourceView>	p_tex_color_srv;
	com_ptr<ID3D11RenderTargetView>		p_tex_color_rtv;
	// depth texture
	com_ptr<ID3D11Texture2D>			p_tex_depth;
	com_ptr<ID3D11DepthStencilView>		p_tex_depth_dsv;
	
	// other stuff:

	// Common rasterizer state. FrontCounterClockwise, CULL_BACK
	com_ptr<ID3D11RasterizerState>		p_rasterizer_state;
	// Sampler that is used all over the place.
	// MIN_MAG_MIP_LINEAR, ADDRESS_CLAMP, LOD in [0, D3D11_FLOAT32_MAX]
	com_ptr<ID3D11SamplerState>			p_sampler;
	D3D11_VIEWPORT						viewport = { 0, 0, 0, 0, 0, 1 };
};

class final_pass final {
public:

	final_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	final_pass(final_pass&&) = delete;
	final_pass& operator=(final_pass&&) = delete;


	void perform(const gbuffer& gbuffer);

private:

	ID3D11Device*						p_device_;
	ID3D11DeviceContext*				p_ctx_;
	ID3D11Debug*						p_debug_;
	hlsl_shader							shader_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
};

class shading_pass final {
public:

	shading_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	shading_pass(shading_pass&&) = delete;
	shading_pass& operator=(shading_pass&&) = delete;

	
	void perform(const gbuffer& gbuffer, const float4x4& pv_matrix, const float3& camera_position);

private:

	static constexpr size_t cb_component_count = 3 * 16 + 2 * 4;
	static constexpr size_t cb_byte_count = cb_component_count * sizeof(float);


	void init_geometry();

	void init_pipeline_state();

	void init_textures(const texture_data_new& td_envmap, const texture_data_new& td_brdf);


	ID3D11Device*						p_device_;
	ID3D11DeviceContext*				p_ctx_;
	ID3D11Debug*						p_debug_;
	hlsl_shader							shader_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
	com_ptr<ID3D11Buffer>				p_cb_vertex_shader_;
	com_ptr<ID3D11Texture2D>			p_tex_envmap_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_envmap_srv_;
	com_ptr<ID3D11Texture2D>			p_tex_brdf_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_brdf_srv_;
	// temporary
	UINT								vertex_stride_;
	UINT								index_count_;
	com_ptr<ID3D11InputLayout>			p_input_layout_;
	com_ptr<ID3D11Buffer>				p_vertex_buffer_;
	com_ptr<ID3D11Buffer>				p_index_buffer_;
};

class skybox_pass final {
public:

	skybox_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	skybox_pass(skybox_pass&&) = delete;
	skybox_pass& operator=(skybox_pass&&) = delete;


	void perform(const gbuffer& gbuffer, const float4x4& pv_matrix, const float3& position);

private:

	void init_pipeline_state();

	void init_skybox_texture();

	ID3D11Device*						p_device_;
	ID3D11DeviceContext*				p_ctx_;
	ID3D11Debug*						p_debug_;
	hlsl_shader							shader_;
	com_ptr<ID3D11RasterizerState>		p_rasterizer_state_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
	com_ptr<ID3D11Buffer>				p_cb_vertex_shader_;
	com_ptr<ID3D11Texture2D>			p_tex_skybox_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_skybox_srv_;
};

class renderer final {
public:

	renderer(HWND p_hwnd, const uint2& viewport_size);

	renderer(renderer&&) = delete;
	renderer& operator=(renderer&&) = delete;

	~renderer() noexcept;


	void draw_frame(frame& frame);

	void resize_viewport(const uint2& size);

private:

	void init_assets();

	void init_device(HWND p_hwnd, const uint2& viewport_size);


	// device stuff:
	com_ptr<ID3D11Device>			p_device_;
	com_ptr<ID3D11DeviceContext>	p_ctx_;
	com_ptr<ID3D11Debug>			p_debug_;
	std::unique_ptr<gbuffer>		p_gbuffer_;
	// swap chain stuff:
	com_ptr<IDXGISwapChain>			p_swap_chain_;
	com_ptr<ID3D11RenderTargetView> p_tex_window_rtv_;
	// render stuff
	D3D11_VIEWPORT					viewport_ = { 0, 0, 0, 0, 0, 1 };
	std::unique_ptr<skybox_pass>	p_skybox_pass_;
	std::unique_ptr<shading_pass>	p_light_pass_;
	std::unique_ptr<final_pass>		p_final_pass_;
};


} // namespace rnd
} // namespace sparki
