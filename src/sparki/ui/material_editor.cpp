#include "sparki/ui/material_editor.h"

#include <cassert>
#include <string>
#include "math/math.h"
#include "imgui/imgui.h"


namespace sparki {
namespace ui {

material_editor_view::material_editor_view()
{
}

void material_editor_view::show()
{
	constexpr float horz_indent = 20;

	static char name[256] = { "My Material #1" };
	static math::float3 base_color_rgb;
	static char base_color_filename[512];

	ImGui::Begin("Material Properties");
	ImGui::InputText("Name", name, 256);
	ImGui::Text("Base Color");
	ImGui::BeginGroup();
		ImGui::Indent(horz_indent);
		ImGui::Text("Color"); ImGui::SameLine();
		ImGui::ColorEdit3("", &base_color_rgb.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImGui::Text("Texture"); ImGui::SameLine();
		//ImGui::ImageButton();
	ImGui::EndGroup();
	ImGui::End();
}

} // namespace ui
} // namespace sparki
