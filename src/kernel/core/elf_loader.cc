#include <os.h>

/*
External module loader in elf32 format:
currently compiled statically without shared libraries */


char* __default_proc_name="_proc_";	/* Default name with a number added */
char 	nb_default='0';

/*
 * Checks if the file whose address is passed as an argument
 * is in ELF format
 */
int is_elf(char *file)
{
	Elf32_Ehdr *hdr;

	hdr = (Elf32_Ehdr *) file;
	if (hdr->e_ident[0] == 0x7f && hdr->e_ident[1] == 'E'
	    && hdr->e_ident[2] == 'L' && hdr->e_ident[3] == 'F')
		return RETURN_OK;
	else
		return ERROR_PARAM;
}

/*
 *	Loads the ELF file into virtual memory and returns the starting address
 */
u32 load_elf(char *file,process_st *proc)
{
	char *p;
	u32 v_begin, v_end;
	Elf32_Ehdr *hdr;
	Elf32_Phdr *p_entry;
	Elf32_Scdr *s_entry;
	int i, pe;

	hdr = (Elf32_Ehdr *) file;
	p_entry = (Elf32_Phdr *) (file + hdr->e_phoff);

	s_entry= (Elf32_Scdr*) (file + hdr->e_shoff);
	
	if (is_elf(file)==ERROR_PARAM) {
		io.print("INFO: load_elf(): file not in ELF format !\n");
		return 0;
	}
	
	for (pe = 0; pe < hdr->e_phnum; pe++, p_entry++) {	/* Read each entry */

		if (p_entry->p_type == PT_LOAD) {
			v_begin = p_entry->p_vaddr;
			v_end = p_entry->p_vaddr + p_entry->p_memsz;
			if (v_begin < USER_OFFSET) {
				io.print ("INFO: load_elf(): can't load executable below %p\n", USER_OFFSET);
				return 0;
			}

			if (v_end > USER_STACK) {
				io.print ("INFO: load_elf(): can't load executable above %p\n", USER_STACK);
				return 0;
			}

			// Description of the executable and read-only data section (exec + rodata)
			if (p_entry->p_flags == PF_X + PF_R) { // TODO: Should this be BITWISE OR?
				proc->b_exec = (char*) v_begin;
				proc->e_exec = (char*) v_end;
			}

			// Description of the bss section
			if (p_entry->p_flags == PF_W + PF_R) { // TODO: Same as above ^ (More specifically, should it be PF_W | PF_R?)
				proc->b_bss = (char*) v_begin;
				proc->e_bss = (char*) v_end;
			}
			//io.print("elf : %x to %x \n",(file + p_entry->p_offset),v_begin);
			memcpy((char *) v_begin, (char *) (file + p_entry->p_offset), p_entry->p_filesz);
			if (p_entry->p_memsz > p_entry->p_filesz)
				for (i = p_entry->p_filesz, p = (char *) p_entry->p_vaddr; i < (int)(p_entry->p_memsz); i++)
					p[i] = 0;
			
			
			
		}
	}
	/* Return program entry point */
	
	return hdr->e_entry;
}

/*
 * Load a file by creating a new process
 * (TODO: Shouldn't execv replace the running program with a new one within the same process? Idk if it has much to with loading a file)
 */
int execv(char* file,int argc,char** argv){
	char* map_elf=NULL;
	File* fp=fsm.path(file);
	if (fp==NULL)
		return ERROR_PARAM;
	
	map_elf=(char*)kmalloc(fp->getSize());
	fp->open(NO_FLAG);
	fp->read(0,(u8*)map_elf,fp->getSize());
	fp->close();
	
	char* name;
	__default_proc_name[strlen(__default_proc_name)-1]=nb_default;
	nb_default++;
	if (argc<=0)
		name=__default_proc_name;
	else
		name=argv[0];
	//io.print("exec %s > %s\n",file,name);

	Process* proc=new Process(name);
	proc->create(map_elf,argc,argv);
	kfree(map_elf);
	return (int)proc->getPid();
}

/*
 *	Load a module
 */
void execv_module(u32 entry,int argc,char** argv){
	char* name;
	__default_proc_name[strlen(__default_proc_name)-1]=nb_default;
	nb_default++;
	if (argc<=0)
		name=__default_proc_name;
	else
		name=argv[0];
	
	Process* proc=new Process(name);
	proc->create((char*)entry,argc,argv);
}


