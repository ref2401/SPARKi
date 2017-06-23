#include "sparki/offline_tool.h"

#include <cassert>
#include "fbxsdk.h"


namespace sparki {

void convert_from_fbx(const char* p_fbx_filename, const char* p_desc_filename)
{
	assert(p_fbx_filename);
	assert(p_desc_filename);

	auto r = FbxManager::Create();
}

} // namespace
