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

	// skybox
	static constexpr UINT skybox_side_size = 512;
	static constexpr UINT skybox_mipmap_count = 10;
	static constexpr UINT skybox_compute_group_x_size = 512;
	static constexpr UINT skybox_compute_group_y_size = 2;
	static constexpr UINT skybox_compute_gx = skybox_side_size / skybox_compute_group_x_size;
	static constexpr UINT skybox_compute_gy = skybox_side_size / skybox_compute_group_y_size;
	// diffuse envmap
	static constexpr UINT diffuse_envmap_side_size = 64;
	static constexpr UINT diffuse_compute_group_x_size = 64;
	static constexpr UINT diffuse_compute_group_y_size = 16;
	static constexpr UINT diffuse_compute_gx = diffuse_envmap_side_size / diffuse_compute_group_x_size;
	static constexpr UINT diffuse_compute_gy = diffuse_envmap_side_size / diffuse_compute_group_y_size;
	// specular envmap
	static constexpr UINT specular_envmap_side_size = 128;
	static constexpr UINT specular_envmap_mipmap_count = 5;
	static constexpr UINT specular_envmap_compute_group_x_size = 8;
	static constexpr UINT specular_envmap_compute_group_y_size = 8;


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

class material_editor_tool final {
public:

	static const ubyte4 defualt_color_value;
	static const ubyte4 defualt_texture_value;


	material_editor_tool(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	material_editor_tool(material_editor_tool&&) = delete;
	material_editor_tool& operator=(material_editor_tool&&) = delete;


	const material& current_material() const noexcept
	{
		return material_;
	}

	ID3D11ShaderResourceView* p_tex_base_color_input_color()
	{
		return p_tex_base_color_input_color_srv_;
	}

	ID3D11ShaderResourceView* p_tex_base_color_input_texture() noexcept
	{
		return p_tex_base_color_input_texture_srv_;
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

	void update_base_color_color(const ubyte4& value);

private:

	ID3D11Device*			p_device_;
	ID3D11DeviceContext*	p_ctx_;
	ID3D11Debug*			p_debug_;
	// current material stuff ---
	material							material_;
	// base color
	com_ptr<ID3D11Texture2D>			p_tex_base_color_input_color_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_base_color_input_color_srv_;
	com_ptr<ID3D11Texture2D>			p_tex_base_color_output_color_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_base_color_output_color_srv_;
	com_ptr<ID3D11Texture2D>			p_tex_base_color_input_texture_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_base_color_input_texture_srv_;
	// reflect color color
	com_ptr<ID3D11Texture2D>			p_tex_reflect_color_output_color_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_reflect_color_output_color_srv_;
};

} // namespace core
} // namespace sparki
