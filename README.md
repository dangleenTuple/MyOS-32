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

I added the below code snippet to the vagrant file to install the prerequisite software:

config.vm.provision "shell", inline: <<-SHELL
     apt-get update
     apt-get install nasm make build-essential grub qemu zip git -y
   SHELL

Keep in mind, if you have any issues with your vagrant file, the syntax of vagrant is Ruby.

Run vagrant up

If a failure occurred, you can get more details by running vagrant --help to see what is suited for the situation.

I had to use vagrant provision --debug since it was failing on the provisioning step (when I realized that Ubuntu no longer supports a 32-bit system...)

## PROJECT STRUCTURE

Our first goal would be to get the kernel to not only compile, but to have all the correct components to produce the correct kernel.elf file that will pass against the
mbchk kernel.elf check. This will validate your kernel.elf file against the multiboot standard.

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
