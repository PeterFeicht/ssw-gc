/**
 * @file    Heap.hpp
 * @author  niob
 * @date    Oct 21, 2016
 * @brief   TODO Declares the {@link Heap} class.
 */

#ifndef HEAP_HPP_
#define HEAP_HPP_
#pragma once

#include <cstddef>
#include <ostream>

#include "TypeDescriptor.hpp"

namespace ssw {

using byte = unsigned char;

template <std::size_t HeapSize, bool Static = true>
class Heap;

template <std::size_t HeapSize>
class Heap<HeapSize, true>
{
	byte mStorage[HeapSize];
	
public:
	void* allocate(std::size_t size);
	
	void dump(std::ostream &os);
};

} // namespace ssw

#endif /* HEAP_HPP_ */
