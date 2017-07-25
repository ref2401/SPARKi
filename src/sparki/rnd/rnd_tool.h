#pragma once

#include "sparki/rnd/rnd_base.h"


namespace sparki {
namespace rnd {

class equirect_to_skybox_converter final {
public:

	explicit equirect_to_skybox_converter(ID3D11Device* p_device);

	equirect_to_skybox_converter(equirect_to_skybox_converter&&) = delete;
	equirect_to_skybox_converter& operator=(equirect_to_skybox_converter&&) = delete;


	void convert(const char* p_source_filename, const char* p_dest_filename, 
		ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug, UINT side_size);

private:

	hlsl_compute				compute_shader_;
	com_ptr<ID3D11SamplerState> p_sampler_;
};

class prefilter_envmap_technique final {
public:

	explicit prefilter_envmap_technique(ID3D11Device* p_device);

	prefilter_envmap_technique(prefilter_envmap_technique&&) = delete;
	prefilter_envmap_technique& operator=(prefilter_envmap_technique&&) = delete;


	void perform();

private:

	hlsl_compute compute_shader_;
};

} // namespace rnd
} // namespace sparki
