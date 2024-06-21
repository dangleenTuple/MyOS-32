
#include "../core/os.h"
#include "ide.h"

#include "../core/api/dev/ioctl.h"

/*
  Integrated Drive Electronics (IDE) is an older interface standard used for
  connecting storage devices like hard disk drives (HDDs) and optical disk drives
  (ODDs) to a computer's motherboard

  PATA (Parallel Advanced Technology Attachment) hard drives are used as the primary storage device
  responsible for organizing all the physical device storage for a computer.

 BL = Block Layer
 In device driver code "block layer" refers to the software layer that interacts with block devices like hard drives and SSDs.
*/


/*
 *	This function waits for the disk to be ready for an operation.
 */
int bl_wait(unsigned short base)
{
	while(io.inb(base+0x206) & 0x80);
	return 0;	
}

/*
 *	This function allows you to move data between the computer's memory and a specific location on a PATA hard drive
 */
int bl_common(int drive, int numblock, int count)
{
	//0x1F0 is likely used for data transfer (reading/writing) and status register access.
	bl_wait(0x1F0);
	
	io.outb(0x1F1, 0x00);	/* NULL byte to port 0x1F1 */
	io.outb(0x1F2, count);	/* Sector count */
	
	//Ports 0x1F3-0x1F5 send the complete block address information
        io.outb(0x1F3, (unsigned char) numblock);	/* Low 8 bits of the block address */
	io.outb(0x1F4, (unsigned char) (numblock >> 8));	/* Next 8 bits of the block address */
	io.outb(0x1F5, (unsigned char) (numblock >> 16));	/* Next 8 bits of the block address */

	// Drive indicator, magic bits, and highest 4 bits of the block address
	// Highest 4 bits are combined with the drive number and sent to port 0x1F6 which accepts control information
	// such as device number, operation code (read or write),  etc.
	io.outb(0x1F6, 0xE0 | (drive << 4) | ((numblock >> 24) & 0x0F));

	// Port 0x1F6 is used to send control commands and receive status information
	// 0xE0: This is a base value for a read/write operation
	// The PATA device we are using would have two channels - device 0 or device 1. So "drive" should be either 0 or 1

	return 0;
}

/*
 *	This function reads a buffer from the disk.
 */
int bl_read(int drive, int numblock, int count, char *buf)
{
	u16 tmpword;
	int idx;

	//We need to call this function so we can set up communicating with the IDE controller
	bl_common(drive, numblock, count);

	//0x20 read operation
	io.outb(0x1F7, 0x20);

	//We need to wait for the IDE controller to be ready to receive commands
	bl_wait(0x1F0);

	/* Wait for the drive to signal that it's ready for a READ operation: */
	while (!(io.inb(0x1F7) & 0x08));

	for (idx = 0; idx < 256 * count; idx++) {
		tmpword = io.inw(0x1F0);
		buf[idx * 2] = (unsigned char) tmpword;
		buf[idx * 2 + 1] = (unsigned char) (tmpword >> 8);
	}
	return count;
}

/*
 *	This function writes to  a buffer on the disk.
 */
int bl_write(int drive, int numblock, int count, char *buf)
{
	u16 tmpword;
	int idx;

	//We need to call this function so we can set up communicating with the IDE controller
	bl_common(drive, numblock, count);

	//0x30 write operation
	io.outb(0x1F7, 0x30);

	//We need to wait for the IDE controller to be ready to receive commands
	bl_wait(0x1F0);

	/* Wait for the drive to signal that it's ready for a WRITE operation: */
	while (!(io.inb(0x1F7) & 0x08));

	for (idx = 0; idx < 256 * count; idx++) {
		tmpword = (buf[idx * 2 + 1] << 8) | buf[idx * 2];
		io.outw(0x1F0, tmpword);
	}

	return count;
}



/* "Make node" = mknod. Make a special file (a new IDE) for the operating system.
   Nodes represent entities in the operating system that provide access to resources or services.
   Keep in mind nodes represent anything that helps access resources on an operating system.
   IDEs help access physical storage, but nodes are a general term that could also
   be in reference to a way to access a virtual resource.
*/
File* ide_mknod(char* name,u32 flag,File* dev){
	Ide* disk=new Ide(name);
	disk->setId(flag);
	return disk;
}

module("module.ide",MODULE_DEVICE,Ide,ide_mknod)

Ide::~Ide(){
	
}

Ide::Ide(char* n) : Device(n)
{
	
}

u32	Ide::close(){
	return RETURN_OK;
}

void Ide::scan(){
	
}

u32	Ide::open(u32 flag){
	return RETURN_OK;
}


/* A more user-friendly function to read from the IDE
   Calculates blocks based on the given position and buffer
   size then passes this in to bl_read.
*/
u32	Ide::read(u32 pos,u8* buffer,u32 sizee){
	int count=(int)sizee;
	
	if (buffer==NULL)
		return -1;
	
	int offset=(int)pos;
	int bl_begin, bl_end, blocks;

	bl_begin = (offset/512);
	bl_end = ((offset + count)/512);
	blocks = bl_end - bl_begin + 1;
	//io.print("%s> read at %d - %d  size=%d begin=%d blocks=%d\n",getName(),offset,offset/512,count,bl_begin,blocks);
	char*bl_buffer = (char *) kmalloc(blocks * 512);
	bl_read(id, bl_begin, blocks,bl_buffer);
	memcpy((char*)buffer, (char *) ((int)bl_buffer + ((int)offset % (int)(512))), count);
	kfree(bl_buffer);
	return count;
}

u32	Ide::write(u32 pos,u8* buffer,u32 sizee){
	return NOT_DEFINED;
}

u32	Ide::ioctl(u32 idd,u8* buffer){
	u32 ret=0;
	switch (idd){
		case DEV_GET_TYPE:
			ret=DEV_TYPE_DISK;
			break;
			
		case DEV_GET_STATE:
			ret=DEV_STATE_OK;
			break;
			
		case DEV_GET_FORMAT:
			ret=DEV_FORMAT_BLOCK;
			break;
			
		default:
			ret=NOT_DEFINED;
			break;
	}	
	return ret;
}



u32	Ide::remove(){
	delete this;
	return RETURN_OK;
}

void Ide::setId(u32 flag){
	id=flag;
}
