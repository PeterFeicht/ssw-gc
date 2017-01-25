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
	class Block;
	
	Block *mFreeList;
	Block* const mHeapStart;
	Block* const mHeapEnd;
	std::vector<byte*> mRoots;
	
protected:
	
	struct HeapStats {
		/** Total size of the heap in bytes. */
		std::size_t heapSize;
		/** Size of used objects in bytes (including overhead). */
		std::size_t usedSize;
		/** Size of free blocks in bytes (including overhead). */
		std::size_t freeSize;
		
		/** The number of blocks in the free list. */
		std::size_t numFreeBlocks;
		/** The total size available for objects in all blocks. */
		std::size_t freeBlockSize;
		
		/** The number of objects in the heap (dead or alive). */
		std::size_t numObjects;
		/** The net size of all objects (not including overhead). */
		std::size_t objectSize;
		/** The number of live objects in the heap. */
		std::size_t numLiveObjects;
		/** The net size of live objects (not including overhead). */
		std::size_t liveObjectSize;
	};
	
	/**
	 * Initialize this heap with the specified storage.
	 * 
	 * @param storage Pointer to the storage to use for objects.
	 * @param size Raw size of the storage.
	 */
	HeapBase(byte *storage, std::size_t size) noexcept;
	
	/**
	 * Collect statistics for this heap.
	 * 
	 * @param countLiveObjects (optional) Whether to count live objects (this requires marking the whole
	 *                         heap so is slower when enabled).
	 * @return HeapStats object, live object count and size are set to 0 if `countLiveObjects` is `false`.
	 */
	HeapStats collectHeapStats(bool countLiveObjects = false) noexcept;
	
public:

	/**
	 * The alignment of memory allocated from this heap.
	 */
	static constexpr std::size_t Align = alignof(std::max_align_t);
	
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
	 * Deallocate the specified object by putting it back into the free list. No destructors are called.
	 * 
	 * This function may be used by `operator delete` of managed objects to free memory immediately instead
	 * of waiting for the next garbage collection. It is not used during garbage collection.
	 * 
	 * @param block Pointer to the object to deallocate.
	 */
	void deallocate(byte *obj) noexcept;
	
	/**
	 * Register the specified object as a heap root for garbage collection.
	 * 
	 * @param object Pointer to the object to register.
	 */
	void registerRoot(void *object) {
		mRoots.push_back(reinterpret_cast<byte*>(object));
	}
	
	// TODO Add way to remove heap roots
	
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
	void dump(std::ostream &os);
	
private:
	
	/**
	 * Get a reference to the block for an object address.
	 * 
	 * @param ptr The object address (pointer to a managed object; pointer to the block's data portion).
	 * @return A reference to the block object.
	 */
	static Block& block(byte *ptr) noexcept {
		return *reinterpret_cast<Block*>(ptr - Align);
	}
	
	/**
	 * Align the specified object size to the heap alignment.
	 * 
	 * @param offset The object size to align.
	 * @return The offset aligned to the next larger multiple of this heap's alignment.
	 */
	static std::size_t align(std::size_t size) noexcept {
		return (size + Align - 1) & ~(Align - 1);
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
	 * Perform marking for the garbage collector on the specified heap root.
	 * 
	 * @param root Pointer to an object whose object graph should be marked.
	 */
	void mark(byte *root) noexcept;
	
	/**
	 * Rebuild the free list with marked objects and destroy unmarked objects.
	 */
	void rebuildFreeList() noexcept;
	
	/**
	 * Write a list of live objects to the specified stream.
	 * 
	 * @param os The output stream to write to.
	 */
	void dumpLiveObjects(std::ostream &os);
};

template <std::size_t HeapSize>
class Heap : public HeapBase
{
	static constexpr std::size_t Align = alignof(std::max_align_t);
	
	// Make storage slightly bigger to allow for type descriptor pointer
	alignas(Align)
	byte mStorage[HeapSize + Align];
	
public:
	
	/** The size of this heap. */
	static constexpr auto size = HeapSize;
	
	Heap() noexcept : HeapBase(mStorage, std::extent<decltype(mStorage)>::value) {
	}
};

} // namespace ssw

#endif /* HEAP_HPP_ */
