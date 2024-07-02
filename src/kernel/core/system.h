
#ifndef SYSTEM_H
#define SYSTEM_H

#include "../runtime/types.h"
#include "env.h"
#include "user.h"


class System
{
	public:
		System();
		~System();
		
		void	init();
		char*	getvar(char* name);
		
		
		void	addUserToList(User* us);
		
		User*	getUser(char* nae);
		
		int		login(User* us,char* pass);
		u32 	isRoot();			//returns 1 if root
		
		
	private:
		User*		listuser;
	
		File*		var;
		
		User*		actual;
		User*		root;
		
		Variable*	uservar;
};

extern System		sys;
#endif
