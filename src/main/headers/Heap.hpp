/**
 * @file    Heap.hpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   Declares and defines the {@link Heap} class.
 */

#ifndef HEAP_HPP_
#define HEAP_HPP_
#pragma once

#include <cstddef>
#include <ostream>

#include "TypeDescriptor.hpp"

namespace ssw {

using byte = unsigned char;

class HeapBase
{
	class FreeListNode;
	
	byte *mFreeList;
	const std::size_t mAlign;
	
protected:
	
	/**
	 * Initialize this heap with the specified storage and alignment.
	 * 
	 * @param storage Pointer to the storage to use for objects.
	 * @param size Raw size of the storage.
	 * @param align The alignment to use for allocated objects, `storage` must have at least that alignment.
	 */
	HeapBase(byte *storage, std::size_t size, std::size_t align) noexcept;
	
	/**
	 * Try to allocate a block of memory for the specified type.
	 * 
	 * @param type Type descriptor for the memory to allocate.
	 * @return A pointer to the allocated memory block, or `nullptr` if the allocation failed.
	 */
	void* tryAllocate(const TypeDescriptor &type) noexcept;
	
	/**
	 * Merge free blocks and build a new free list.
	 */
	void mergeBlocks() noexcept;
	
	/**
	 * Deallocate the specified block by putting it back into the free list. No destructors are called.
	 * 
	 * @param block Pointer to the block to deallocate.
	 */
	void deallocate(byte *block) noexcept;
	
	/**
	 * Align the specified offset to the heap alignment.
	 * 
	 * @param offset The offset to align.
	 * @return The offset aligned to the next larger multiple of this heap's alignment.
	 */
	std::size_t align(std::size_t offset) noexcept {
		return (offset + mAlign - 1) & ~(mAlign - 1);
	}
	
	/**
	 * Set the head of the free list to the specified value.
	 * 
	 * @param newHead Pointer to the new head of the free list.
	 */
	void freeList(FreeListNode *newHead) noexcept {
		mFreeList = reinterpret_cast<byte*>(newHead);
	}
	
	/**
	 * Get the head of the free list.
	 * 
	 * @return Pointer to the first node in the free list, `nullptr` if there are no free blocks.
	 */
	FreeListNode* freeList() noexcept {
		return reinterpret_cast<FreeListNode*>(mFreeList);
	}
	
	/**
	 * Get a reference to the type descriptor pointer from an object address.
	 * 
	 * @param ptr The object address (pointer to a managed object).
	 * @return A reference to the pointer to the type descriptor for the object.
	 */
	static const TypeDescriptor*& typeDescriptorPtr(byte *ptr) noexcept {
		return *reinterpret_cast<const TypeDescriptor**>(ptr - sizeof(TypeDescriptor*));
	}
	
public:

	/**
	 * Dump the contents of this heap to the specified stream.
	 * 
	 * The dump contains a list of live objects, a list of free blocks, and overall statistics.
	 * 
	 * @param os The output stream to write to.
	 */
	void dump(std::ostream &os) const;
};

template <std::size_t HeapSize, std::size_t Align = alignof(std::max_align_t)>
class Heap : public HeapBase
{
	// Make storage slightly bigger to allow for type descriptor pointer
	alignas(Align)
	byte mStorage[HeapSize + Align];
	
public:
	
	/** The alignment of this heap. */
	static constexpr auto alignment = Align;
	
	/** The size of this heap. */
	static constexpr auto size = HeapSize;
	
	Heap() noexcept : HeapBase(mStorage, HeapSize + Align, Align) {
	}
	
	/**
	 * Allocate a block of memory for the specified type.
	 * 
	 * @param type Type descriptor for the memory to allocate.
	 * @return A pointer to the allocated memory block, or `nullptr` if the allocation failed.
	 */
	void* allocate(const TypeDescriptor &type) noexcept {
		if(this->freeList() == nullptr) {
			// There are no free blocks at all, don't even try
			return nullptr;
		}
		
		auto result = this->tryAllocate(type);
		if(!result) {
			// No sufficiently sized block found using first-fit, merge blocks and try again
			this->mergeBlocks();
			result = this->tryAllocate(type);
		}
		return result;
	}
	
	/**
	 * Allocate a block of memory for the specified type.
	 * 
	 * @return Pointer to the newly allocated and uninitialized object, or `nullptr` if the allocation
	 *         failed.
	 * 
	 * @tparam The type to allocate memory for, must have a static member variable `type` that holds the
	 *         type descriptor to use.
	 */
	template <typename T>
	T* allocate() noexcept {
		return reinterpret_cast<T*>(allocate(T::type));
	}
};

} // namespace ssw

#endif /* HEAP_HPP_ */
