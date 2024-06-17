#ifndef KERNEL_H
#define KERNEL_H

#include "../runtime/alloc.h"
#include "../runtime/libc.h"
#include "../runtime/string.h"


#include "file.h"
#include "filesystem.h"
#include "elf_loader.h"
#include "syscalls.h"
#include "env.h"
#include "user.h"
#include "modulelink.h"
#include "device.h"
#include "socket.h"
#include "system.h"


#include "../modules/module.h"


#include "../arch/x86/io.h"
#include "../arch/x86/architecture.h"
#include "../arch/x86/vmm.h"
#include "process.h"

#endif
