#pragma once

#include "sparki/rnd/rnd_base.h"


namespace sparki {
namespace rnd {

class brdf_integrator final {
public:

	brdf_integrator(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	brdf_integrator(brdf_integrator&&) = delete;
	brdf_integrator& operator=(brdf_integrator&&) = delete;


	void perform(const char* p_specular_brdf_filename);

private:

	static constexpr UINT brdf_side_size = 512;
	static constexpr UINT compute_group_x_size = 512;
	static constexpr UINT compute_group_y_size = 2;

	ID3D11Device*			p_device_;
	ID3D11DeviceContext*	p_ctx_;
	ID3D11Debug*			p_debug_;
	hlsl_compute			specular_brdf_compute_;
};

class envmap_texture_builder final {
public:

	envmap_texture_builder(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, 
		ID3D11Debug* p_debug, ID3D11SamplerState* p_sampler);

	envmap_texture_builder(envmap_texture_builder&&) = delete;
	envmap_texture_builder& operator=(envmap_texture_builder&&) = delete;


	void perform(const char* p_hdr_filename, const char* p_skybox_filename,
		const char* p_diffuse_envmap_filename, const char* p_specular_envmap_filename);

private:

	// skybox
	static constexpr UINT skybox_side_size = 512;
	static constexpr UINT skybox_mipmap_count = 10;
	static constexpr UINT skybox_compute_group_x_size = 512;
	static constexpr UINT skybox_compute_group_y_size = 2;
	static constexpr UINT skybox_compute_gx = skybox_side_size / skybox_compute_group_x_size;
	static constexpr UINT skybox_compute_gy = skybox_side_size / skybox_compute_group_y_size;
	// diffuse & specular envmaps
	static constexpr UINT envmap_side_size = 128;
	static constexpr UINT envmap_mipmap_count = 5;
	static constexpr UINT envmap_compute_group_x_size = 8;
	static constexpr UINT envmap_compute_group_y_size = 8;


	com_ptr<ID3D11Texture2D> make_cube_texture(UINT side_size, UINT mipmap_level_count,
		D3D11_USAGE usage, UINT bing_flags, UINT misc_flags = 0);

	com_ptr<ID3D11Texture2D> make_skybox(const char* p_hdr_filename);

	com_ptr<ID3D11Texture2D> make_specular_envmap(ID3D11ShaderResourceView* p_tex_skybox_srv);


	ID3D11Device*				p_device_;
	ID3D11DeviceContext*		p_ctx_;
	ID3D11Debug*				p_debug_;
	ID3D11SamplerState*			p_sampler_;
	hlsl_compute				equirect_to_skybox_compute_;
	hlsl_compute				specular_envmap_compute_;
	com_ptr<ID3D11Buffer>		p_cb_prefilter_envmap_;
};

} // namespace rnd
} // namespace sparki
