#include "sparki/core/platform_input.h"

#include <windows.h>



namespace sparki {
namespace core {

// ----- input_state -----

input_state::input_state()
{
	std::fill(std::begin(keys), std::end(keys), key_state::released);
}


} // namespace core
} // namespace sparki
