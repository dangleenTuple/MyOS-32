#!/bin/bash
qemu-img create c.img 2M
fdisk ./c.img  << EOF

# Switch to Expert commands
x

# Change number of cylinders (1-1048576)
c
4

# Change number of heads (1-256, default 16):
h
16

# Change number of sectors/track (1-63, default 63)
s
63

# Return to main menu
r

# Add a new partition
n

# Choose primary partition
p

# Choose partition number
1

# Choose first sector (1-4, default 1)
1

# Choose last sector, +cylinders or +size{K,M,G} (1-4, default 4)
4

# Toggle bootable flag
a

# Choose first partition for bootable flag
1

# Write table to disk and exit
w
EOF
fdisk -l -u ./c.img

# Using fdisk -l -u c.img, you get: 63 * 512 = 32256.
losetup -o 32256 /dev/loop1 ./c.img

# We create a EXT2 filesystem on this new device using:
mke2fs /dev/loop1

# We copy our files on a mounted disk:
mount  /dev/loop1 /mnt/
cp -R bootdisk/* /mnt/
umount /mnt/

# Install GRUB on the disk:
grub --device-map=/dev/null << EOF
device (hd0) ./c.img
geometry (hd0) 4 16 63
root (hd0,0)
setup (hd0)
quit
EOF

# And finally we detach the loop device:
losetup -d /dev/loop1
