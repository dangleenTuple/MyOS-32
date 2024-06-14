#!/bin/bash
qemu-img create c.img 2M
fdisk ./c.img  << EOF

x # Switch to Expert commands

c # Change number of cylinders (1-1048576) 
4

h # Change number of heads (1-256, default 16):
16

s # Change number of sectors/track (1-63, default 63)
63

r # Return to main menu

n # Add a new partition

p # Choose primary partition

1 # Choose partition number

1 # Choose first sector (1-4, default 1)

4 # Choose last sector, +cylinders or +size{K,M,G} (1-4, default 4)

a # Toggle bootable flag

1 # Choose first partition for bootable flag

w # Write table to disk and exit
EOF
fdisk -l -u ./c.img

losetup -o 32256 /dev/loop1 ./c.img # Using fdisk -l -u c.img, you get: 63 * 512 = 32256.

mke2fs /dev/loop1 # We create a EXT2 filesystem on this new device using:

mount  /dev/loop1 /mnt/
cp -R bootdisk/* /mnt/
umount /mnt/

grub --device-map=/dev/null << EOF
device (hd0) ./c.img
geometry (hd0) 4 16 63
root (hd0,0)
setup (hd0)
quit
EOF

losetup -d /dev/loop1
