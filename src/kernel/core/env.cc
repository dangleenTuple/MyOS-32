#include "os.h"

/*
* Defines the structure of an environment variable
* Each variable is a file stored in the virtual directory /sys/env
*/

/*
Environment variables are a way to store and access key-value pairs of data within a program's execution environment.
They are typically set outside the program itself, often during system startup or through configuration files.
Programs can then access and use the values of these environment variables through specific functions provided by the operating system.


When a process searches for an environment variable by its key (e.g., "PATH"), the operating system searches its environment variable
store and retrieves the corresponding value (e.g., "/usr/local/bin:/bin").
*/

/*
* Destructor of the variable
*/
Variable::~Variable(){
	if (value!=NULL)
		kfree(value);
}

/* 
 *	Constructor :
 *		n : name
 *		v : value
 */
Variable::Variable(char* n,char* v) : File(n,TYPE_FILE)
{
	fsm.addFile("/sys/env/",this);
	if (v!=NULL){
		io.print("env: create %s (%s) \n",n,v);
		value=(char*)kmalloc(strlen(v)+1);
		memcpy(value,v,strlen(v)+1);
		setSize(strlen(v)+1);
	}
	else{
		value=NULL;
	}
}

u32	Variable::open(u32 flag){
	return RETURN_OK;
}

u32	Variable::close(){
	return RETURN_OK;
}

/* reading the value into buffer */
u32	Variable::read(u32 pos,u8* buffer,u32 size){
	if (value==NULL)
		return NOT_DEFINED;
	else{
		strncpy((char*)buffer,value,size);
		return size;		
	}
}

/* writing the buffer into the variable */
u32	Variable::write(u32 pos,u8* buffer,u32 size){
	if (value!=NULL)
		kfree(value);
	value=(char*)kmalloc(size+1);
	memset((char*)value,0,size+1);
	memcpy(value,(char*)buffer,size+1);
	value[size]=0;	//to make sure it's a string
	setSize(size+1);
	return size;
}

/* variable control (TODO) */
u32	Variable::ioctl(u32 id,u8* buffer){
	return NOT_DEFINED;
}

/* remove the variable */
u32	Variable::remove(){
	delete this;
	return NOT_DEFINED;
}

/* Only for folders */
void Variable::scan(){

}

