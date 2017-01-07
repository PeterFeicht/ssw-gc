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
	
	explicit FreeListNode(std::size_t size, FreeListNode *next = nullptr)
			: mSize(size), mNext(next) {
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
		: mFreeList(storage + align), mAlign(align) {
	assert(size >= align + sizeof(FreeListNode));
	// This is probably not strictly necessary, but I don't want to make the offset calculations too complex
	assert(align >= sizeof(TypeDescriptor*) && align >= alignof(TypeDescriptor*));
	assert(align >= alignof(FreeListNode));
	assert((align & (align - 1)) == 0 /* Alignment must be a power of two */);
	
	// A type descriptor pointer of null signals an unused block
	typeDescriptorPtr(mFreeList) = nullptr;
	new(mFreeList) FreeListNode(size);
}

void* HeapBase::tryAllocate(const TypeDescriptor &type) noexcept {
	// The minimum size of a block to be split off, including slack
	const std::size_t MinBlockSize = 2 * sizeof(FreeListNode) + mAlign;
	
	if(this->freeList() == nullptr) {
		// There are no free blocks at all, don't even try
		return nullptr;
	}
	
	FreeListNode *prev = nullptr;
	auto cur = this->freeList();
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
			this->freeList(cur->next());
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
	auto type = std::exchange(typeDescriptorPtr(block), nullptr);
	assert(type != nullptr /* Tried to deallocate an unused block */);
	
	this->freeList(new(block) FreeListNode(align(type->size()), this->freeList()));
}

} // namespace ssw
