#pragma once

#include "sparki/core/rnd_tool.h"


namespace sparki {


class material_editor_view final {
public:

	material_editor_view(HWND p_hwnd, core::material_editor_tool& met);

	void show();


private:

	static constexpr size_t c_material_name_max_length = 256;


	void show_base_color_ui();

	void show_reflect_color_ui();

	void show_material_properties_ui();

	// core stuff ---
	HWND							p_hwnd_;
	core::material_editor_tool&		met_;
	// view model ---
	float3							base_color_color_;
	bool							base_color_color_active_;
	std::string						base_color_texture_filename_;
	float3							reflect_color_color_;
	bool							reflect_color_color_active_;
	std::string						reflect_color_texture_filename_;
	std::string						property_mask_texture_filename_;
};

} // namespace sparki
