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
 * The type descriptor stores information about the size of objects, and locations of pointers to other
 * managed objects. It allows iteration through pointer offsets using standard {@link begin} and
 * {@link end} iterators.
 */
class TypeDescriptor
{
	const std::size_t mSize;
	const std::vector<std::ptrdiff_t> mOffsets;
	
public:
	
	/**
	 * Create a TypeDescriptor with the specified size and no pointers.
	 * 
	 * @param size The size of objects of the described type.
	 */
	TypeDescriptor(std::size_t size)
			: mSize(size), mOffsets() {
	}
	
	/**
	 * Create a TypeDescriptor with the specified size and pointer offsets.
	 * 
	 * @param size The size of objects of the described type.
	 * @param offsets The offsets within objects of the described type of pointers to other managed objects.
	 */
	TypeDescriptor(std::size_t size, std::initializer_list<std::ptrdiff_t> offsets)
			: mSize(size), mOffsets(offsets) {
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
