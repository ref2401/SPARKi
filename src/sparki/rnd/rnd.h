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

class light_pass final {
public:

	light_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	light_pass(light_pass&&) = delete;
	light_pass& operator=(light_pass&&) = delete;

	
	void perform();

private:

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
	// swap chain stuff:
	com_ptr<IDXGISwapChain>			p_swap_chain_;
	com_ptr<ID3D11RenderTargetView> p_tex_window_rtv_;
	// render stuff
	D3D11_VIEWPORT					viewport_ = { 0, 0, 0, 0, 0, 1 };
	std::unique_ptr<skybox_pass>	p_skybox_pass_;
	std::unique_ptr<light_pass>		p_light_pass_;
};

} // namespace rnd
} // namespace sparki
