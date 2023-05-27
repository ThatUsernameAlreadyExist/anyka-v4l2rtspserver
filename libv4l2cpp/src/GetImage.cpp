/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** GetImage.cpp
** 
** Cmdline utility to get jpeg image from anyka camera/shared memory. 
**
** -------------------------------------------------------------------------*/


#include<stdio.h>
#include <cstdlib>


#include "SharedMemory.h"


#ifdef BUILD_GETIMAGE

int main(int argc, char *argv[]) {

    SharedMemory& mem = SharedMemory::instance();

    void* memory = mem.lockImage();
    if (memory > 0)
    {    
        size_t memLen = mem.getImageSize();
        if (memLen > 0)
        {
            fwrite(memory, memLen, 1, stdout);
        }
        
        mem.unlockImage(memory);
    }

    return 0;
}

#endif
