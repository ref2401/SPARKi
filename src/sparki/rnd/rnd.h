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

class shading_pass final {
public:

	shading_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	shading_pass(shading_pass&&) = delete;
	shading_pass& operator=(shading_pass&&) = delete;

	
	void perform(const float4x4& pv_matrix);

private:

	void init_geometry();

	void init_pipeline_state();

	void init_textures(const texture_data& td_envmap, const texture_data& td_brdf);


	ID3D11Device*						p_device_;
	ID3D11DeviceContext*				p_ctx_;
	ID3D11Debug*						p_debug_;
	hlsl_shader							shader_;
	com_ptr<ID3D11RasterizerState>		p_rasterizer_state_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
	com_ptr<ID3D11Buffer>				p_cb_vertex_shader_;
	com_ptr<ID3D11SamplerState>			p_sampler_;
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


	void perform(const float4x4& pv_matrix, const float3& position);

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
	com_ptr<ID3D11SamplerState>			p_sampler_;
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
	com_ptr<IDXGISwapChain>			p_swap_chain_;
	// rtv stuff:
	com_ptr<ID3D11RenderTargetView> p_tex_window_rtv_;
	com_ptr<ID3D11Texture2D>		p_tex_depth_stencil_;
	com_ptr<ID3D11DepthStencilView> p_tex_depth_stencil_dsv_;
	// render stuff
	D3D11_VIEWPORT					viewport_ = { 0, 0, 0, 0, 0, 1 };
	std::unique_ptr<skybox_pass>	p_skybox_pass_;
	std::unique_ptr<shading_pass>	p_light_pass_;
};

} // namespace rnd
} // namespace sparki
