#include "os.h"

/*
This class organizes the files within the system.
*/

Filesystem::Filesystem() {

}

void Filesystem::init() {
  root = new File("/", TYPE_DIRECTORY);

  dev = root->createChild("dev", TYPE_DIRECTORY);       // directory containing peripherals
  root->createChild("proc", TYPE_DIRECTORY);           // directory containing running processes
  root->createChild("mnt", TYPE_DIRECTORY);           // directory containing disk mount points
  File* sysd = root->createChild("sys", TYPE_DIRECTORY); // directory containing system information
    var = sysd->createChild("env", TYPE_DIRECTORY);     // directory containing environment variables
    sysd->createChild("usr", TYPE_DIRECTORY);           // directory containing all users
    sysd->createChild("mods", TYPE_DIRECTORY);           // directory containing available modules
    sysd->createChild("sockets", TYPE_DIRECTORY);       // directory containing all current sockets
}

Filesystem::~Filesystem(){
	delete root;
}

void Filesystem::mknod(char* module,char* name,u32 flag){
	modm.createDevice(name,module,flag);
}

File* Filesystem::getRoot(){
	return root;
}

File* Filesystem::path(char* p){
	if (!p)
		return NULL;
		
	File* fp=root;
	char *name, *beg_p, *end_p;
	
	if (p[0]=='/')
		fp=root;
	else{
		if (arch.pcurrent!=NULL)		/* Get the current directory from the current file object */
			fp=(arch.pcurrent)->getCurrentDir();
	}
	beg_p = p;
	while (*beg_p == '/')
		beg_p++;
	end_p = beg_p + 1;
	
	while (*beg_p != 0) {
		if (fp->getType() != TYPE_DIRECTORY){
			return NULL;
		}
		while (*end_p != 0 && *end_p != '/')
			end_p++;
		name = (char *) kmalloc(end_p - beg_p + 1);
		memcpy(name, beg_p, end_p - beg_p);
		name[end_p - beg_p] = 0;

		if (strcmp("..", name) == 0) {		// '..' 
			fp = fp->getParent();
		} else if (strcmp(".", name) == 0) {	// '.' 
		
		} else {
			fp->scan();
			if (!(fp = fp->find(name))) {
				kfree(name);
				return 0;
			}
			
			if (fp->getType()==TYPE_LINK && (fp->getLink()!=NULL)){
				fp=fp->getLink();
			}
		}

		beg_p = end_p;
		while (*beg_p == '/')
			beg_p++;
		end_p = beg_p + 1;

		kfree(name);
	}
	
	return fp;
}

File* Filesystem::pivot_root(File* targetdir){
	if (targetdir == NULL)
		return root;
	else
	{
	  File* newRoot = new File("/",TYPE_DIRECTORY);
	  File* mainChild = targetdir->getChild();
	  newRoot->addChild(mainChild);
	  s8 i, ii = 0;
	  File* tempChild = mainChild->getPrec(); //Should the File object be initialized? Or not?
	      do
	      {
		  if(tempChild == NULL)
		  {
		    i = 0;
		  }
		  else
		  {
		    newRoot->addChild(tempChild);
		    tempChild = tempChild->getPrec();
		  }
	      }while(i == 1);
	       tempChild = mainChild->getNext();
	      i = 1;
	      do
	      {
		  if(tempChild == NULL)
		    i = 0;
		  else
		  {
		    newRoot->addChild(tempChild);
		    tempChild = tempChild->getNext();
		  }
	      }while(i == 1);
	return newRoot;
	}
 
}

File* Filesystem::path_parent(char* p,char *fname){
	if (!p)
		return NULL;
	File* ofp;
	File* fp=root;
	char *name, *beg_p, *end_p;
	
	if (p[0]=='/')
		fp=root;
		
	beg_p = p;
	while (*beg_p == '/')
		beg_p++;
	end_p = beg_p + 1;
	
	while (*beg_p != 0) {
		if (fp->getType() != TYPE_DIRECTORY)
			return NULL;

		while (*end_p != 0 && *end_p != '/')
			end_p++;
		name = (char *) kmalloc(end_p - beg_p + 1);
		memcpy(name, beg_p, end_p - beg_p);
		name[end_p - beg_p] = 0;


		if (strcmp("..", name) == 0) {		// '..' 
			fp = fp->getParent();
		} else if (strcmp(".", name) == 0) {	// '.' 
		
		} else {
			ofp=fp;
			
			if (fp->getType()==TYPE_LINK && (fp->getLink()!=NULL)){
				fp=fp->getLink();
			}
			
			if (!(fp = fp->find(name))) {
				strcpy(fname,name);
				kfree(name);
				return ofp;
			}
		
		}

		beg_p = end_p;
		while (*beg_p == '/')
			beg_p++;
		end_p = beg_p + 1;

		kfree(name);
	}
	
	return fp;
}

u32 Filesystem::link(char* fname,char *newf){

	File*tolink=path(fname);
	if (tolink==NULL)
		return -1;

	char* nname=(char*)kmalloc(255);
	File* par=path_parent(newf,nname);
	File* fp=new File(nname,TYPE_LINK);
	fp->setLink(tolink);
	par->addChild(fp);
	return RETURN_OK;
}


u32 Filesystem::addFile(char* dir,File* fp){
	File* fdir=path(dir);
	if (fdir==NULL)
		return ERROR_PARAM;
	else{
		return fdir->addChild(fp);
	}
}



