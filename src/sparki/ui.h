#pragma once

#include "sparki/core/rnd_tool.h"


namespace sparki {


class material_editor_view final {
public:

	material_editor_view(HWND p_hwnd, core::material_editor_tool& met);

	void show();


private:

	static constexpr size_t material_name_max_length = 256;

	void show_base_color_ui();

	// core stuff ---
	HWND						p_hwnd_;
	core::material_editor_tool&	met_;
	// user settings ---
	char			name_[material_name_max_length];
	float3			base_color_color_;
	bool			base_color_color_active_;
	std::string		base_color_texture_filename_;
};

} // namespace sparki
