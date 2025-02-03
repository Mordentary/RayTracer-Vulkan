#include <new>
#include <cstdlib>
#include <cstdio>
#include"memory.hpp"
void* operator new(std::size_t size) {
	void* ptr = SE::SE_ALLOC(size);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

void* operator new(std::size_t size, std::size_t alignment) {
	void* ptr = SE::SE_ALLOC(size, alignment);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

void* operator new[](std::size_t size) {
	void* ptr = SE::SE_ALLOC(size);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

void* operator new[](std::size_t size, std::size_t alignment) {
	void* ptr = SE::SE_ALLOC(size, alignment);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

void operator delete(void* ptr) noexcept {
	SE::SE_FREE(ptr);
}

void operator delete[](void* ptr) noexcept {
	SE::SE_FREE(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
	SE::SE_FREE(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
	SE::SE_FREE(ptr);
}