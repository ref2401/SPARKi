#pragma once

#include "math/math.h"
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

	explicit com_ptr(T* p_handle) noexcept
		: p_handle(p_handle)
	{}

	com_ptr(nullptr_t) noexcept {}

	com_ptr(com_ptr&& com_ptr) noexcept
		: p_handle(com_ptr.p_handle)
	{
		com_ptr.p_handle = nullptr;
	}

	com_ptr& operator=(com_ptr&& com_ptr) noexcept
	{
		if (this == &com_ptr) return *this;

		dispose();
		p_handle = com_ptr.p_handle;
		com_ptr.p_handle = nullptr;
		return *this;
	}

	~com_ptr() noexcept
	{
		dispose();
	}


	com_ptr& operator=(T* p_handle) noexcept
	{
		dispose();
		this->p_handle = p_handle;
		return *this;
	}

	com_ptr& operator=(nullptr_t) noexcept
	{
		dispose();
		return *this;
	}

	T& operator*() const noexcept
	{
		return *p_handle;
	}

	T* operator->() const noexcept
	{
		return p_handle;
	}

	operator bool() const noexcept
	{
		return (p_handle != nullptr);
	}

	operator T*() const noexcept
	{
		return p_handle;
	}


	// Releases the managed COM object if such is present.
	void dispose() noexcept;

	// Releases the ownership of the managed COM object and returns a pointer to it.
	// Does not call p_handle->Release(). p_handle == nullptr after that. 
	T* release_ownership() noexcept
	{
		T* tmp = p_handle;
		p_handle = nullptr;
		return tmp;
	}

	// Pointer to the managed COM object.
	T* p_handle = nullptr;
};

template<typename T>
void com_ptr<T>::dispose() noexcept
{
	T* temp = p_handle;
	if (temp == nullptr) return;

	p_handle = nullptr;
	temp->Release();
}


template<typename T>
inline bool operator==(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.p_handle == r.p_handle;
}

template<typename T>
inline bool operator==(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.p_handle == nullptr;
}

template<typename T>
inline bool operator==(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.p_handle == nullptr;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.p_handle != r.p_handle;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.p_handle != nullptr;
}

template<typename T>
inline bool operator!=(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.p_handle != nullptr;
}

} // namespace sparki
