
;_start and _kmain are declared as global symbols, meaning they can be accessed from other parts of the code.
;kmain, start_ctors, end_ctors, start_dtors, and end_dtors are declared as external symbols. This means they are defined elsewhere (probably in the kernel code) but referenced here.

global _start, _kmain
extern kmain, start_ctors, end_ctors, start_dtors, end_dtors

;MULTIBOOT_HEADER_MAGIC is a specific value that identifies a Multiboot header.
;MULTIBOOT_HEADER_FLAGS defines some flags for the header (value 3 in this case).
;CHECKSUM is calculated by negating the sum of the magic number and flags, likely for basic verification.
%define MULTIBOOT_HEADER_MAGIC  0x1BADB002
%define MULTIBOOT_HEADER_FLAGS	0x00000003
%define CHECKSUM -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)


;-- Entry point
_start:
	jmp start

align 4

;uses dd (double word) assembly instruction to define 4-byte values.
multiboot_header:
dd MULTIBOOT_HEADER_MAGIC
dd MULTIBOOT_HEADER_FLAGS
dd CHECKSUM

;ebx is the general-purpose register we are going to be using
start:
	push ebx

;start_ctors points to the beginning of an array containing constructor functions.
;The loop uses a test instruction to compare ebx (pointing to current constructor) with end_ctors (end of the array).
;If ebx is less than end_ctors, the function pointed to by ebx is called (call [ebx]). This likely calls functions used to initialize parts of the kernel before kmain.
;ebx is then incremented by 4 to move to the next constructor function in the array.
;The loop continues until all constructors are called.
static_ctors_loop:
   mov ebx, start_ctors
   jmp .test
.body:
   call [ebx]
   add ebx,4
.test:
   cmp ebx, end_ctors
   jb .body

   call kmain                      ; call kernel proper


;After constructors, the kernel's main function, kmain, is called. This is where the actual kernel execution begins.
;A similar loop (static_dtors_loop) iterates through destructor functions (pointed to by start_dtors and ending at end_dtors) likely used for cleanup tasks before the system halts.
static_dtors_loop:
   mov ebx, start_dtors
   jmp .test
.body:
   call [ebx]
   add ebx,4
.test:
   cmp ebx, end_dtors
   jb .body

	cli ; stop interrupts
	hlt ; halt the CPU
