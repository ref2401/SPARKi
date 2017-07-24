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

	hlsl_compute compute_shader_;
};

} // namespace rnd
} // namespace sparki
