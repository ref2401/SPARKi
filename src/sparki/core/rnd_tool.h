#pragma once

#include "sparki/core/rnd_base.h"


namespace sparki {
namespace core {

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

	// skybox ---
	static constexpr UINT c_skybox_side_size = 512;
	static constexpr UINT c_skybox_mipmap_count = 10;
	static constexpr UINT c_skybox_compute_group_x_size = 512;
	static constexpr UINT c_skybox_compute_group_y_size = 2;
	static constexpr UINT c_skybox_compute_gx = c_skybox_side_size / c_skybox_compute_group_x_size;
	static constexpr UINT c_skybox_compute_gy = c_skybox_side_size / c_skybox_compute_group_y_size;
	// diffuse envmap ---
	static constexpr UINT c_diffuse_envmap_side_size = 64;
	static constexpr UINT c_diffuse_compute_group_x_size = 64;
	static constexpr UINT c_diffuse_compute_group_y_size = 16;
	static constexpr UINT c_diffuse_compute_gx = c_diffuse_envmap_side_size / c_diffuse_compute_group_x_size;
	static constexpr UINT c_diffuse_compute_gy = c_diffuse_envmap_side_size / c_diffuse_compute_group_y_size;
	// specular envmap ---
	static constexpr UINT c_specular_envmap_side_size = 128;
	static constexpr UINT c_specular_envmap_mipmap_count = 5;
	static constexpr UINT c_specular_envmap_compute_group_x_size = 8;
	static constexpr UINT c_specular_envmap_compute_group_y_size = 8;


	com_ptr<ID3D11Texture2D> make_skybox(const char* p_hdr_filename);

	com_ptr<ID3D11Texture2D> make_diffuse_envmap(ID3D11ShaderResourceView* p_tex_skybox_srv);

	com_ptr<ID3D11Texture2D> make_specular_envmap(ID3D11Texture2D* p_tex_skybox, 
		ID3D11ShaderResourceView* p_tex_skybox_srv);

	void save_skybox_to_file(const char* p_filename, ID3D11Texture2D* p_tex_skybox);


	ID3D11Device*				p_device_;
	ID3D11DeviceContext*		p_ctx_;
	ID3D11Debug*				p_debug_;
	ID3D11SamplerState*			p_sampler_;
	hlsl_compute				equirect_to_skybox_compute_;
	hlsl_compute				diffuse_envmap_compute_;
	hlsl_compute				specular_envmap_compute_;
	com_ptr<ID3D11Buffer>		p_cb_prefilter_envmap_;
};

// Retrieves a list of unique colors from the specified image (up to 32 colors)
// The colors are sorted and stored in color_buffer_cpu() & p_color_buffers().
class unique_color_miner final {
public:

	unique_color_miner(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	unique_color_miner(unique_color_miner&&) = delete;
	unique_color_miner& operator=(unique_color_miner&&) = delete;


	const std::vector<uint32_t>& color_buffer_cpu() const noexcept
	{
		return color_buffer_cpu_;
	}

	void perform(const char* p_image_filename);

private:

	static constexpr UINT c_compute_group_x_size		= 32;
	static constexpr UINT c_compute_group_y_size		= 32;
	static constexpr UINT c_color_buffer_count			= 32;
	static constexpr UINT c_color_buffer_byte_count		= sizeof(uint32_t) * c_color_buffer_count;
	static constexpr UINT c_hash_buffer_count			= 0x1'00'00'00; // 0xffffff + 1, or 2^24
	static constexpr UINT c_result_buffer_count			= c_color_buffer_count + 1; // + 1 stands for color_buffer counter value.
	static constexpr UINT c_result_buffer_byte_count	= sizeof(uint32_t) * c_result_buffer_count;


	ID3D11Device*						p_device_;
	ID3D11DeviceContext*				p_ctx_;
	ID3D11Debug*						p_debug_;
	hlsl_compute						hash_colors_compute_;
	com_ptr<ID3D11Buffer>				p_hash_buffer_;
	com_ptr<ID3D11UnorderedAccessView>	p_hash_buffer_uav_;
	com_ptr<ID3D11Buffer>				p_color_buffer_;
	com_ptr<ID3D11UnorderedAccessView>	p_color_buffer_uav_;
	com_ptr<ID3D11Buffer>				p_result_buffer_;
	std::vector<uint32_t>				color_buffer_cpu_;
};

class material_editor_tool final {
public:

	static const ubyte4 c_default_color_value;
	static const ubyte4 c_default_texture_value;
	static const ubyte4 c_default_param_mask_value;


	material_editor_tool(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	material_editor_tool(material_editor_tool&&) = delete;
	material_editor_tool& operator=(material_editor_tool&&) = delete;


	const material& current_material() const noexcept
	{
		return material_;
	}

	const std::vector<uint32_t>& param_mask_color_buffer() const noexcept
	{
		return color_miner_.color_buffer_cpu();
	}

	ID3D11ShaderResourceView* p_tex_base_color_input_texture_srv() noexcept
	{
		return p_tex_base_color_input_texture_srv_;
	}

	ID3D11ShaderResourceView* p_tex_param_mask_srv() noexcept
	{
		return p_tex_param_mask_srv_;
	}




	void activate_base_color_color() 
	{ 
		material_.p_tex_base_color_srv = p_tex_base_color_output_color_srv_; 
	}

	void activate_base_color_texture() 
	{
		material_.p_tex_base_color_srv = p_tex_base_color_input_texture_srv_;
	}

	void reload_base_color_input_texture(const char* p_filename);

	void reload_param_mask_texture(const char* p_filename);

	void update_base_color_color(const ubyte4& value);

private:

	ID3D11Device*			p_device_;
	ID3D11DeviceContext*	p_ctx_;
	ID3D11Debug*			p_debug_;
	unique_color_miner		color_miner_;
	// current material stuff ---
	material							material_;
	// base color ---
	com_ptr<ID3D11Texture2D>			p_tex_base_color_output_color_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_base_color_output_color_srv_;
	com_ptr<ID3D11Texture2D>			p_tex_base_color_input_texture_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_base_color_input_texture_srv_;
	// reflect color color ---
	com_ptr<ID3D11Texture2D>			p_tex_reflect_color_output_color_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_reflect_color_output_color_srv_;
	// parameter mask ---
	com_ptr<ID3D11Texture2D>			p_tex_param_mask_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_param_mask_srv_;
};

} // namespace core
} // namespace sparki
