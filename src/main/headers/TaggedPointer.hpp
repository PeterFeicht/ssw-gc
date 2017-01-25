/**
 * @file    TaggedPointer.hpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   Declares and defines the {@link TaggedPointer} class.
 */

#ifndef TAGGEDPOINTER_HPP_
#define TAGGEDPOINTER_HPP_
#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace ssw {

class TaggedPointer
{
	static constexpr std::uintptr_t sMaskMark{1};
	static constexpr std::uintptr_t sMaskFree{2};
	static constexpr std::uintptr_t sMaskAll{sMaskMark | sMaskFree};
	
	std::uintptr_t mPointer;
	
public:
	/**
	 * Default construct a TaggedPointer with a null pointer.
	 */
	constexpr TaggedPointer() noexcept
			: mPointer(reinterpret_cast<std::uintptr_t>(nullptr)) {
	}
	
	/**
	 * Construct a TaggedPointer from a null pointer.
	 * 
	 * @param A null pointer.
	 */
	constexpr TaggedPointer(std::nullptr_t) noexcept
			: mPointer(reinterpret_cast<std::uintptr_t>(nullptr)) {
	}
	
	/**
	 * Construct a TaggedPointer from the specified pointer.
	 * 
	 * @param ptr The value of the pointer.
	 * @param free (optional) Whether the free bit should be set, defaults to `false`.
	 * 
	 * @tparam T The type of the pointer being stored.
	 */
	template <typename T, typename = std::enable_if_t<alignof(T) >= 4>>
	explicit TaggedPointer(T *ptr, bool free = false) noexcept
			: mPointer(reinterpret_cast<std::uintptr_t>(ptr)) {
		assert((mPointer & sMaskAll) == 0);
		this->free(free);
	}
	
	/**
	 * Copy construct a TaggedPointer from another one.
	 * 
	 * @param The TaggedPointer to copy. 
	 */
	TaggedPointer(const TaggedPointer&) = default;
	
	/**
	 * Move construct a TaggedPointer from another one.
	 * 
	 * @param The TaggedPointer to move from.
	 */
	TaggedPointer(TaggedPointer&&) = default;
	
	/**
	 * Destroy this TaggedPointer.
	 */
	~TaggedPointer() = default;
	
	/**
	 * Assign the specified pointer to this TaggedPointer. The tags are preserved.
	 * 
	 * @param ptr The value to assign.
	 * @return A reference to this object.
	 * 
	 * @tparam T The type of the pointer being stored.
	 */
	template <typename T, typename = std::enable_if_t<alignof(T) >= 4>>
	TaggedPointer& operator=(T *ptr) noexcept {
		assert((reinterpret_cast<std::uintptr_t>(ptr) & sMaskAll) == 0);
		mPointer = reinterpret_cast<std::uintptr_t>(ptr) | (mPointer & sMaskAll);
		return *this;
	}
	
	/**
	 * Copy assign this TaggedPointer from another one.
	 * 
	 * @param The TaggedPointer to copy. 
	 * @return A reference to this object.
	 */
	TaggedPointer& operator=(const TaggedPointer&) = default;
	
	/**
	 * Move assign this TaggedPointer from another one.
	 * 
	 * @param The TaggedPointer to move from. 
	 * @return A reference to this object.
	 */
	TaggedPointer& operator=(TaggedPointer&&) = default;
	
	/**
	 * Set or clear the mark.
	 * 
	 * @param mark `true` to set the mark, `false` to clear it.
	 */
	void mark(bool mark) noexcept {
		if(mark) {
			mPointer |= sMaskMark;
		} else {
			mPointer &= ~sMaskMark;
		}
	}
	
	/**
	 * Get the mark.
	 * 
	 * @return `true` if the mark is set, `false` otherwise.
	 */
	bool mark() const noexcept {
		return (mPointer & sMaskMark);
	}
	
	/**
	 * Set or clear the free flag.
	 * 
	 * @param mark `true` to set the free flag, `false` to clear it.
	 */
	void free(bool free) noexcept {
		if(free) {
			mPointer |= sMaskFree;
		} else {
			mPointer &= ~sMaskFree;
		}
	}
	
	/**
	 * Get the free flag.
	 * 
	 * @return `true` if the free flag is set, `false` otherwise.
	 */
	bool free() const noexcept {
		return (mPointer & sMaskFree);
	}
	
	/**
	 * Get the negation of the free flag.
	 * 
	 * @return `!this->free()`.
	 */
	bool used() const noexcept {
		return !this->free();
	}
	
	/**
	 * Get the wrapped pointer value.
	 * 
	 * @return The value of this pointer.
	 * 
	 * @tparam T The type of the pointer being stored. This must be the same type that was used for the
	 *           construction of this pointer, or for the most recent assignment, if this pointer was
	 *           assigned another value since construction.
	 */
	template <typename T, typename = std::enable_if_t<alignof(T) >= 4>>
	T* get() const noexcept {
		return reinterpret_cast<T*>(mPointer & ~sMaskAll);
	}
	
	/**
	 * Test whether this pointer contains a value (is not `null`).
	 */
	explicit operator bool() const noexcept {
		return (mPointer & ~sMaskAll);
	}
	
	/**
	 * Swap the values of this TaggedPointer and another one.
	 * 
	 * @param other The TaggedPointer to swap values with.
	 */
	void swap(TaggedPointer &other) noexcept {
		std::swap(mPointer, other.mPointer);
	}
};

/**
 * Swap the specified {@link TaggedPointer TaggedPointers}. Calls `lhs.swap(rhs)`.
 * 
 * @param lhs The first TaggedPointer to swap.
 * @param rhs The other TaggedPointer to swap.
 */
inline void swap(TaggedPointer &lhs, TaggedPointer &rhs) noexcept {
	lhs.swap(rhs);
}

} // namespace ssw

#endif /* TAGGEDPOINTER_HPP_ */
