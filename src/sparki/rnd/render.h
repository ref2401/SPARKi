#pragma once

#include "render_base.h"


namespace sparki {

struct hlsl_compute final {

	hlsl_compute() noexcept = default;

	hlsl_compute(ID3D11Device* p_device, const hlsl_compute_desc& desc);

	hlsl_compute(hlsl_compute&& s) noexcept = default;
	hlsl_compute& operator=(hlsl_compute&& s) noexcept = default;


	com_ptr<ID3D11ComputeShader>	p_compute_shader;
	com_ptr<ID3DBlob>				p_compute_shader_bytecode;
};

struct hlsl_shader final {

	hlsl_shader() noexcept = default;

	hlsl_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	hlsl_shader(hlsl_shader&& s) noexcept = default;
	hlsl_shader& operator=(hlsl_shader&& s) noexcept = default;


	com_ptr<ID3D11VertexShader> p_vertex_shader;
	com_ptr<ID3DBlob>			p_vertex_shader_bytecode;
	com_ptr<ID3D11HullShader>	p_hull_shader;
	com_ptr<ID3DBlob>			p_hull_shader_bytecode;
	com_ptr<ID3D11DomainShader> p_domain_shader;
	com_ptr<ID3DBlob>			p_domain_shader_bytecode;
	com_ptr<ID3D11PixelShader>	p_pixel_shader;
	com_ptr<ID3DBlob>			p_pixel_shader_bytecode;

private:

	void init_vertex_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	void init_hull_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	void init_domain_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	void init_pixel_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);
};

class renderer final {
public:

	renderer(HWND p_hwnd, const uint2& viewport_size);

	renderer(renderer&&) = delete;
	renderer& operator=(renderer&&) = delete;

	~renderer() noexcept;


	void draw_frame();

	void resize_viewport(const uint2& size);

private:

	void init_device(HWND hwnd, const uint2& viewport_size);

	void load_assets();


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
};

} // namespace sparki
