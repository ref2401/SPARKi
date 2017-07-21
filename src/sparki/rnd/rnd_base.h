#pragma once

#include "sparki/asset/asset.h"
#include <windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>

using namespace math;


#if !defined(NDEBUG)
	#define SPARKI_DEBUG 1
#elif
	#define SPARKI_RELEASE 1
#endif // !defined(NDEBUG)


namespace sparki {

// Unique_com_ptr is a smart pointer that owns and manages a COM object through a pointer 
// and disposes of that object when the Unique_com_ptr goes out of scope.
template<typename T>
struct com_ptr final {

	static_assert(std::is_base_of<IUnknown, T>::value, "T must be derived from IUnknown.");


	com_ptr() noexcept = default;

	explicit com_ptr(T* ptr) noexcept
		: ptr(ptr)
	{}

	com_ptr(nullptr_t) noexcept {}

	com_ptr(com_ptr&& com_ptr) noexcept
		: ptr(com_ptr.ptr)
	{
		com_ptr.ptr = nullptr;
	}

	com_ptr& operator=(com_ptr&& com_ptr) noexcept
	{
		if (this == &com_ptr) return *this;

		dispose();
		ptr = com_ptr.ptr;
		com_ptr.ptr = nullptr;
		return *this;
	}

	~com_ptr() noexcept
	{
		dispose();
	}


	com_ptr& operator=(T* ptr) noexcept
	{
		dispose();
		this->ptr = ptr;
		return *this;
	}

	com_ptr& operator=(nullptr_t) noexcept
	{
		dispose();
		return *this;
	}

	T& operator*() const noexcept
	{
		return *ptr;
	}

	T* operator->() const noexcept
	{
		return ptr;
	}

	operator bool() const noexcept
	{
		return (ptr != nullptr);
	}

	operator T*() const noexcept
	{
		return ptr;
	}


	// Releases the managed COM object if such is present.
	void dispose() noexcept
	{
		T* temp = ptr;
		if (temp == nullptr) return;

		ptr = nullptr;
		temp->Release();
	}

	// Releases the ownership of the managed COM object and returns a pointer to it.
	// Does not call ptr->Release(). ptr == nullptr after that. 
	T* release_ownership() noexcept
	{
		T* tmp = ptr;
		ptr = nullptr;
		return tmp;
	}

	// Pointer to the managed COM object.
	T* ptr = nullptr;
};

struct hlsl_compute final {

	hlsl_compute() noexcept = default;

	hlsl_compute(ID3D11Device* p_device, const hlsl_compute_desc& desc);

	hlsl_compute(hlsl_compute&& s) noexcept = default;
	hlsl_compute& operator=(hlsl_compute&& s) noexcept = default;


	com_ptr<ID3D11ComputeShader>	p_compute_shader;
	com_ptr<ID3DBlob>				p_compute_shader_bytecode;
};

struct hlsl_shader final {

	hlsl_shader() noexcept = default;

	hlsl_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	hlsl_shader(hlsl_shader&& s) noexcept = default;
	hlsl_shader& operator=(hlsl_shader&& s) noexcept = default;


	com_ptr<ID3D11VertexShader> p_vertex_shader;
	com_ptr<ID3DBlob>			p_vertex_shader_bytecode;
	com_ptr<ID3D11HullShader>	p_hull_shader;
	com_ptr<ID3DBlob>			p_hull_shader_bytecode;
	com_ptr<ID3D11DomainShader> p_domain_shader;
	com_ptr<ID3DBlob>			p_domain_shader_bytecode;
	com_ptr<ID3D11PixelShader>	p_pixel_shader;
	com_ptr<ID3DBlob>			p_pixel_shader_bytecode;

private:

	void init_vertex_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	void init_hull_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	void init_domain_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);

	void init_pixel_shader(ID3D11Device* p_device, const hlsl_shader_desc& desc);
};


template<typename T>
inline bool operator==(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.ptr == r.ptr;
}

template<typename T>
inline bool operator==(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.ptr == nullptr;
}

template<typename T>
inline bool operator==(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.ptr == nullptr;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.ptr != r.ptr;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.ptr != nullptr;
}

template<typename T>
inline bool operator!=(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.ptr != nullptr;
}

com_ptr<ID3DBlob> compile_shader(const std::string& source_code, const std::string& source_filename, 
	uint32_t compile_flags, const char* p_entry_point_name, const char* p_shader_model);

// Creates an unitialized a constant buffer object.
com_ptr<ID3D11Buffer> constant_buffer(ID3D11Device* device, size_t byte_count);

} // namespace sparki
