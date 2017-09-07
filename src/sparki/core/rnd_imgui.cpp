#include "sparki/core/rnd_imgui.h"

#include <cassert>
#include <limits>


namespace sparki {
namespace core {

// ----- imgui_pass -----

imgui_pass::imgui_pass(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device),
	p_ctx_(p_ctx),
	p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	init_font_texture();
	init_shader();
	p_cb_vertex_shader_ = make_constant_buffer(p_device_, sizeof(math::float4x4));
	init_vertex_buffers();
	init_pipeline_state_objects();
}

imgui_pass::~imgui_pass() noexcept
{

}

void imgui_pass::init_font_texture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* p_data;
	int w, h;
	io.Fonts->GetTexDataAsRGBA32(&p_data, &w, &h);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = p_data;
	data.SysMemPitch = w * 4;
	HRESULT hr = p_device_->CreateTexture2D(&desc, &data, &p_tex_font_.ptr);
	assert(hr == S_OK);
	hr = p_device_->CreateShaderResourceView(p_tex_font_, nullptr, &p_tex_font_srv_.ptr);
	assert(hr == S_OK);

	io.Fonts->TexID = p_tex_font_srv_.ptr;
}

void imgui_pass::init_pipeline_state_objects()
{
	D3D11_RASTERIZER_DESC rastr_desc = {};
	rastr_desc.FillMode = D3D11_FILL_SOLID;
	rastr_desc.CullMode = D3D11_CULL_NONE;
	rastr_desc.ScissorEnable = true;
	rastr_desc.DepthClipEnable = true;
	HRESULT hr = p_device_->CreateRasterizerState(&rastr_desc, &p_rasterizer_state_.ptr);
	assert(hr == S_OK);

	D3D11_BLEND_DESC blend_desc = {};
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = p_device_->CreateBlendState(&blend_desc, &p_blend_state_.ptr);
	assert(hr == S_OK);

	D3D11_DEPTH_STENCIL_DESC ds_desc = {};
	hr = p_device_->CreateDepthStencilState(&ds_desc, &p_depth_stencil_state_.ptr);
	assert(hr == S_OK);
}

void imgui_pass::init_shader()
{
	hlsl_shader_desc shader_desc;
	shader_desc.source_code = R"(
cbuffer vertexBuffer : register(b0) {
	float4x4 g_projection_matrix;
};
struct VS_INPUT {
	float2 pos : POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

PS_INPUT vs_main(VS_INPUT input)
{
	PS_INPUT output;
	output.pos = mul(g_projection_matrix, float4(input.pos.xy, 0.0f, 1.0f));
	output.col = input.col;
	output.uv  = input.uv;
	return output;
}

Texture2D		g_tex_0 : register(t0);
SamplerState	g_sampler : register(s0);

float4 ps_main(PS_INPUT input) : SV_Target0
{
	return input.col * g_tex_0.Sample(g_sampler, input.uv);
}
)";

	shader_ = hlsl_shader(p_device_, shader_desc);
}

void imgui_pass::init_vertex_buffers()
{
	assert(shader_.p_vertex_shader_bytecode); // shader has been compiled

	D3D11_BUFFER_DESC vb_desc = {};
	vb_desc.ByteWidth = imgui_pass::vertex_buffer_byte_count;
	vb_desc.Usage = D3D11_USAGE_DYNAMIC;
	vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = p_device_->CreateBuffer(&vb_desc, nullptr, &p_vertex_buffer_.ptr);
	assert(hr == S_OK);

	D3D11_BUFFER_DESC ib_desc = {};
	ib_desc.ByteWidth = imgui_pass::index_buffer_byte_count;
	ib_desc.Usage = D3D11_USAGE_DYNAMIC;
	ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ib_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = p_device_->CreateBuffer(&ib_desc, nullptr, &p_index_buffer_.ptr);
	assert(hr == S_OK);

	constexpr UINT vertex_count = 3;
	D3D11_INPUT_ELEMENT_DESC layout_desc[vertex_count] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, UINT(offsetof(ImDrawVert, pos)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, UINT(offsetof(ImDrawVert, uv)),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, UINT(offsetof(ImDrawVert, col)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = p_device_->CreateInputLayout(layout_desc, vertex_count,
		shader_.p_vertex_shader_bytecode->GetBufferPointer(),
		shader_.p_vertex_shader_bytecode->GetBufferSize(),
		&p_input_layout_.ptr);
	assert(hr == S_OK);
}

void imgui_pass::fill_vertex_buffers(ImDrawData* p_draw_data)
{
	assert(UINT(sizeof(ImDrawVert) * p_draw_data->TotalVtxCount) <= imgui_pass::vertex_buffer_byte_count);
	assert(UINT(sizeof(ImDrawIdx) * p_draw_data->TotalIdxCount) <= imgui_pass::index_buffer_byte_count);

	D3D11_MAPPED_SUBRESOURCE vbm;
	HRESULT hr = p_ctx_->Map(p_vertex_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &vbm);
	assert(hr == S_OK);
	D3D11_MAPPED_SUBRESOURCE ibm;
	hr = p_ctx_->Map(p_index_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &ibm);
	assert(hr == S_OK);

	ImDrawVert* p_vert = reinterpret_cast<ImDrawVert*>(vbm.pData);
	ImDrawIdx* p_idx = reinterpret_cast<ImDrawIdx*>(ibm.pData);
	for (int i = 0; i < p_draw_data->CmdListsCount; ++i) {
		const ImDrawList* p_cl = p_draw_data->CmdLists[i];

		std::memcpy(p_vert, p_cl->VtxBuffer.Data, p_cl->VtxBuffer.Size * sizeof(ImDrawVert));
		std::memcpy(p_idx, p_cl->IdxBuffer.Data, p_cl->IdxBuffer.Size * sizeof(ImDrawIdx));

		p_vert += p_cl->VtxBuffer.Size;
		p_idx += p_cl->IdxBuffer.Size;
	}

	p_ctx_->Unmap(p_vertex_buffer_, 0);
	p_ctx_->Unmap(p_index_buffer_, 0);
}

void imgui_pass::perform(ImDrawData* p_draw_data, ID3D11SamplerState* p_sampler)
{
	assert(p_draw_data);
	assert(p_sampler);
	static_assert(sizeof(ImDrawIdx) == 2, "2 -> DXGI_FORMAT_R16_UINT, 4 -> DXGI_FORMAT_R32_UINT");

	// ----- update buffers -----
	fill_vertex_buffers(p_draw_data);

	const ImVec2 vp_size = ImGui::GetIO().DisplaySize;
	// update p_cb_vertex_shader_
	const float4x4 projection_matrix = orthographic_matrix_directx(0.0f, vp_size.x, vp_size.y, 0.0f, -1.0f, 1.0f);
	float cb_data[16];
	to_array_column_major_order(projection_matrix, cb_data);
	p_ctx_->UpdateSubresource(p_cb_vertex_shader_, 0, nullptr, cb_data, 0, 0);

	// ----- setup rnd pipeline -----
	// IA
	const UINT stride = sizeof(ImDrawVert);
	const UINT offset = 0;
	p_ctx_->IASetVertexBuffers(0, 1, &p_vertex_buffer_.ptr, &stride, &offset);
	p_ctx_->IASetIndexBuffer(p_index_buffer_, DXGI_FORMAT_R16_UINT, 0);
	p_ctx_->IASetInputLayout(p_input_layout_);
	p_ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// VS
	p_ctx_->VSSetShader(shader_.p_vertex_shader, nullptr, 0);
	p_ctx_->VSSetConstantBuffers(0, 1, &p_cb_vertex_shader_.ptr);
	// RS
	const D3D11_VIEWPORT vp = { 0, 0, vp_size.x, vp_size.y, 0.0f, 1.0f };
	p_ctx_->RSSetViewports(1, &vp);
	p_ctx_->RSSetState(p_rasterizer_state_);
	// PS
	p_ctx_->PSSetShader(shader_.p_pixel_shader, nullptr, 0);
	p_ctx_->PSSetSamplers(0, 1, &p_sampler);
	// OM
	p_ctx_->OMSetBlendState(p_blend_state_, &float4::zero.x, std::numeric_limits<UINT>::max());
	p_ctx_->OMSetDepthStencilState(p_depth_stencil_state_, 0);

	// ----- rnd -----
	int base_vertex = 0;
	UINT index_offset = 0;
	for (int l = 0; l < p_draw_data->CmdListsCount; ++l) {
		const ImDrawList* p_cmd_list = p_draw_data->CmdLists[l];

		for (int c = 0; c < p_cmd_list->CmdBuffer.Size; ++c) {
			const ImDrawCmd* p_cmd = &p_cmd_list->CmdBuffer[c];
			assert(!p_cmd->UserCallback);

			const D3D11_RECT rect = { LONG(p_cmd->ClipRect.x), LONG(p_cmd->ClipRect.y),
				LONG(p_cmd->ClipRect.z), LONG(p_cmd->ClipRect.w) };
			p_ctx_->RSSetScissorRects(1, &rect);

			ID3D11ShaderResourceView* p_tex_srv = reinterpret_cast<ID3D11ShaderResourceView*>(p_cmd->TextureId);
			p_ctx_->PSSetShaderResources(0, 1, &p_tex_srv);

#ifdef SPARKI_DEBUG
			HRESULT hr = p_debug_->ValidateContext(p_ctx_);
			assert(hr == S_OK);
#endif
			p_ctx_->DrawIndexed(p_cmd->ElemCount, index_offset, base_vertex);
			index_offset += p_cmd->ElemCount;
		}

		base_vertex += p_cmd_list->VtxBuffer.Size;
	}
}

} // namespace core
} // namespace sparki

