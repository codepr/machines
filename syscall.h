#ifndef SYSCALL_H
#define SYSCALL_H

#include "bytecode.h"
#include <stdio.h>

void syscall_exit(void);
ssize_t syscall_write(qword fd, qword *addr, size_t len);
ssize_t syscall_read(qword fd, qword *addr, size_t len);

#endif
