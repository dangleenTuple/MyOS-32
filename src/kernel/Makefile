ARCH=x86
KERNEL=kernel.elf
SDKDIR=../sdk
# INCDIR= -I ./ -I ./modules -I ./core -I ./arch/$(ARCH)

# include ./arch/$(ARCH)/config.make

include ./runtime/Makefile
# include ./core/Makefile
# include ./modules/Makefile
# include ./arch/$(ARCH)/Makefile

# FLAG :=$(FLAG) -D__$(ARCH)__
# Linker
LD=ld
LDFLAG= -melf_i386 -static  -L ./  -T ./arch/$(ARCH)/linker.ld

# C++ compiler
SC=g++
FLAG= $(INCDIR) -g -O2 -w -trigraphs -fno-builtin  -fno-exceptions -fno-stack-protector -O0 -m32  -fno-rtti -nostdlib -nodefaultlibs 

# Assembly compiler
ASM=nasm
ASMFLAG=-f elf -o

PLATFORMS= `find ./arch/ -type d | sed "s/.*\///" | sort`


all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAG) -o $@ $^ 
	cp $(KERNEL) $(SDKDIR)/bootdisk/

help:
	@echo "Makefile for Kernel."
	@echo "Please see COPYING for licensing information."
	@echo "Usage: make [ all | clean] " 
	@echo "Currently supported platforms:"
	@echo

tosdk:
	cp $(KERNEL) $(SDKDIR)/disk/

install:
	sudo cp $(KERNEL) /boot/

debug:
	$(NM) -n $(KERNEL)

# dasm:
#	$(OBJDUMP) -d $(KERNEL) > dasm.txt

run:
	cd $(SDKDIR) &&	sh ./diskimage.sh
	cd $(SDKDIR) &&	sh ./qemu.sh

%.o: %.cc
	$(SC) $(FLAG) -c $< -o  $@

%.o: %.S
	$(SC) $(FLAG) -c $< -o  $@

%.o: %.asm
	$(ASM) $(ASMFLAG)  -c $< -o  $@

clean:
	rm -f $(OBJS)  $(KERNEL) dasm.txt
