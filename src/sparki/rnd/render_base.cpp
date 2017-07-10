#include "sparki/rnd/render_base.h"

#include <cassert>


namespace sparki {

com_ptr<ID3DBlob> compile_shader(const std::string& source_code, const std::string& source_filename, 
	uint32_t compile_flags, const char* p_entry_point_name, const char* p_shader_model)
{
	com_ptr<ID3DBlob> p_bytecode;
	com_ptr<ID3DBlob> p_error_blob;

	HRESULT hr = D3DCompile(
		source_code.c_str(),
		source_code.size(),
		source_filename.c_str(),
		nullptr,					// defines
		nullptr,					// includes
		p_entry_point_name,
		p_shader_model,
		compile_flags,
		0,							// effect compilation flags
		&p_bytecode.ptr,
		&p_error_blob.ptr
	);

	if (hr != S_OK) {
		std::string error(static_cast<char*>(p_error_blob->GetBufferPointer()), p_error_blob->GetBufferSize());
		throw std::runtime_error(error);
	}

	return p_bytecode;
}

} // namespace sparki
