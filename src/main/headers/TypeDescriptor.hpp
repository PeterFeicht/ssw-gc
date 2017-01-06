/**
 * @file    TypeDescriptor.hpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   Defines the {@link TypeDescriptor} class.
 */

#ifndef TYPEDESCRIPTOR_HPP_
#define TYPEDESCRIPTOR_HPP_
#pragma once

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace ssw {

/**
 * Represents a type descriptor for a managed object.
 * 
 * The type descriptor stores information about the size of objects, a pointer to the destructor, and
 * locations of pointers to other managed objects. It allows iteration through pointer offsets using
 * standard {@link begin} and {@link end} iterators.
 */
class TypeDescriptor
{
	using Destructor = void(*)(const void*);
	
	const std::size_t mSize;
	const Destructor mDestructor;
	const std::vector<std::ptrdiff_t> mOffsets;
	
	TypeDescriptor(std::size_t size, Destructor destructor, std::initializer_list<std::ptrdiff_t> offsets)
			: mSize(size), mDestructor(destructor), mOffsets(offsets) {
	}
	
public:
	
	/**
	 * Create a TypeDescriptor for the specified type with the specified pointer offsets.
	 *
	 * @param offsets (optional) The offsets, within `T` objects, of pointers to other managed objects.
	 * @return A TypeDescriptor.
	 *
	 * @tparam T The type to create the descriptor for.
	 */
	template <typename T>
	static TypeDescriptor make(std::initializer_list<std::ptrdiff_t> offsets = {}) {
		return TypeDescriptor(sizeof(T), [](const void* x) { static_cast<const T*>(x)->~T(); }, offsets);
	}
	
	/**
	 * Get the size of the objects described by this type descriptor.
	 */
	std::size_t size() const noexcept {
		return mSize;
	}
	
	/**
	 * Get a reference to the vector of pointer offsets.
	 */
	const std::vector<std::ptrdiff_t>& offsets() const noexcept {
		return mOffsets;
	}
	
	/**
	 * Determine whether the described object has any pointers to other managed objects.
	 */
	explicit operator bool() const noexcept {
		return !mOffsets.empty();
	}
	
	/**
	 * Get an iterator to the beginning of the pointer offset container.
	 * 
	 * @return A `const` iterator to the first pointer offset.
	 */
	auto begin() const noexcept(noexcept(mOffsets.cbegin())) {
		return mOffsets.cbegin();
	}
	
	/**
	 * Get an iterator to the end of the pointer offset container.
	 * 
	 * @return A `const` iterator past the last pointer offset.
	 */
	auto end() const noexcept(noexcept(mOffsets.cend())) {
		return mOffsets.cend();
	}
};

} // namespace ssw

#endif /* TYPEDESCRIPTOR_HPP_ */
