#pragma once

#include "sparki/rnd/rnd_base.h"


namespace sparki {
namespace rnd {

class brdf_integrator final {
public:

	static constexpr UINT brdf_lut_side_min_limit = 1024;


	brdf_integrator(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	brdf_integrator(brdf_integrator&&) = delete;
	brdf_integrator& operator=(brdf_integrator&&) = delete;


	void perform(const char* p_brdf_lut_filename, UINT side_size);

private:

	ID3D11Device*			p_device_;
	ID3D11DeviceContext*	p_ctx_;
	ID3D11Debug*			p_debug_;
	hlsl_compute			compute_shader_;
};

class ibl_texture_builder final {
public:

	static constexpr UINT skybox_side_min_limit = 512;
	static constexpr UINT envmap_compute_group_size = 16;
	static constexpr UINT envmap_side_min_limit = 256;
	static constexpr UINT envmap_mipmap_level_count = 5;


	ibl_texture_builder(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug,
		const hlsl_compute_desc& hlsl_equirect_to_skybox,
		const hlsl_compute_desc& hlsl_prefilter_envmap);

	ibl_texture_builder(ibl_texture_builder&&) = delete;
	ibl_texture_builder& operator=(ibl_texture_builder&&) = delete;


	void perform(const char* p_hdr_filename, 
		const char* p_skybox_filename, UINT skybox_side_size,
		const char* p_envmap_filename, UINT envmap_side_size);

private:

	void convert_equirect_to_skybox(ID3D11ShaderResourceView* p_tex_equirect_srv,
		ID3D11UnorderedAccessView* p_tex_skybox_uav, UINT skybox_side_size);

	void filter_envmap(ID3D11ShaderResourceView* p_tex_skybox_srv,
		ID3D11Texture2D* p_tex_envmap, UINT envmap_side_size, UINT mipmap_level_count);

	com_ptr<ID3D11Texture2D> make_cube_texture(UINT side_size, UINT mipmap_level_count,
		D3D11_USAGE usage, UINT bing_flags);


	ID3D11Device*				p_device_;
	ID3D11DeviceContext*		p_ctx_;
	ID3D11Debug*				p_debug_;
	hlsl_compute				equirect_to_skybox_compute_shader_;
	hlsl_compute				prefilter_envmap_compute_shader_;
	com_ptr<ID3D11Buffer>		p_cb_prefilter_envmap_;
	com_ptr<ID3D11SamplerState> p_sampler_;
};

} // namespace rnd
} // namespace sparki
