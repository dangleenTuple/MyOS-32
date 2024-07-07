#include "../core/os.h"

template <typename T>
size_t get_size(T* ptr) {
  return sizeof(*((char *)ptr));
}


extern "C" {

/*
 * The memcpy function is used to copy a specified number of bytes from one memory location to another
 * linear (contiguous?) address
 */
void *memcpy(void *dst, void *src, size_t n) {
	void *p = dst;
	while (n--) {
		*(char *)dst++ = *(char *)src++;
	}
	return p;
}
/*
 * Set a memory block (pointed to by dst) to the value of size n
 */
void *memset(void *dst, void *src, size_t n) {
	void *p = dst;
	size_t size_of_src = get_size(src); // Get the size of the data type pointed to by src
	while (n >= size_of_src) {
		memcpy(dst, src, size_of_src);
		dst += size_of_src;
		n -= size_of_src;
	}
	// Handle remaining bytes (if any) using a loop similar to the original memset
	return p;
}

}
