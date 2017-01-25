/**
 * @file    TypeDescriptor.cpp
 * @author  niob
 * @date    Jan 9, 2017
 * @brief   Defines the {@link TypeDescriptor} class.
 */

#include "TypeDescriptor.hpp"

#include <algorithm>
#include <cassert>
#include <type_traits>
#include <utility>

namespace ssw {

// XXX Is that really necessary?
static_assert(std::is_standard_layout<TypeDescriptor>::value, "TypeDescriptor must be standard-layout.");

namespace {

/** The offset of the list of pointer offsets from the beginning of a TypeDescriptor. */
constexpr std::size_t ListOffset =
		(sizeof(TypeDescriptor) + alignof(std::ptrdiff_t) - 1) & ~(alignof(std::ptrdiff_t) - 1);

}

void* TypeDescriptor::operator new(std::size_t size, std::size_t offsets, AllocTag) {
	assert(size <= ListOffset);
	return ::operator new(ListOffset + sizeof(std::ptrdiff_t) * (offsets + 1));
}

TypeDescriptor::TypeDescriptor(std::string name, std::size_t size, Destructor destructor,
		std::initializer_list<std::ptrdiff_t> offsets)
		: mName(std::move(name)), mSize(size), mDestructor(destructor), mOffsets(offsets.size()) {
	std::copy(offsets.begin(), offsets.end(), const_cast<std::ptrdiff_t*>(this->begin()));
	*const_cast<std::ptrdiff_t*>(this->end()) =
			reinterpret_cast<const char*>(this) - reinterpret_cast<const char*>(this->end());
}

const std::ptrdiff_t* TypeDescriptor::begin() const noexcept {
	return reinterpret_cast<const std::ptrdiff_t*>(reinterpret_cast<const char*>(this) + ListOffset);
}

} // namespace ssw
