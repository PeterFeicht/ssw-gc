/**
 * @file    HeapObject.hpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   Defines the {@link HeapObject} class.
 */

#ifndef HEAPOBJECT_HPP_
#define HEAPOBJECT_HPP_
#pragma once

#include <cassert>
#include <new>

namespace ssw {

template <typename T, typename Heap>
struct HeapObject
{
	static void* operator new(size_t size, bool isRoot = false) {
		// `new` might allocate more bytes for alignment, it should never allocate less than the type
		// descriptor says (which would mean the type descriptor is wrong)
		assert(size >= T::type.size());
		
		void *mem = Heap::allocate(T::type, isRoot);
		if(!mem) {
			throw std::bad_alloc();
		}
		return mem;
	}
	
	static void operator delete(void* obj) {
		Heap::deallocate(obj);
	}
}; // class HeapObject

} // namespace ssw

#endif /* HEAPOBJECT_HPP_ */
