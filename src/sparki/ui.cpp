#include "sparki/ui.h"

#include <cassert>
#include <limits>
#include <string>
#include "math/math.h"
#include "imgui/imgui.h"


namespace {

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

} // namespace


namespace sparki {

material_editor_view::material_editor_view(core::material_editor_tool& met)
	: met_(met),
	name_("My Material #1"),
	base_color_color_(make_color_float3(core::material_editor_tool::defualt_color_value))
{
}

void material_editor_view::show()
{
	constexpr float horz_indent = 20;

	ImGui::Begin("Material Properties");
	ImGui::InputText("Name", name_, material_editor_view::material_name_max_length);
	ImGui::Text("Base Color");
	ImGui::BeginGroup();
		ImGui::Indent(horz_indent);

		ImGui::Text("Color"); ImGui::SameLine();
		if (ImGui::ColorEdit3("", &base_color_color_.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
			const ubyte4 c = make_color_ubyte4(base_color_color_, 1.0f);
			met_.update_base_color_color(c);
		}
		
		ImGui::Text("Texture"); ImGui::SameLine();
		//ImGui::ImageButton();
	ImGui::EndGroup();
	ImGui::End();
}

} // namespace sparki
