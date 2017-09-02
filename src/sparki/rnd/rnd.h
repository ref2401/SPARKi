#pragma once

#include <memory>
#include "sparki/rnd/rnd_base.h"
#include "sparki/rnd/rnd_tool.h"


namespace sparki {
namespace rnd {

struct frame final {
	math::float4x4	projection_matrix;
	float3			camera_position;
	float3			camera_target;
	float3			camera_up;
};

struct gbuffer final {

	static constexpr float viewport_factor = 1.333f;


	explicit gbuffer(ID3D11Device* p_device);

	gbuffer(gbuffer&&) = delete;
	gbuffer& operator=(gbuffer&&) = delete;


	void resize(ID3D11Device* p_device, const uint2 size);


	// color texture
	com_ptr<ID3D11Texture2D>			p_tex_color;
	com_ptr<ID3D11ShaderResourceView>	p_tex_color_srv;
	com_ptr<ID3D11RenderTargetView>		p_tex_color_rtv;
	// post processing
	com_ptr<ID3D11Texture2D>			p_tex_tone_mapping;
	com_ptr<ID3D11ShaderResourceView>	p_tex_tone_mapping_srv;
	com_ptr<ID3D11UnorderedAccessView>	p_tex_tone_mapping_uav;
	com_ptr<ID3D11Texture2D>			p_tex_aa;
	com_ptr<ID3D11ShaderResourceView>	p_tex_aa_srv;
	com_ptr<ID3D11UnorderedAccessView>	p_tex_aa_uav;
	// depth texture
	com_ptr<ID3D11Texture2D>			p_tex_depth;
	com_ptr<ID3D11DepthStencilView>		p_tex_depth_dsv;
	
	// other stuff:

	// Common rasterizer state. FrontCounterClockwise, CULL_BACK
	com_ptr<ID3D11RasterizerState>		p_rasterizer_state;
	// Sampler: MIN_MAG_MIP_LINEAR, ADDRESS_CLAMP, LOD in [0, D3D11_FLOAT32_MAX]
	com_ptr<ID3D11SamplerState>			p_sampler_linear;
	// Sampler: MIN_MAG_MIP_POINT, ADDRESS_CLAMP, LOD in [0, D3D11_FLOAT32_MAX]
	com_ptr<ID3D11SamplerState>			p_sampler_point;
	D3D11_VIEWPORT						rnd_viewport = { 0, 0, 0, 0, 0, 1 };
	D3D11_VIEWPORT						window_viewport = { 0, 0, 0, 0, 0, 1 };
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

	void init_textures();


	ID3D11Device*						p_device_;
	ID3D11DeviceContext*				p_ctx_;
	ID3D11Debug*						p_debug_;
	hlsl_shader							shader_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
	com_ptr<ID3D11Buffer>				p_cb_vertex_shader_;
	com_ptr<ID3D11Texture2D>			p_tex_diffuse_envmap_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_diffuse_envmap_srv_;
	com_ptr<ID3D11Texture2D>			p_tex_specular_envmap_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_specular_envmap_srv_;
	com_ptr<ID3D11Texture2D>			p_tex_specular_brdf_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_specular_brdf_srv_;
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

// Post-processing pass. Comprises such techniques as: tone mapping, anti-aliasing, etc.
// Applies tone mapping to gbuffer.p_tex_color and computes luminance for each texel.
// The output has rgbl format.
class postproc_pass final {
public:

	postproc_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	postproc_pass(postproc_pass&&) = delete;
	postproc_pass& operator=(postproc_pass&&) = delete;


	void perform(const gbuffer& gbuffer, ID3D11UnorderedAccessView* p_tex_window_uav);

private:

	static constexpr UINT postproc_compute_group_x_size = 512;
	static constexpr UINT postproc_compute_group_y_size = 2;
	static constexpr UINT downsample_compute_group_x_size = 32;
	static constexpr UINT downsample_compute_group_y_size = 32;


	ID3D11Device*			p_device_;
	ID3D11DeviceContext*	p_ctx_;
	ID3D11Debug*			p_debug_;
	hlsl_compute			tone_mapping_compute_;
	hlsl_compute			fxaa_compute_;
	hlsl_compute			downsample_compute_;
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

	void init_dx_device(HWND p_hwnd, const uint2& viewport_size);

	void init_passes_and_tools();


	// device stuff:
	com_ptr<ID3D11Device>			p_device_;
	com_ptr<ID3D11DeviceContext>	p_ctx_;
	com_ptr<ID3D11Debug>			p_debug_;
	std::unique_ptr<gbuffer>		p_gbuffer_;
	// swap chain stuff:
	com_ptr<IDXGISwapChain>				p_swap_chain_;
	com_ptr<ID3D11Texture2D>			p_tex_window_;
	com_ptr<ID3D11RenderTargetView>		p_tex_window_rtv_;
	com_ptr<ID3D11UnorderedAccessView>	p_tex_window_uav_;
	// rnd tools
	std::unique_ptr<envmap_texture_builder> p_envmap_builder_;
	std::unique_ptr<brdf_integrator>		p_brdf_integrator_;
	// render stuff
	std::unique_ptr<skybox_pass>		p_skybox_pass_;
	std::unique_ptr<shading_pass>		p_light_pass_;
	std::unique_ptr<postproc_pass>		p_postproc_pass_;
};


} // namespace rnd
} // namespace sparki
