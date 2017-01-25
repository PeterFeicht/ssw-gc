/**
 * @file    Heap.cpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   Defines the {@link Heap} and {@link HeapBase} classes.
 */

#include "Heap.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iterator>
#include <utility>
#include <type_traits>

#include "RestoreStream.hpp"
#include "TaggedPointer.hpp"

namespace ssw {

/**
 * Represents a block of memory in the heap. This class holds the block size and either a pointer to the
 * type of the stored object (in case the block is used) or a pointer to the next free block.
 */
class alignas(HeapBase::Align) HeapBase::Block
{
	std::size_t mSize;
	TaggedPointer mPtr;
	
public:
	
	/**
	 * Initialize a new, free block with the specified size.
	 * 
	 * @param size The usable size of the block (will be properly aligned).
	 * @param next (optional) The next block in the free list.
	 */
	explicit Block(std::size_t size, Block *next = nullptr) noexcept 
			: mSize(size), mPtr(next) {
		assert(size >= Align);
		mPtr.free(true);
	}
	
	/**
	 * Get the data size of this block.
	 * 
	 * @return The usable size of this block, not including the block descriptor.
	 */
	std::size_t size() const noexcept {
		return mSize;
	}
	
	/**
	 * Mark this block as free and set the next block in the free list.
	 * 
	 * @param next The next block in the free list, or `nullptr`.
	 */
	void next(Block *next) noexcept {
		assert(next != this);
		mPtr = next;
		mPtr.free(true);
	}
	
	/**
	 * Mark this block as free and set its successor and size.
	 * 
	 * @param next The next block in the free list, or `nullptr`.
	 * @param size The usable size of this block, not including the block descriptor.
	 */
	void next(Block *next, std::size_t size) noexcept {
		this->next(next);
		assert(size >= Align);
		mSize = size;
	}
	
	/**
	 * Get the next block in the free list. This block must represent a free block.
	 * 
	 * @return Pointer to the next block in the free list.
	 */
	Block* next() const noexcept {
		assert(this->free() && !this->mark());
		return mPtr.get<Block>();
	}
	
	/**
	 * Get a pointer to the block following this block in the heap.
	 * 
	 * @return Pointer to the physically next block in the heap.
	 */
	Block* following() const noexcept {
		return reinterpret_cast<Block*>(this->data() + align(mSize));
	}
	
	/**
	 * Mark this block as used and set the data type.
	 * 
	 * @param type The type descriptor for the data in this block.
	 */
	void type(const TypeDescriptor &type) noexcept {
		mPtr = &type;
		mPtr.free(false);
	}
	
	/**
	 * Get the data type in this block. This block must represent a used block.
	 * 
	 * @return Reference to the type descriptor for the data in this block.
	 */
	const TypeDescriptor& type() const noexcept {
		assert(this->used() && !this->mark());
		return *mPtr.get<const TypeDescriptor>();
	}
	
	/**
	 * Get whether this block is free.
	 * 
	 * @return `true` if this block is part of the free list and does not hold an object.
	 */
	bool free() const noexcept {
		return mPtr.free();
	}
	
	/**
	 * Get whether this block is used.
	 * 
	 * @return `true` if this block contains an object and is not part of the free list.
	 */
	bool used() const noexcept {
		return mPtr.used();
	}
	
	/**
	 * Get the GC mark for this block.
	 * 
	 * @return `true` if the mark is set, `false` otherwise.
	 */
	bool mark() const noexcept {
		return mPtr.mark();
	}
	
	/**
	 * Get the pointer in this block.
	 * 
	 * @return Reference to the pointer.
	 */
	TaggedPointer& ptr() noexcept {
		return mPtr;
	}
	
	/**
	 * Get a pointer to the data portion of this block.
	 * 
	 * @return Pointer to the data portion of this block.
	 */
	byte* data() const noexcept {
		return const_cast<byte*>(reinterpret_cast<const byte*>(this) + Align);
	}
	
	/**
	 * Split this block in two blocks if possible. If there is enough space for another block, then this
	 * block will be resized to the new size and a new block will be added after it; if there is not enough
	 * space then nothing will be changed. This block must be a free block.
	 * 
	 * @param newSize The new size of this block, will be properly aligned.
	 */
	void split(std::size_t newSize) noexcept {
		assert(this->free());
		const std::size_t restSize = align(mSize) - align(newSize) - Align;
		if(restSize >= Align) {
			Block *newBlock = reinterpret_cast<Block*>(reinterpret_cast<byte*>(this) + Align + align(newSize));
			new(newBlock) Block(restSize, mPtr.get<Block>());
			mPtr = newBlock;
		}
	}
};

HeapBase::HeapBase(byte *storage, std::size_t size) noexcept
		: mFreeList(reinterpret_cast<Block*>(storage)),
		  mHeapStart(reinterpret_cast<Block*>(storage)),
		  mHeapEnd(reinterpret_cast<Block*>(storage + (size & ~(Align - 1)))),
		  mRoots() {
	static_assert(sizeof(Block) <= Align, "Block class is too large");

	assert((reinterpret_cast<std::uintptr_t>(storage) & (Align - 1)) == 0);
	assert(size >= 2 * Align);
	
	new(mFreeList) Block(size - Align);
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
	Block *prev = nullptr;
	Block *cur = mFreeList;
	// Use first-fit method to find a block
	while(cur && cur->size() < type.size()) {
		prev = std::exchange(cur, cur->next());
	}
	if(cur) {
		cur->split(type.size());
		if(prev) {
			prev->next(cur->next());
		} else {
			mFreeList = cur->next();
		}
		cur->type(type);
		return cur->data();
	}
	return nullptr;
}

void HeapBase::mergeBlocks() noexcept {
	// TODO implement merging of free blocks
}

void HeapBase::deallocate(byte *obj) noexcept {
	Block &blk = block(obj);
	assert(blk.used() /* Tried to deallocate an unused block */);
	assert(!blk.mark() /* Tried to deallocate during garbage collection (GC builds a new free list itself) */);
	
	blk.next(mFreeList);
	mFreeList = &blk;
}

void HeapBase::gc() noexcept {
	for(auto root : mRoots) {
		this->mark(root);
	}
	this->rebuildFreeList();
}

// Mark the object graph for the specified heap root object using the Deutsch-Schorr-Waite marking algorithm
void HeapBase::mark(byte *root) noexcept {
	assert(root);
	assert(!block(root).mark());
	
	byte *cur = root;
	byte *prev = nullptr;
	while(true) {
		auto &blk = block(cur);
		if(!blk.mark()) {
			// Mark the object and begin iteration
			blk.ptr() = blk.type().begin();
			blk.ptr().mark(true);
		} else {
			blk.ptr() = blk.ptr().get<const std::ptrdiff_t>() + 1;
		}
		
		auto offset = *blk.ptr().get<const std::ptrdiff_t>();
		if(offset >= 0) {
			// Advance
			auto &field = *reinterpret_cast<byte**>(cur + offset);
			if(field && !block(field).mark()) {
				auto tmp = std::exchange(field, prev);
				prev = std::exchange(cur, tmp);
			}
		} else {
			// Retreat
			blk.ptr() = reinterpret_cast<const TypeDescriptor*>(
					reinterpret_cast<const byte*>(blk.ptr().get<const std::ptrdiff_t>()) + offset);
			if(!prev) {
				return;
			}
			auto tmp = std::exchange(cur, prev);
			offset = *block(cur).ptr().get<std::ptrdiff_t>();
			prev = std::exchange(*reinterpret_cast<byte**>(cur + offset), tmp);
		}
	} // while(true)
}

// Rebuild the free list while destroying garbage objects
void HeapBase::rebuildFreeList() noexcept {
	Block *freeList = nullptr;
	
	for(Block *blk = mHeapStart; blk < mHeapEnd;) {
		if(blk->mark()) {
			blk->ptr().mark(false);
			blk = blk->following();
			
		} else {
			auto free = blk;
			// Extend the free block, destroying garbage objects as necessary
			do {
				if(free->used()) {
					free->type().destroy(free->data());
				}
				free = free->following();
			} while(free < mHeapEnd && !free->mark());

			static_assert(std::is_trivially_destructible<Block>::value, "Block must be trivially destructible.");
			blk->next(freeList, reinterpret_cast<byte*>(free) - reinterpret_cast<byte*>(blk) - Align);
			freeList = blk;
			blk = free;
		}
	}
	mFreeList = freeList;
}

void HeapBase::dump(std::ostream &os) {
	RestoreStream osRestore{os};
	
	const HeapStats stats = this->collectHeapStats(true);
	
	os << "==== Statistics for heap at " << std::hex << mHeapStart << std::dec << " ====\n";
	os << "Heap size:  " << stats.heapSize << " bytes\n";
	os << "Used space: " << stats.usedSize << " bytes\n";
	os << "Free space: " << stats.freeSize << " bytes\n";
	os << '\n';
	os << "Object count:    " << stats.numObjects << " (" << stats.numLiveObjects << " live)\n";
	os << "Object size:     " << stats.objectSize << " bytes (" << stats.liveObjectSize << " in live objects)\n";
	os << "Available space: " << stats.freeBlockSize << " bytes in " << stats.numFreeBlocks << " blocks\n";
	os << '\n';
	os << "= Free Blocks =\nAddress    Size(net)\n";
	
	// Print free blocks: just use the free list
	os.fill('0');
	for(auto blk = mFreeList; blk; blk = blk->next()) {
		os << std::hex << std::setw(sizeof(void*)) << blk
				<< ' ' << std::dec << blk->size() << '\n';
	}
	os << std::setfill(' ') << '\n';
	
	os << "= Live Objects =\n";
	// For printing live objects we need to do marking
	this->dumpLiveObjects(os);
}

void HeapBase::dumpLiveObjects(std::ostream &os) {
	constexpr std::size_t numDataBytes = 4;
	static const std::string indent(4, ' ');
	
	for(auto root : mRoots) {
		this->mark(root);
	}
	os << std::hex;
	for(auto blk = mHeapStart; blk < mHeapEnd; blk = blk->following()) {
		if(blk->mark()) {
			blk->ptr().mark(false);
			os << static_cast<void*>(blk->data()) << ' ' << "TODO NAME" << '\n';
			os << "  Data: ";
			std::copy_n(blk->data(), std::min(blk->type().size(), numDataBytes), std::ostream_iterator<int>(os, " "));
			if(blk->type().size() > numDataBytes) {
				os << "...";
			}
			os << "\n  Pointers: ";
			if(blk->type().offsets() > 0) {
				os << "\n";
				for(auto offset : blk->type()) {
					os << indent << *reinterpret_cast<void**>(blk->data() + offset) << '\n';
				}
			} else {
				os << "none\n";
			}
		} // if(blk->mark())
	}
	os << std::dec;
}

HeapBase::HeapStats HeapBase::collectHeapStats(bool countLiveObjects) noexcept {
	HeapStats result{};
	result.heapSize = reinterpret_cast<byte*>(mHeapEnd) - reinterpret_cast<byte*>(mHeapStart);
	
	if(countLiveObjects) {
		for(auto root : mRoots) {
			this->mark(root);
		}
	}
	for(auto blk = mHeapStart; blk < mHeapEnd; blk = blk->following()) {
		if(blk->free()) {
			result.numFreeBlocks++;
			result.freeBlockSize += blk->size();
			result.freeSize += Align + align(blk->size());
		} else {
			if(blk->mark()) {
				blk->ptr().mark(false);
				result.numLiveObjects++;
				result.liveObjectSize += blk->type().size();
			}
			result.numObjects++;
			result.objectSize += blk->type().size();
			result.usedSize += Align + align(blk->size());
		}
	}
	assert(result.freeSize + result.usedSize == result.heapSize);
	
	return result;
}

} // namespace ssw
