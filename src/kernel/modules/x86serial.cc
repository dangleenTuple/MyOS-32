#include "../core/os.h"
#include "x86serial.h"

#include "../core/api/dev/ioctl.h"
#include "../core/api/dev/tty.h"

/* Serial port driver for an x86-based operating system

   The serial port device driver acts as an abstraction layer. It hides the specifics
   of the underlying hardware (e.g., the specific UART chip on the motherboard) from
   the operating system and applications.

   This device driver appears to be abstracted from an emulated UART chip.
*/

u8 X86Serial::init_serial=0;

File* x86serial_mknod(char* name,u32 flag,File* dev){
	X86Serial* cons=new X86Serial(name);
	return cons;
}

module("module.x86serial",MODULE_DEVICE,X86Serial,x86serial_mknod)

X86Serial::~X86Serial(){

}

X86Serial::X86Serial(char* n) : Device(n)
{

}


void X86Serial::putc(char c){
        //Wait for status ready by using 0x20 to check the Transmit Holding Register Empty (THRE) bit
	while(io.inb(COM1 + 5) & 0x20 == 0 );
	//When we break out of the spinlock, we send the character
	io.outb(COM1,c);
}

char X86Serial::getc(){
	//Wait for status received by using 0x1 to check the Received Data Ready (RXRDY) bit.
	while(io.inb(COM1 + 5) & 0x1 == 0 );
	return (char)io.inb(COM1);
}

//All of the below values seem to be initializing a UART chip.
u32	X86Serial::open(u32 flag){
	if (init_serial==0){
		//set the Line Control Register (LCR) to its default state
		io.outb( COM1 + 1,	0x00 );
		//set the Data Control Register (DCR) to enable DTR (Data Terminal Ready) signal for the serial port.
		io.outb( COM1 + 3,	0x80 );
		//set the Transmitter Holding Register (THR) to send a value of 3 (likely for setting baud rate or other configuration).
		io.outb( COM1,		0x03 );
		//setting the LCR again, potentially for further configuration.
		io.outb( COM1 + 1,	0x00 );
		//setting the DCR again, possibly for additional control signal configuration.
		io.outb( COM1 + 3, 0x03 );
		//set the Interrupt Enable Register (IER) to enable specific UART interrupts.
		io.outb( COM1 + 2, 0xC7 );
		//set the Divisor Latch (DLL) and Divisor Latch High (DLH) registers to configure the baud rate (details depend on the specific UART clock frequency).
		io.outb( COM1 + 4, 0x0B );
		//Indicates the serial port has been initialized
		init_serial=1;
	}
	return RETURN_OK;
}

u32	X86Serial::close(){
	return RETURN_OK;
}

void X86Serial::scan(){

}

u32	X86Serial::read(u32 pos,u8* buffer,u32 sizee){
	int i;
	for (i=0;i<sizee;i++){
		*buffer=getc();
		buffer++;
	}
	return sizee;
}

u32	X86Serial::write(u32 pos,u8* buffer,u32 sizee){	
	int i;
	for (i=0;i<sizee;i++){
		putc(*buffer);
		buffer++;
	}
	return sizee;
}

u32	X86Serial::ioctl(u32 id,u8* buffer){
	u32 ret=0;
	switch (id){
		case DEV_GET_TYPE:
			ret=DEV_TYPE_TTY;
			break;

		case DEV_GET_STATE:
			ret=DEV_STATE_OK;
			break;

		case DEV_GET_FORMAT:
			ret=DEV_FORMAT_CHAR;
			break;

		default:
			ret=NOT_DEFINED;
			break;
	}
	return ret;
}

u32	X86Serial::remove(){
	delete this;
	return RETURN_OK;
}
