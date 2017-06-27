#pragma once

#include "render_base.h"


namespace sparki {

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
