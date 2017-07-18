#pragma once

#include "render_base.h"


namespace sparki {

struct frame final {
	math::float4x4	projection_matrix;
	float3 camera_position;
	float3 camera_target;
	float3 camera_up;
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

	static constexpr UINT cube_index_count = 14;


	void init_assets();

	void init_tex_cube();

	void init_device(HWND p_hwnd, const uint2& viewport_size);


	// device stuff:
	com_ptr<ID3D11Device>			p_device_;
	com_ptr<ID3D11DeviceContext>	p_ctx_;
#ifdef SPARKI_DEBUG
	com_ptr<ID3D11Debug>			p_debug_;
#endif
	// swap chain stuff:
	com_ptr<IDXGISwapChain>			p_swap_chain_;
	com_ptr<ID3D11RenderTargetView> p_tex_window_rtv_;
	// other
	D3D11_VIEWPORT					viewport_ = { 0, 0, 0, 0, 0, 1 };
	// temporary stuff
	com_ptr<ID3D11Buffer>				p_cb_vertex_shader_;
	com_ptr<ID3D11RasterizerState>		p_rastr_state_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
	com_ptr<ID3D11Texture2D>			p_tex_cubemap_;
	com_ptr<ID3D11SamplerState>			p_sampler_state_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_cubemap_srv_;
	com_ptr<ID3D11UnorderedAccessView>	p_tex_cubemap_uav_;
	hlsl_compute						gen_cubemap_compute_;
	hlsl_shader							rnd_cubemap_shader_;
};


} // namespace sparki
