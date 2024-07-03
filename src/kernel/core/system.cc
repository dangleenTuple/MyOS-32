#include "os.h"

/*
 *   10/04/06 (Samy Pess) : creation of the file
 *                           added login management and environment variable
 *   10/04/07 (Samy Pess) : the System class now manages a linked list of users (User)
 */
/*
This class organizes the system itself: user, variable, ...
*/

System::System(){
	
}

System::~System(){

}


void System::init(){
	var=fsm.path("/sys/env/");

	/** System user **/
	root=new User("root");
	root->setUType(USER_ROOT);
	
	actual=new User("liveuser");

	
	/** Environnement variable **/
	uservar=new Variable("USER","liveuser");
	new Variable("OS_NAME",KERNEL_NAME);
	new Variable("OS_VERSION",KERNEL_VERSION);
	new Variable("OS_DATE",KERNEL_DATE);
	new Variable("OS_TIME",KERNEL_TIME);
	new Variable("OS_LICENCE",KERNEL_LICENCE);
	new Variable("COMPUTERNAME",KERNEL_COMPUTERNAME);
	new Variable("PROCESSOR_IDENTIFIER",KERNEL_PROCESSOR_IDENTIFIER);
	new Variable("PROCESSOR_NAME",arch.detect());
	new Variable("PATH","/bin/");
	new Variable("SHELL","/bin/sh");
}

// Login function
int	System::login(User* us,char* pass){
	if (us==NULL)
		return ERROR_PARAM;
	if (us->getPassword() != NULL){	//Check if the user has a password set
		
		if (pass==NULL)	//Check if the provided password is NULL
			return PARAM_NULL;
			//
		if (strncmp(pass,us->getPassword(),strlen(us->getPassword())))	//test password
			return RETURN_FAILURE;
		//io.print("login %s with %s (%s)\n",us->getName(),pass,us->getPassword());
	}
		
	uservar->write(0,(u8*)us->getName(),strlen(us->getName()));
	actual=us;
	return RETURN_OK;
}

/* don't forget to free the memory used by the variable */
char* System::getvar(char* name){
	char* varin;
	File* temp=var->find(name);
	if (temp==NULL){
		return NULL;
	}
	varin=(char*)kmalloc(temp->getSize());
	memset(varin,0,temp->getSize());
	temp->read(0,(u8*)varin,temp->getSize());
	return varin;
}

User* System::getUser(char* nae){
	User* us=listuser;
	while (us!=NULL){
		//io.print("test '%s' with '%s'\n",nae,us->getName());
		if (!strncmp(nae,us->getName(),strlen(us->getName())))
			return us;
		us=us->getUNext();
	}
	return NULL;
}

void System::addUserToList(User* us){
	if (us==NULL)
		return;
	us->setUNext(listuser);
	listuser=us;
}

u32 System::isRoot(){
	if (actual!=NULL){
		if (actual->getUType() == USER_ROOT)
			return 1;
		else
			return 0;
	}
	else
		return 0;
}
