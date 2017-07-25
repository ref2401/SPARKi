#pragma once

#include "sparki/rnd/rnd_base.h"


namespace sparki {
namespace rnd {

class ibl_texture_builder final {
public:

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

	ID3D11Device*				p_device_;
	ID3D11DeviceContext*		p_ctx_;
	ID3D11Debug*				p_debug_;
	hlsl_compute				equirect_to_skybox_compute_shader_;
	hlsl_compute				prefilter_envmap_compute_shader_;
	com_ptr<ID3D11SamplerState> p_sampler_;
};

} // namespace rnd
} // namespace sparki
