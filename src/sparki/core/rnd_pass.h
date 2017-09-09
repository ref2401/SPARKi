#pragma once

#include "sparki/core/rnd_base.h"


namespace sparki {
namespace core {

class shading_pass final {
public:

	shading_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	shading_pass(shading_pass&&) = delete;
	shading_pass& operator=(shading_pass&&) = delete;


	void perform(const gbuffer& gbuffer, const float4x4& pv_matrix,
		const material& material, const float3& camera_position);

private:

	static constexpr size_t cb_vertex_shader_component_count = 3 * 16 + 2 * 4;


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

} // namespace core
} // namespace sparki
