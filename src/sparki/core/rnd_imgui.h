#pragma once

#include "sparki/core/rnd_base.h"
#include "imgui/imgui.h"


namespace sparki {
namespace core {

class imgui_pass final {
public:

	imgui_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	imgui_pass(imgui_pass&&) = delete;
	imgui_pass& operator=(imgui_pass&&) = delete;

	~imgui_pass() noexcept;


	void perform(ImDrawData* p_draw_data, ID3D11SamplerState* p_sampler);

private:

	static constexpr UINT vertex_buffer_byte_count = UINT(megabytes(2));
	static constexpr UINT index_buffer_byte_count = UINT(megabytes(1));


	void init_font_texture();

	void init_pipeline_state_objects();

	void init_shader();

	void init_vertex_buffers();

	void fill_vertex_buffers(ImDrawData* p_draw_data);


	ID3D11Device*						p_device_;
	ID3D11DeviceContext*				p_ctx_;
	ID3D11Debug*						p_debug_;
	hlsl_shader							shader_;
	com_ptr<ID3D11Buffer>				p_cb_vertex_shader_;
	com_ptr<ID3D11Buffer>				p_vertex_buffer_;
	com_ptr<ID3D11Buffer>				p_index_buffer_;
	com_ptr<ID3D11InputLayout>			p_input_layout_;
	com_ptr<ID3D11RasterizerState>		p_rasterizer_state_;
	com_ptr<ID3D11BlendState>			p_blend_state_;
	com_ptr<ID3D11DepthStencilState>	p_depth_stencil_state_;
	com_ptr<ID3D11Texture2D>			p_tex_font_;
	com_ptr<ID3D11ShaderResourceView>	p_tex_font_srv_;
};


} // namespace core
} // namespace sparki

