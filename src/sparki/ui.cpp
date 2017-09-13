#include "sparki/ui.h"

#include <cassert>
#include <algorithm>
#include <limits>
#include <string>
#include "math/math.h"
#include "imgui/imgui.h"


namespace {

static constexpr ImGuiColorEditFlags flags_color_button = ImGuiColorEditFlags_NoInputs
	| ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip;


inline float3 make_color_float3(const ubyte4& rgba)
{
	constexpr float max = float(std::numeric_limits<ubyte4::component_type>::max());
	return float3(rgba.x / max, rgba.y / max, rgba.z / max);
}

inline ubyte4 make_color_ubyte4(const float3& rgb, float alpha = 1.0f)
{
	assert(rgb >= 0 && rgb <= 1);
	assert(0 <= alpha&& alpha <= 1);

	using num_t = ubyte4::component_type;
	constexpr float max = float(std::numeric_limits<num_t>::max());
	return ubyte4(num_t(rgb.x * max), num_t(rgb.y * max), num_t(rgb.z * max), num_t(alpha * max));
}

bool show_open_file_dialog(HWND p_hwnd, std::string& filename)
{
	assert(p_hwnd);
	assert(filename.capacity() > 0);

	// NOTE(ref2401): MSDN quote: lpstrFile - The file name used to initialize the File Name edit control.
	//  The first character of this buffer must be NULL if initialization is not necessary.
	filename[0] = '\0';

	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = p_hwnd;
	ofn.hInstance = GetModuleHandle(nullptr);
	ofn.lpstrFilter = "Image Files\0*.jpg;*.png;*.tga;\0\0";
	ofn.lpstrCustomFilter = nullptr;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = &filename[0];
	ofn.nMaxFile = DWORD(filename.size());
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.lpstrTitle = "Base Color Texture";
	ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	return GetOpenFileName(&ofn);
}

} // namespace


namespace sparki {

material_editor_view::material_editor_view(HWND p_hwnd, core::material_editor_tool& met)
	: p_hwnd_(p_hwnd),
	met_(met),
	name_("My Material #1"),
	base_color_color_(make_color_float3(core::material_editor_tool::c_default_color_value)),
	base_color_color_active_(true),
	base_color_texture_filename_(512, '\0'),
	param_mask_texture_filename_(512, '\0')
{
	assert(p_hwnd);
}

void material_editor_view::show()
{
	ImGui::Begin("Material Properties");
	ImGui::InputText("Name", name_, material_editor_view::material_name_max_length);
	ImGui::Spacing(); 
	ImGui::Spacing();
	
	// base color ---
	if (ImGui::CollapsingHeader("Base Color", ImGuiTreeNodeFlags_DefaultOpen)) show_base_color_ui();
	if (ImGui::CollapsingHeader("Metal & Roughness", ImGuiTreeNodeFlags_DefaultOpen)) show_metal_roughness_ui();

	ImGui::End();
}

void material_editor_view::show_base_color_ui()
{
	ImVec4 bcc_color(0.2f, 0.85f, 0.2f, 1.0f);
	ImVec4 bct_color(0.3f, 0.3f, 0.3f, 1.0f);
	if (!base_color_color_active_) std::swap(bcc_color, bct_color);

	ImGui::BeginGroup();
		// color ---
		ImGui::BeginGroup();
		const ImVec4 bcc(base_color_color_.x, base_color_color_.y, base_color_color_.z, 1.0f);
		ImGui::TextColored(bcc_color, "Color");
		if (ImGui::ColorButton("##bcc_color_button", bcc, flags_color_button, ImVec2(64, 64))) {
			if (base_color_color_active_) ImGui::OpenPopup("bcc_popup");
			base_color_color_active_ = true;
		}
		ImGui::EndGroup();
		if (ImGui::BeginPopup("bcc_popup")) {
			ImGui::ColorPicker3("##bcc_picker", &base_color_color_.x);
			ImGui::EndPopup();
			met_.update_base_color_color(make_color_ubyte4(base_color_color_, 1.0f));
		}

		ImGui::SameLine();

		// texture ---
		ImGui::BeginGroup();
		ImGui::TextColored(bct_color, "Texture");
		if (ImGui::ImageButton(met_.p_tex_base_color_input_texture_srv(), ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), 0)) {
			if (!base_color_color_active_) {
				if (show_open_file_dialog(p_hwnd_, base_color_texture_filename_))
					met_.reload_base_color_input_texture(base_color_texture_filename_.c_str());
			}
			base_color_color_active_ = false;
		}
		ImGui::EndGroup();

	ImGui::EndGroup();
	ImGui::Spacing();

	if (base_color_color_active_) met_.activate_base_color_color();
	else met_.activate_base_color_texture();
}

void material_editor_view::show_metal_roughness_ui()
{
	if (ImGui::ImageButton(met_.p_tex_param_mask_srv(), ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), 0)) {
		if (show_open_file_dialog(p_hwnd_, param_mask_texture_filename_))
			met_.reload_param_mask_texture(param_mask_texture_filename_.c_str());
	}

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::Button("Clear param mask");
	ImGui::EndGroup();

	if (met_.param_mask_color_buffer().size() == 1) {
		static bool is_metal;
		static float roughness;
		ImGui::Checkbox("Metal", &is_metal);
		ImGui::SliderFloat("Roughness##-1", &roughness, 0.0f, 1.0f);
	}
}

} // namespace sparki
