/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** GetFlag.cpp
** 
** Cmdline utility to threadsafe read 0/1 value from file. 
**
** -------------------------------------------------------------------------*/


#include<stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include "SharedMemory.h"

#ifdef BUILD_GETFLAG

int main(int argc, char *argv[]) {

    char flag = '0';
    if (argc > 1)
    {
        MutexFile lockFile(argv[1], false);

        if (lockFile.lock(true))
        {
            int fd = lockFile.getFileId();

            if (lseek(fd, 0, SEEK_SET) == 0)
            {
                read(fd, &flag, 1);
            }

            lockFile.unlock();
        }

        printf("%c", flag);
    }

    return flag;
}

#endif
