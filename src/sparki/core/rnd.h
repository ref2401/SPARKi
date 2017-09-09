#pragma once

#include <memory>
#include "sparki/core/rnd_base.h"
#include "sparki/core/rnd_imgui.h"
#include "sparki/core/rnd_pass.h"
#include "sparki/core/rnd_tool.h"


namespace sparki {
namespace core {

struct frame final {
	float4x4		projection_matrix;
	float3			camera_position;
	float3			camera_target;
	float3			camera_up;
	ImDrawData*		p_imgui_draw_data = nullptr;
};

class render_system final {
public:

	render_system(HWND p_hwnd, const uint2& viewport_size);

	render_system(render_system&&) = delete;
	render_system& operator=(render_system&&) = delete;

	~render_system() noexcept;


	void draw_frame(frame& frame);

	void resize_viewport(const uint2& size);

	ID3D11ShaderResourceView* p_tex_base_color_tmp_srv() noexcept
	{
		return p_tex_base_color_tmp_srv_;
	}

private:

	void init_dx_device(HWND p_hwnd, const uint2& viewport_size);

	void init_materials();

	void init_passes_and_tools();


	// device stuff ---
	com_ptr<ID3D11Device>			p_device_;
	com_ptr<ID3D11DeviceContext>	p_ctx_;
	com_ptr<ID3D11Debug>			p_debug_;
	std::unique_ptr<gbuffer>		p_gbuffer_;
	// swap chain stuff ---
	com_ptr<IDXGISwapChain>				p_swap_chain_;
	com_ptr<ID3D11Texture2D>			p_tex_window_;
	com_ptr<ID3D11RenderTargetView>		p_tex_window_rtv_;
	com_ptr<ID3D11UnorderedAccessView>	p_tex_window_uav_;
	// rnd tools ---
	std::unique_ptr<envmap_texture_builder> p_envmap_builder_;
	std::unique_ptr<brdf_integrator>		p_brdf_integrator_;
	// render stuff ---
	std::unique_ptr<skybox_pass>	p_skybox_pass_;
	std::unique_ptr<shading_pass>	p_light_pass_;
	std::unique_ptr<postproc_pass>	p_postproc_pass_;
	std::unique_ptr<imgui_pass>		p_imgui_pass_;
	// temporary here ---
	material material_;
	com_ptr<ID3D11Texture2D>			p_tex_base_color_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_base_color_srv_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_base_color_tmp_srv_;
	com_ptr<ID3D11Texture2D>			p_tex_reflect_color_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_reflect_color_srv_;
};

} // namespace core
} // namespace sparki
