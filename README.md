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

## ARCH/X86
Many of the functionalities listed in arch/x86 are tightly coupled with the x86 architecture. These components interact directly with x86-specific hardware features like memory management registers, interrupt controllers, and IO devices. So, this section is responsible for implementing kernel memory functions such as kmalloc, kfree etc., process management regarding setting up CPU registers and memory segments for new processes (and destroying them), an IO controller for a VGA display, ISRs, and the implementation of virtual memory management (please read the walk-through of VM) and the global descriptor table (GDT).

The arch/x86 section may also contain architecture-specific optimizations for the kernel code in the future. For example, CPU Feature Detection and Utilization, Atomic operations, timer ISRs, system call handling (which could also be in core, but here we could implement system call gates and handling the transition between user-mode and kernel-mode when a system call occurs), etc. For now, we will keep it simple and add these features as needed!

NOTE: The importance of using assembly in this portion of the OS is to have direct access to hardware. For instance, the CR3 register which we need to use often is a Control Register directly interacting with the Memory Management Unit (MMU) of the CPU. Assembly allows low-level manipulation of such hardware registers, providing fine-grained control over virtual memory management.

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


## AN ENTIRE WALK-THROUGH OF LINUX FILE SYSTEMS

I've always heard that "everything is a file" in linux, but is it actually?

Memory blocks vs Files:

- Files: Designed for persistence. They reside on the disk (HDD or SSD) and retain data even after a computer shutdown or power outage. This makes them ideal for storing program code, /documents, images, videos, programs, and any information that needs to be saved for long-term access.

- Memory Blocks: Volatile storage. Data in memory blocks (RAM) is lost when the computer is turned off or if there's a power interruption. This impermanence makes them unsuitable for storing information that needs to survive beyond the current session. Memory blocks are typically used for temporary data for processes.

- Some data might have a hybrid existence. For example, a large document might be partially loaded into memory for editing while the entire file remains on disk.

Every file has an inode that serves as an index/unique identifier. Inodes also store essential information about the file, but not the actual data. This metadata includes:
```
    - File size
    - File owner and group
    - File permissions (read, write, execute)
    - Time information (creation, modification, access)
    - Block pointers: Locations of data blocks on the disk that hold the actual file content.
```

Inodes are scattered across the disk/storage device. When the kernel is mounting our file system, every file is all over the place initially. During mounting, the kernel doesn't physically move inodes around. Instead, it builds a logical directory entry tree in memory (RAM). However, a key distinction here is that the directory entry tree does not contain inodes themselves. It contains its own struct that has a file name and a pointer to the actual inode (which contains the real data blocks on disk). In the end, we have a tree that represents our file system when the kernel finishes mounting. Another key distinction here is that the entries do not copy anything. They are simply pointers to the real information (inode) on disk, so the file system tree is incredibly lightweight. 

Sometimes, these pointers can point to the same inode. A hard link is created when a new directory entry is made that points to an existing inode. This essentially creates another way to access the same file content.

Hard links are "copies" of a file but they are not true/deep copies - they are just two or more pointers pointing to the same thing, so only one file exists in memory.

This can be incredibly useful for many programs needing the same dependencies for libraries - why have a bunch of copies when you can just use pointers here?

Another real world example is Git - Git uses version control by indirectly using hard links to previous versions of a file.

Similar to how virtual memory uses "lazy allocation", file systems use a method called "demand paging" which optimizes loading files by only mapping the block pointer to real data blocks on disk when the user or process "demands" it.

If I were simply cruising through a filepath on an OS, the process would look like this:

```
    1.) The operating system reads the file system information, including directory structures and inodes, from the disk (HDD or SSD).
    2.) Based on the provided filepath, the OS locates the relevant inode, which holds metadata about the file (size, permissions, etc.) and pointers to the data blocks (using direct and indirect pointers) on disk where the actual file content resides.
    3.) The OS uses the information from the inodes to display the file structure (directory hierarchy) and file names on your screen. This allows you to navigate your file system and see what files exist.
```
We would simply be looking at the directory entry tree. But what if I actually open up one of these files or start a process that may need one or multiple files? What happens then when using demand paging?

```
    1.) File Access: When you open a file, the operating system first consults the associated inode.
    2.) Inode Information: The inode provides details about the file, including the locations of its data blocks on the disk.
    3.) Initial Load (Optional): Depending on the file system configuration and program behavior, the operating system might load a small portion of the file (e.g., the beginning) into memory for initial display or processing. This is not strictly demand paging but can be an optimization for certain scenarios.
    4.) Demand Paging: The operating system employs demand paging. It might not load the entire file into memory at once.
    5.) Block Translation: When the program needs to access a specific part of the file, the inode's block pointers are used to locate the data block on the disk.
    6.) MMU and Memory Access: The MMU translates the disk address into a physical memory address, and the required data block is loaded into RAM if not already present.
    7.) Program Access: Once the data block is loaded into memory, the program can access and process the requested file content. This cycle of demand paging (locating, translating, loading) repeats as you interact with different parts of the file.
```


Finally - how would our filesystem be available to access in the first place? Let's revisit the very first thing that happens when we turn on our computer - the bootloader.

```
1.) Bootloader

Performs Power-On Self Test (POST) to check hardware functionality.
Loads the kernel image from a designated location (e.g., boot partition).
Passes any necessary parameters to the kernel and initiates the boot process.

2.) Kernel Initialization

The kernel takes over and initializes essential system components like memory management and device drivers. We can access these components before mounting the file system due to compression/decompression algorithms within the kernel image.

3.) Root File System Mounting

    - You specify the storage device (like a disk partition) and the file system type (ext2 (which we are using), XFS, etc.) to be mounted.
    - You designate a directory within your existing file system structure to act as the "access point" for the mounted device. This directory becomes the virtual entry point for all the files and folders within the mounted file system.
    - Integrates the mounted file system's directory structure into your overall file system hierarchy, making the files and folders accessible through the chosen mount point.

If the mounting process is successful, the kernel can access the files and programs necessary to continue booting up.

4.) Init Process and System Startup

Once the root file system is mounted, the kernel launches the initial process (often named init) which is responsible for starting essential system services and user environments like a graphical desktop.

```
