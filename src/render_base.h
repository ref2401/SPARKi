#pragma once

#include "math/math.h"
#include <windows.h>


namespace sparki {

// Unique_com_ptr is a smart pointer that owns and manages a COM object through a pointer 
// and disposes of that object when the Unique_com_ptr goes out of scope.
template<typename T>
struct com_ptr final {

	static_assert(std::is_base_of<IUnknown, T>::value, "T must be derived from IUnknown.");


	com_ptr() noexcept = default;

	explicit com_ptr(T* p_ptr) noexcept
		: p_ptr(p_ptr)
	{}

	com_ptr(nullptr_t) noexcept {}

	com_ptr(com_ptr&& com_ptr) noexcept
		: p_ptr(com_ptr.p_ptr)
	{
		com_ptr.p_ptr = nullptr;
	}

	com_ptr& operator=(com_ptr&& com_ptr) noexcept
	{
		if (this == &com_ptr) return *this;

		dispose();
		p_ptr = com_ptr.p_ptr;
		com_ptr.p_ptr = nullptr;
		return *this;
	}

	~com_ptr() noexcept
	{
		dispose();
	}


	com_ptr& operator=(T* p_ptr) noexcept
	{
		dispose();
		this->p_ptr = p_ptr;
		return *this;
	}

	com_ptr& operator=(nullptr_t) noexcept
	{
		dispose();
		return *this;
	}

	T& operator*() const noexcept
	{
		return *p_ptr;
	}

	T* operator->() const noexcept
	{
		return p_ptr;
	}

	operator bool() const noexcept
	{
		return (p_ptr != nullptr);
	}

	operator T*() const noexcept
	{
		return p_ptr;
	}


	// Releases the managed COM object if such is present.
	void dispose() noexcept;

	// Releases the ownership of the managed COM object and returns a pointer to it.
	// Does not call p_ptr->Release(). p_ptr == nullptr after that. 
	T* release_ownership() noexcept
	{
		T* tmp = p_ptr;
		p_ptr = nullptr;
		return tmp;
	}

	// Pointer to the managed COM object.
	T* p_ptr = nullptr;
};

template<typename T>
void com_ptr<T>::dispose() noexcept
{
	T* temp = p_ptr;
	if (temp == nullptr) return;

	p_ptr = nullptr;
	auto c = temp->Release();
}


template<typename T>
inline bool operator==(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.p_ptr == r.p_ptr;
}

template<typename T>
inline bool operator==(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.p_ptr == nullptr;
}

template<typename T>
inline bool operator==(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.p_ptr == nullptr;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.p_ptr != r.p_ptr;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.p_ptr != nullptr;
}

template<typename T>
inline bool operator!=(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.p_ptr != nullptr;
}


} // namespace sparki
