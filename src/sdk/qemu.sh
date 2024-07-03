#!/bin/bash
qemu-system-i386 -kernel ./bootdisk/kernel.elf -s -hda ./c.img  -curses -serial /dev/tty  -redir tcp:2323::23
