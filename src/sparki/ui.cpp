#include "sparki/ui.h"

#include <cassert>
#include <algorithm>
#include <limits>
#include <string>
#include "math/math.h"
#include "imgui/imgui.h"


namespace {

constexpr ImGuiColorEditFlags c_flags_color_button = ImGuiColorEditFlags_NoInputs
	| ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip;

constexpr size_t c_property_mapping_max_count = 32;

const char* property_mapping_widget_names[2 * c_property_mapping_max_count] = {
	"##m00", "##r00", "##m01", "##r01", "##m02", "##r02", "##m03", "##r03", "##m04", "##r04",
	"##m05", "##r05", "##m06", "##r06", "##m07", "##r07", "##m08", "##r08", "##m09", "##r09",
	
	"##m10", "##r10", "##m11", "##r11", "##m12", "##r12", "##m13", "##r13", "##m14", "##r14",
	"##m15", "##r15", "##m16", "##r16", "##m17", "##r17", "##m18", "##r18",	"##m19", "##r19",

	"##m20", "##r20", "##m21", "##r21", "##m22", "##r22", "##m23", "##r23", "##m24", "##r24",
	"##m25", "##r25", "##m26", "##r26",	"##m27", "##r27", "##m28", "##r28", "##m29", "##r29",

	"##m30", "##r30", "##m31", "##r31"
};


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

inline ImVec4 make_color_imvec4(uint32_t v)
{
	const float4 c4 = unpack_8_8_8_8_into_unorm(v);
	return ImVec4(c4.x, c4.y, c4.z, c4.w);
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
	base_color_color_(make_color_float3(core::material_editor_tool::c_default_color)),
	base_color_color_active_(true),
	base_color_texture_filename_(512, '\0'),
	property_mask_texture_filename_(512, '\0')
{
	assert(p_hwnd);
}

void material_editor_view::show()
{
	ImGui::Begin("Material Properties");
	ImGui::InputText("Name", name_, c_material_name_max_length);
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
		ImGui::TextColored(bcc_color, "Color");
		if (ImGui::ImageButton(met_.p_tex_base_color_color_srv(), ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), 0)) {
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
		if (ImGui::ImageButton(met_.p_tex_base_color_texture_srv(), ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), 0)) {
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
	bool update_properties_texture = false;
	if (ImGui::ImageButton(met_.p_tex_property_mask_srv(), ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), 0)) {
		if (show_open_file_dialog(p_hwnd_, property_mask_texture_filename_)) {
			met_.reload_property_mask_texture(property_mask_texture_filename_.c_str());
			met_.activate_properties_texture();
			update_properties_texture = true;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset")) {
		property_mask_texture_filename_[0] = '\0';
		met_.reset_property_mask_texture();
		met_.update_properties_color();
		met_.activate_properties_color();
	}

	if (met_.property_count() <= 1) {
		float2& props = met_.property_values()[0];

		bool upd = false;
		bool check = props.x;
		upd |= ImGui::Checkbox("Metal", &check);
		upd |= ImGui::SliderFloat("Roughness", &props.y, 0.0f, 1.0f);
		if (upd) {
			props.x = (check) ? 1.0f : 0.0f;
			met_.update_properties_color();
		}
	} 
	else {
		bool upd = update_properties_texture;
		for (size_t i = 0; i < met_.property_count(); ++i) {
			const ImVec4 c = make_color_imvec4(met_.property_colors()[i]);
			float2& props = met_.property_values()[i];

			ImGui::ColorButton("", c, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
			ImGui::SameLine();
			bool check = props.x;
			if (ImGui::Checkbox(property_mapping_widget_names[i * 2], &check)) {
				upd = true;
				props.x = (check) ? 1.0f : 0.0f;
			}
			ImGui::SameLine();
			upd |= ImGui::SliderFloat(property_mapping_widget_names[i * 2 + 1], &props.y, 0.0f, 1.0f);
		}

		if (upd)
			met_.update_properties_texture();
	}
}

} // namespace sparki
