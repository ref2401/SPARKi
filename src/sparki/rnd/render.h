#pragma once

#include <memory>
#include "sparki/rnd/rnd_base.h"
#include "sparki/rnd/rnd_converter.h"


namespace sparki {

struct frame final {
	math::float4x4	projection_matrix;
	float3 camera_position;
	float3 camera_target;
	float3 camera_up;
};

class skybox_pass final {
public:

	explicit skybox_pass(ID3D11Device* p_device);

	skybox_pass(skybox_pass&&) = delete;
	skybox_pass& operator=(skybox_pass&&) = delete;


	ID3D11UnorderedAccessView* p_tex_skybox_uav() noexcept
	{
		return p_tex_skybox_uav_;
	}

	void perform(ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug,
		const float4x4& pv_matrix, const float3& position);

private:

	void init_pipeline_state(ID3D11Device* p_device);

	void init_skybox_texture(ID3D11Device* p_device);

	hlsl_shader							shader_;
	com_ptr<ID3D11RasterizerState>		p_rasterizer_state_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
	com_ptr<ID3D11Buffer>				p_cb_vertex_shader_;
	com_ptr<ID3D11SamplerState>			p_sampler_;
	com_ptr<ID3D11Texture2D>			p_tex_skybox_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_skybox_srv_;
	com_ptr<ID3D11UnorderedAccessView>	p_tex_skybox_uav_;
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
	std::unique_ptr<skybox_pass>	p_skybox_pass_;
	// swap chain stuff:
	com_ptr<IDXGISwapChain>			p_swap_chain_;
	com_ptr<ID3D11RenderTargetView> p_tex_window_rtv_;
	// other
	D3D11_VIEWPORT					viewport_ = { 0, 0, 0, 0, 0, 1 };
	// temporary stuff
};


} // namespace sparki
