//We will be referencing the macro that defines module_builder, an array that points to module_class objects
#include "../core/os.h"


#include "../core/file.h"
#include "../runtime/alloc.h"
#include "../runtime/libc.h"
#include "../core/filesystem.h"
#include "module.h"

/* Inclusion of the module list */
#include "modules.conf"

Module::Module(){

}

Module::~Module(){

}

// Allocate memory for every node in the module_builder list
void Module::initLink(){
    int i=0;
    ModLink* mod;
    while (i < module_builder.size() && module_builder[i] != nullptr){
        mod=new ModLink(module_builder[i]->module_name);
        i++;
    }
}


File* Module::createDevice(char* name,char* module,u32 flag){
	int i=0;
	//File pointer
	File* fp;
	while (module_builder[i] != 0){
		if (!strcmp(module_builder[i]->module_name,module)){
			if (module_builder[i]->module_type==MODULE_DEVICE){
				fp=module_builder[i]->drive(name,flag,NULL);
				return fp;
			}
		}
		i++;
	}
	return NULL;
}

// Let's make these modules accessible! Mount all of the modules (FUNCTION NOT FINISHED)
File* Module::mount(char* dev,char* dir,char* module,u32 flag){
	File* fdev=fsm.path(dev);
	if (fdev==NULL)
		return NULL;
	int i=0;
	File* fp;
	while (module_builder[i] != 0){
		if (!strcmp(module_builder[i]->module_name,module)){
			fp=module_builder[i]->drive(dir,flag,fdev);

			// TODO: We only have the functionality of filesystem modules. We need to finish
			// writing the other modules.
			if (module_builder[i]->module_type==MODULE_FILESYSTEM && fp!=NULL){
				fsm.addFile("/mnt/",fp);
				fp->setType(TYPE_DIRECTORY);
				return fp;
			}
			else
				return NULL;
		}
		i++;
	}
	return NULL;
}

// Now that the modules are accessible, we need to make them functional. Let's install the contents! (FUNCTION NOT FINISHED)
File* Module::install(char* dir,char* module,u32 flag,char* dev){
	File* fdev=fsm.path(dev);
	if (fdev==NULL)
		return NULL;
	int i=0;
	File* fp;
	while (module_builder[i] != 0){

		// TODO: We only have the functionality of filesystem modules. We need to finish
                // writing the other modules.
		if (!strcmp(module_builder[i]->module_name,module)){
			if (module_builder[i]->module_type==MODULE_FILESYSTEM){
				return module_builder[i]->drive(dir,flag,fdev);
			}
		}
		i++;
	}
	return NULL;
}
