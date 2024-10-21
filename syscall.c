#include "syscall.h"
#include <stdlib.h>
#include <unistd.h>

void syscall_exit(void) { exit(EXIT_SUCCESS); }

ssize_t syscall_write(qword fd, qword *addr, size_t len)
{
    if (fd < 0)
        return -1;

    return write(fd, (const char *)addr, len);
}

ssize_t syscall_read(qword fd, qword *addr, size_t len)
{

    if (fd < 0)
        return -1;

    return read(fd, (char *)addr, len);
}
