#include <os.h>


// The below extern block ensures these functions are accessible by C code, even though they might be written in C++.
extern "C"
{
	int __cxa_atexit(void (*Destructor) (void *), void *Parameter, void *HomeDSO);
	void __cxa_finalize(void *);
	void __cxa_pure_virtual();
	void __stack_chk_guard_setup();
	void __attribute__((noreturn)) __stack_chk_fail();
	void _Unwind_Resume();
}

void *__dso_handle;
void *__stack_chk_guard(0);

//Exception handling
namespace __cxxabiv1
{
	__extension__ typedef int __guard __attribute__((mode(__DI__)));

	extern "C"
	{
		int __cxa_guard_acquire(__guard *Guard)		{ return ! *(char *) (Guard); }
		void __cxa_guard_release(__guard *Guard)	{ *(char *) Guard = 1; }
		void __cxa_guard_abort(__guard *)			{ }
	}
}


int __cxa_atexit(void (*) (void *), void *, void *)
{
	return 0;
}

void _Unwind_Resume(){
0}

void __cxa_finalize(void *){
}

void __cxa_pure_virtual(){
}

//Stack overflow detection
void __stack_chk_guard_setup()
{
	unsigned char *Guard;
	Guard = (unsigned char *) &__stack_chk_guard;

	//Set the last value to 255. If this gets corrupted, then we have a stack overflow.
	Guard[sizeof(__stack_chk_guard) - 1] = 255;
	//Set this to distinguish the last byte from other information
	Guard[sizeof(__stack_chk_guard) - 2] = '\n';
	//Idk why this is set
	Guard[0] = 0;
}


struct IntRegs;

//Prints error then goes into infinite loop
void __attribute__((noreturn)) __stack_chk_fail()
{
	io.print("Buffer Overflow (SSP Signal)\n");
	for(;;) ;
}

void operator delete(void *ptr) 
{
		//Frees physical contiguous memory in kernel
		kfree(ptr);
}

#ifndef __arm__
void* operator new(size_t len) 
{
	//Allocates physical continguous (mem pool) memory in kernel
	return (void*)kmalloc(len);
}

void operator delete[](void *ptr) 
{
	::operator delete(ptr);
}

void* operator new[](size_t len) 
{
	return ::operator new(len);
}

#else

void* operator new(size_t len) 
{
	return (void*)kmalloc(len);
}

void operator delete[](void *ptr) 
{
	::operator delete(ptr);
}

void* operator new[](size_t len) 
{
	return ::operator new(len);
}
#endif


void *__gxx_personality_v0=(void*)0xDEADBEAF;

//NOTE: The operators new and delete cannot be used before virtual memory and pagination have been initialized.
