#include "../core/os.h"
#include "null.h"

#include "../core/api/dev/ioctl.h"

/* This essentially is a "dummy" device driver.

   Dummy device drivers are especially useful for debugging without interacting with any actual
   physical hardware. We may need to emulate functionality of a device driver or use this as a placeholder
   for a missing driver.

   They are also useful for data sinks (unused data that gets thrown into the "sink to later be discarded)
   or redirection (when we would want to suppress or redirect standard streams (standard input, output, and error)).
*/

File* null_mknod(char* name,u32 flag,File* dev){
	Null* cons=new Null(name);
	return cons;
}

module("module.null",MODULE_DEVICE,Null,null_mknod)

Null::~Null(){
}

Null::Null(char* n) : Device(n){

}

void Null::scan(){

}

u32	Null::close(){
	return RETURN_OK;
}

u32	Null::open(u32 flag){
	return RETURN_OK;
}

u32	Null::read(u32 pos,u8* buffer,u32 size){
	memset((char*)buffer,0,size);
	return size;
}

u32	Null::write(u32 pos,u8* buffer,u32 size){
	return size;
}

u32	Null::ioctl(u32 id,u8* buffer){
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
	}
	return ret;
}

u32	Null::remove(){
	delete this;
	return RETURN_OK;
}
