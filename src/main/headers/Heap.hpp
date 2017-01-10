/**
 * @file    Heap.hpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   Declares the {@link Heap} and {@link HeapBase} classes.
 */

#ifndef HEAP_HPP_
#define HEAP_HPP_
#pragma once

#include <cstddef>
#include <ostream>
#include <vector>

#include "TaggedPointer.hpp"
#include "TypeDescriptor.hpp"

namespace ssw {

using byte = unsigned char;

class HeapBase
{
	using TypePtr = TaggedPointer;
	
	class FreeListNode;
	
	FreeListNode *mFreeList;
	const std::size_t mAlign;
	std::vector<byte*> mRoots;
	
protected:
	
	/**
	 * Initialize this heap with the specified storage and alignment.
	 * 
	 * @param storage Pointer to the storage to use for objects.
	 * @param size Raw size of the storage.
	 * @param align The alignment to use for allocated objects, `storage` must have at least that alignment.
	 */
	HeapBase(byte *storage, std::size_t size, std::size_t align) noexcept;
	
public:
	
	/**
	 * Allocate a block of memory for the specified type.
	 * 
	 * @param type Type descriptor for the memory to allocate.
	 * @param isRoot (optional) Whether to register the allocated object as a heap root.
	 * @return A pointer to the allocated memory block, or `nullptr` if the allocation failed.
	 */
	void* allocate(const TypeDescriptor &type, bool isRoot = false) noexcept;
	
	/**
	 * Allocate a block of memory for the specified type.
	 * 
	 * @param isRoot (optional) Whether to register the allocated object as a heap root.
	 * @return Pointer to the newly allocated and uninitialized object, or `nullptr` if the allocation
	 *         failed.
	 * 
	 * @tparam The type to allocate memory for, must have a static member variable `type` that holds the
	 *         type descriptor to use.
	 */
	template <typename T>
	T* allocate(bool isRoot = false) noexcept {
		assert(T::type.size() >= sizeof(T));
		return reinterpret_cast<T*>(this->allocate(T::type, isRoot));
	}
	
	/**
	 * Deallocate the specified block by putting it back into the free list. No destructors are called.
	 * 
	 * This function may be used by `operator delete` of managed objects to free memory immediately instead
	 * of waiting for the next garbage collection. It is not used during garbage collection.
	 * 
	 * @param block Pointer to the block to deallocate.
	 */
	void deallocate(byte *block) noexcept;
	
	/**
	 * Register the specified object as a heap root for garbage collection.
	 * 
	 * @param object Pointer to the object to register.
	 */
	void registerRoot(void *object) {
		mRoots.push_back(reinterpret_cast<byte*>(object));
	}
	
	/**
	 * Run garbage collection on this heap.
	 * 
	 * The implemented algorithm is the Deutsch-Schorr-Waite mark and sweep collector, i.e. a non-moving
	 * collector, which uses the registered heap roots to find living objects.
	 */
	void gc() noexcept;

	/**
	 * Dump the contents of this heap to the specified stream.
	 * 
	 * The dump contains a list of live objects, a list of free blocks, and overall statistics.
	 * 
	 * @param os The output stream to write to.
	 */
	void dump(std::ostream &os) const;
	
private:
	
	/**
	 * Get a reference to the type descriptor pointer from an object address. The type descriptor pointer
	 * for that address must already have been created before.
	 * 
	 * @param ptr The object address (pointer to a managed object).
	 * @return A reference to the pointer to the type descriptor for the object.
	 */
	static TypePtr& typeDescriptorPtr(byte *ptr) noexcept {
		return *reinterpret_cast<TypePtr*>(ptr - sizeof(TypePtr));
	}
	
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
	 * Align the specified offset to the heap alignment.
	 * 
	 * @param offset The offset to align.
	 * @return The offset aligned to the next larger multiple of this heap's alignment.
	 */
	std::size_t align(std::size_t offset) noexcept {
		return (offset + mAlign - 1) & ~(mAlign - 1);
	}
	
	/**
	 * Perform marking for the garbage collector on the specified heap root.
	 * 
	 * @param root Pointer to an object whose object graph should be marked.
	 */
	void mark(byte *root) noexcept;
	
	/**
	 * Rebuild the free list with marked objects and destroy unmarked objects.
	 */
	void rebuildFreeList() noexcept;
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
};

} // namespace ssw

#endif /* HEAP_HPP_ */
