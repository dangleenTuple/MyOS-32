#include <os.h>


extern "C" {

/*
 * The memcpy function is used to copy a specified number of bytes from one memory location to another
 * linear (contiguous?) address
 */
void *memcpy(char *dst, char *src, int n)
{
	char *p = dst;
	while (n--)
		*dst++ = *src++;
	return p;
}

/*
 * Set a memory block (pointed to by dst) to the value of size n
 */
void *memset(char *dst,char src, int n)
{
	char *p = dst;
	while (n--)
		*dst++ = src;
	return p;
}

}
