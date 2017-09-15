#pragma once

#include "sparki/core/rnd_tool.h"


namespace sparki {


class material_editor_view final {
public:

	material_editor_view(HWND p_hwnd, core::material_editor_tool& met);

	void show();


private:

	static constexpr size_t c_material_name_max_length = 256;

	struct property_mapping final {
		uint32_t 	color;
		bool 		metallic_mask;
		float 		roughness;
	};



	void update_property_mappings();

	void show_base_color_ui();

	void show_metal_roughness_ui();

	// core stuff ---
	HWND						p_hwnd_;
	core::material_editor_tool&	met_;
	// view model ---
	char							name_[c_material_name_max_length];
	float3							base_color_color_;
	bool							base_color_color_active_;
	std::string						base_color_texture_filename_;
	std::string						param_mask_texture_filename_;
	std::vector<property_mapping>	property_mappings_;
	size_t							property_mapping_count_;
};

} // namespace sparki
