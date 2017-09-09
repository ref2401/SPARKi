#pragma once

#include "sparki/core/rnd_tool.h"


namespace sparki {


class material_editor_view final {
public:

	explicit material_editor_view(core::material_editor_tool& met);

	void show();

private:

	core::material_editor_tool& met_;
};

} // namespace sparki
