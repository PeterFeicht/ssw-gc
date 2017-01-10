/**
 * @file    Heap.cpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   TODO Defines the {@link Heap} class.
 */

#include "Heap.hpp"

#include <cassert>
#include <utility>
#include <type_traits>

#include "TaggedPointer.hpp"

namespace ssw {

class HeapBase::FreeListNode
{
	std::size_t mSize;
	FreeListNode *mNext;
	
public:
	
	/**
	 * Create a block with the specified size and next pointer, and create an empty type pointer before this
	 * new node.
	 * 
	 * @param size The size of the block.
	 * @param next (optional) The next block in the free list.
	 */
	explicit FreeListNode(std::size_t size, FreeListNode *next = nullptr)
			: mSize(size), mNext(next) {
		// A type descriptor pointer of null signals an unused block
		new(reinterpret_cast<byte*>(this) - sizeof(TypePtr)) TypePtr(nullptr);
	}
	
	void size(std::size_t size) noexcept {
		assert(size > sizeof(FreeListNode));
		mSize = size;
	}
	
	auto size() const noexcept {
		return mSize;
	}
	
	void next(FreeListNode *next) noexcept {
		assert(next != this);
		mNext = next;
	}
	
	auto next() const noexcept {
		return mNext;
	}
	
	auto rawPtr() noexcept {
		return reinterpret_cast<byte*>(this);
	}
};

HeapBase::HeapBase(byte *storage, std::size_t size, std::size_t align) noexcept
		: mFreeList(reinterpret_cast<FreeListNode*>(storage + align)), mAlign(align), mRoots() {
	assert((reinterpret_cast<std::uintptr_t>(storage) & (align - 1)) == 0);
	assert(size >= align + sizeof(FreeListNode));
	// This is probably not strictly necessary, but I don't want to make the offset calculations too complex
	assert(align >= sizeof(TypePtr) && align >= alignof(TypePtr));
	assert(align >= alignof(FreeListNode));
	assert((align & (align - 1)) == 0 /* Alignment must be a power of two */);
	
	new(mFreeList) FreeListNode(size);
}

void* HeapBase::allocate(const TypeDescriptor &type, bool isRoot) noexcept {
	if(mFreeList == nullptr) {
		// There are no free blocks at all, don't even try
		return nullptr;
	}
	
	auto result = this->tryAllocate(type);
	if(!result) {
		// No sufficiently sized block found using first-fit, merge blocks and try again
		this->mergeBlocks();
		result = this->tryAllocate(type);
	}
	if(result && isRoot) {
		this->registerRoot(result);
	}
	return result;
}

void* HeapBase::tryAllocate(const TypeDescriptor &type) noexcept {
	// The minimum size of a block to be split off, including slack
	const std::size_t MinBlockSize = 2 * sizeof(FreeListNode) + mAlign;
	
	FreeListNode *prev = nullptr;
	auto cur = mFreeList;
	// Use first-fit method to find a block
	while(cur && cur->size() < type.size()) {
		prev = std::exchange(cur, cur->next());
	}
	if(cur) {
		if(cur->size() >= type.size() + MinBlockSize) {
			// Split off the block to be returned from the front of the current block
			const auto offset = align(type.size()) + mAlign;
			cur->next(new(cur->rawPtr() + offset) FreeListNode(cur->size() - offset, cur->next()));
			cur->size(align(type.size()));
		}
		if(prev) {
			prev->next(cur->next());
		} else {
			mFreeList = cur->next();
		}
		static_assert(std::is_trivially_destructible<FreeListNode>::value,
				"FreeListNode needs to be destroyed.");
		
		typeDescriptorPtr(cur->rawPtr()) = &type;
		return cur;
	}
	return nullptr;
}

void HeapBase::mergeBlocks() noexcept {
	// TODO implement merging of free blocks
}

void HeapBase::deallocate(byte *block) noexcept {
	auto type = typeDescriptorPtr(block);
	assert(type /* Tried to deallocate an unused block */);
	
	// new(ptr) FreeListNode(...) creates a new TypePtr, make sure the old one doesn't need destruction
	static_assert(std::is_trivially_destructible<TypePtr>::value, "TypePtr needs to be destroyed.");
	
	mFreeList = new(block) FreeListNode(align(type->size()), mFreeList);
}

} // namespace ssw
