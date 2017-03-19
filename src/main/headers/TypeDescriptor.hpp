/**
 * @file    TypeDescriptor.hpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   Declares the {@link TypeDescriptor} class.
 */

#ifndef TYPEDESCRIPTOR_HPP_
#define TYPEDESCRIPTOR_HPP_
#pragma once

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>

#include <boost/type_index.hpp>

namespace ssw {

/**
 * Represents a type descriptor for a managed object.
 * 
 * The type descriptor stores information about the size of objects, a pointer to the destructor, and
 * locations of pointers to other managed objects. It allows iteration through pointer offsets using
 * standard {@link begin} and {@link end} iterators.
 * 
 * Since the size of a type descriptor depends on the number of pointers in the described type, but it must
 * be a standard-layout type (for the Deutsch-Schorr-Waite garbage collector), it can only be allocated on
 * the heap and uses some low-level pointer magic for iteration. 
 */
class TypeDescriptor
{
	using Destructor = void(*)(const void*);
	
	class AllocTag {};
	
	const std::string mName;
	const std::size_t mSize;
	const Destructor mDestructor;
	const std::size_t mOffsets;
	
	TypeDescriptor(std::string name, std::size_t size, Destructor destructor,
			std::initializer_list<std::ptrdiff_t> offsets);
	
public:
	
	/**
	 * Allocate memory for a type descriptor and the specified number of pointer offsets.
	 * 
	 * @param size The size of the TypeDescriptor object.
	 * @param offsets The number of pointer offsets to be placed after the object.
	 */
	void* operator new(std::size_t size, std::size_t offsets, AllocTag);
	
	/**
	 * Create a TypeDescriptor for the specified type with the specified pointer offsets.
	 *
	 * @param offsets (optional) The offsets, within `T` objects, of pointers to other managed objects.
	 * @return A pointer to the created TypeDescriptor.
	 *
	 * @tparam T The type to create the descriptor for.
	 */
	template <typename T>
	static TypeDescriptor* make(std::initializer_list<std::ptrdiff_t> offsets = {}) {
		return new(offsets.size(), AllocTag{}) TypeDescriptor(
				boost::typeindex::type_id<T>().pretty_name(),
				sizeof(T), [](const void *x) { static_cast<const T*>(x)->~T(); }, offsets);
	}
	
	/**
	 * Get the name of the described type.
	 */
	const std::string& name() const noexcept {
		return mName;
	}
	
	/**
	 * Get the size of the objects described by this type descriptor.
	 */
	std::size_t size() const noexcept {
		return mSize;
	}
	
	/**
	 * Get the number of pointer offsets.
	 */
	auto offsets() const noexcept {
		return mOffsets;
	}
	
	/**
	 * Determine whether the described object has any pointers to other managed objects.
	 */
	explicit operator bool() const noexcept {
		return mOffsets;
	}
	
	/**
	 * Get an iterator to the beginning of the pointer offsets.
	 * 
	 * @return A `const` iterator to the first pointer offset.
	 */
	const std::ptrdiff_t* begin() const noexcept;
	
	/**
	 * Get an iterator to the end of the pointer offsets.
	 * 
	 * @return A `const` iterator past the last pointer offset.
	 */
	const std::ptrdiff_t* end() const noexcept {
		return this->begin() + mOffsets;
	}
	
	/**
	 * Destroy the specified object.
	 * 
	 * @param object The object to be destroyed.
	 */
	void destroy(void *object) const noexcept {
		mDestructor(object);
	}
}; // class TypeDescriptor

} // namespace ssw

#endif /* TYPEDESCRIPTOR_HPP_ */
