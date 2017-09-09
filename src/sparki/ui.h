#pragma once

#include "sparki/core/rnd_tool.h"


namespace sparki {


class material_editor_view final {
public:

	explicit material_editor_view(core::material_editor_tool& met);

	void show();

private:

	static constexpr size_t material_name_max_length = 256;

	core::material_editor_tool&	met_;
	char						name_[material_name_max_length];
	float3						base_color_color_;
};

} // namespace sparki
