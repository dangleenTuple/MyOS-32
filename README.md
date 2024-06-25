 # HOW TO MAKE A COMPUTER OPERATING SYSTEM IN C++ (functioning tutorial)

I started building a 32-bit OS following the tutorial [How To Make a Computer Operating System](https://github.com/SamyPesse/How-to-Make-a-Computer-Operating-System/tree/master).
However, a lot of the steps were incomplete or unexplained. For the latest version, it did not compile probably due to some components being outdated.

So I will write a breakdown of a functioning 32-bit OS using the tutorial as a template (because I am learning with you!).

To be clear, I won't be following the book that's referenced perfectly (mainly because it hasn't been finished...). My goal is to make this functioning enough to learn about how 
everything comes together in a way that I can use the OS. I will probably skip making the Lua Interpreter or come back to it in the future.

## VAGRANT

For starters, the 32-bit Ubuntu system that the tutorial uses for its virtual machine is outdated. Ubuntu no longer has a stable 32-bit version, so I switched to Debian.
Debian consistently maintained a stable version of a 32-bit OS, in addition to this it is similar enough to Ubuntu it won't make a difference for this project.

The virtual box (follow the installation instructions in the tutorial, this part is fine) I will be using is "generic-x32/debian9".

Run `vagrant init generic-x32/debian9`

I added the below code snippet to the generated vagrant file to install the prerequisite software:

```
config.vm.provision "shell", inline: <<-SHELL
     apt-get update
     apt-get install nasm make build-essential grub qemu zip git -y
   SHELL
```

Keep in mind, if you have any issues with your vagrant file, the syntax of vagrant is Ruby.

Run `vagrant up`

If a failure occurred, you can get more details by running `vagrant --help` to see what is suited for the situation.

I had to use `vagrant provision --debug` since it was failing on the provisioning step (when I realized that Ubuntu no longer supports a 32-bit system...)

//TODO: Create a separate branch to demonstrate how the vagrant setup should look like - at least, how it looks like for my VM.

## PROJECT STRUCTURE

Our first goal would be to get the kernel to not only compile, but to have all the correct components to produce the correct ``kernel.elf`` file that will pass against the
`mbchk kernel.elf` check. This will validate your kernel.elf file against the multiboot standard.

```
\src
    \kernel
        \arch\x86 --- architecture and memory management
        \core --- main kernel code, filesystems, system calls, APIs
        \modules --- device drivers (and their controllers)
        \runtime --- C++ (and sometimes C) code
        config.h --- all of the info about the kernel (which cpu processor we're using, etc.)
        Makefile --- the component that brings everything together and makes it compile!
    \sdk
        \bootdisk
        \include
        \lib
        \src\libc
        dishimage.sh
        qemu.sh
```
This is everything we need to get a basic 32-bit OS to function. From here, we will look into each component and see how it plays a role. When I am finished, there will be
comments explaining every single file that's in the code and hopefully put the missing pieces together from what's not written in the book.

## RUNTIME

We are throwing some standard functions of C++ in this directory to be able to use throughout the code (strlen, strcpy, strcmp, itoa, memcpy, memset). We also are using a couple of data structures
that are necessary such as a Buffer (replacement for std::vector, so a dynamic array) and a linked list.

## MODULES

This is where we are putting all of our device drivers and the controllers for them.

The way this works is we are using a global "module_builder" array. At runtime, the module controller loads the necessary drivers, likely based on configuration or user selection. The controller then interacts with these drivers to perform various tasks related to the specific hardware they manage.

A good thing to consider about device drivers is that they act as an abstraction layer for software and hardware interaction within the operating system. They simplify how programs access physical components.

For example, to get the current date for our system, we need information from the CMOS chip (a battery-powered component that keeps time even when the main power is off). However, the device driver hides the complexity of this chip. We don't need to write code that directly interacts with the chip's electrical circuits, which use internal electronics to keep track of time (using an electrical toggle).

Instead, the driver allows us to write a simple file that interacts with the chip's registers (internal storage locations) to retrieve the data. The driver might then perform a conversion from Binary-Coded Decimal (BCD) format to a more standard format before providing the date to the program through a function like "GetDate." This function hides all the electrical details of how the clock works, making it easier for programmers to use.

## AN ENTIRE WALK-THROUGH OF VIRTUAL MEMORY

Let's start with a process. From the CPU's perspective, we don't know how much memory we may or may not use yet. A virtual address space is assigned which can be accessed from a page table that is dedicated to the process.

What is a page table? It is a data structure that contains contiguous entries. These entries can be thought of as a struct containing a pointer that is initially set to null but is dedicated to be set to a physical memory frame (in RAM), and some status bits like a valid bit(indicating whether or not the memory is used or unused) or permission bits. Each Page Table Entry is mapped to a virtual memory page (which is a specific size decided by the OS).

How can the virtual memory page map to the entry? The Memory Management Unit (MMU).

Think of the MMU as a librarian helping you find the actual book you want from a catalog. The catalog has a virtual page number (VPN) that corresponds to a Page Table Entry. Think of the VPN as the key in a map structure and the value as the PTE. So the lookup is very fast!

The MMU is also responsible for mapping the PTEs to physical memory *if necessary*. As I mentioned before, the pointer per PTE that represents a real address is initially set to null. This is called lazy allocation - a key component to this "magic" called virtual memory. When we try to access or modify actual memory for a process, a page fault (when a program tries to access data or code that is located in its virtual address space, but that data is not currently loaded in physical memory (RAM)) occurs on the first account and triggers the MMU to map the entry to actual physical memory.

```
int* num = new int[5]; //Nothing happens yet except an array of size five is allocated in the virtual memory address space.

num[0] = 7; //This triggers a page fault because we are modifying/accessing physical memory for the first time
// (through indirect mapping) and it hasn't been loaded into our page table yet as a real address. The OS will find a free,
// unused memory frame of the appropriate size within the assigned page table for the process. If it can't, it might use a page replacement algorithm.

int x = 5; //This is somehow allocated in virtual memory, this doesn't require any actual access to physical memory. So no page fault is triggered.

x++; //The virtual memory is altered to be 6. Nothing else happens.

num[1] = 5; //No page fault because this isn't the first time we are accessing this array. However, through indirect mapping
// handled by the page table, we will send a write instruction to the mapped physical address to change the value at index 1 to 5

```

Each virtual memory page can contain a number of variables or data structures contiguously since it is essentially a fixed size block dedicated to the process. The number of memory pages dedicated to the process is heavily dependent on how much virtual memory the process might need. If the process ends up exceeding this number - the OS can swap out some inactive pages to be active for the process.

Let's talk more about inactive vs active page tables. This is really important since this is how we can take a finite amount of space and make it "magically" appear as there's more memory available.

Page tables are organized into a hierarchical paging system. At the top, there's a dedicated page directory that is always in RAM. The page directory is responsible for pointing to page tables, which are considered secondary-level in the hierarchical paging system. The PD is a similar structure to a page table itself in that it is a contiguous block of entries, however, it doesn't contain page table entries - it contains pointers to other page tables that can be either active or inactive.

The page directory is an index of all page tables that the OS is using except for the page tables that are swapped out to disk (due to not being used for an extended period of time). Some of these page tables might be completely empty, but it's useful to have these in the PD in case if we need to use them for a process.

So what is it meant exactly by being "swapped"? Some inactive or empty page tables might be send to disk in case if we are running out of room in RAM. They can be later grabbed if we need them, and sometimes the swapping can be determined by an LRU algorithm if we don't have a lot of RAM. However, this isn't ideal since it is a lot of overhead to grab a page table from disk, so it's best to just put page tables there unless if we absolutely have to. This is why we try to keep everything in RAM if we can, even empty page tables.

In conclusion, virtual memory is seemingly memory created by magic, but it's actually all managed within physical memory itself in a safe, efficient manner.
