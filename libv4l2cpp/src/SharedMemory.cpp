/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** SharemMemory.cpp
** 
**
** -------------------------------------------------------------------------*/

#include "SharedMemory.h"
#include <cstdlib>


MutexFile::MutexFile(const char *name, bool isWriteLock)
    : fd(-1)
    , writeLock(isWriteLock)
    , bLocked(false)
{
    if (name)
    {
        int flags = writeLock ? O_WRONLY : O_RDONLY;
        fd = open(name, flags | O_CREAT, 0666);

        lockdata.l_whence = SEEK_SET;
        lockdata.l_start = 0;
        lockdata.l_len = 0;
        lockdata.l_pid = getpid();
    }
}


MutexFile::~MutexFile()
{
    if (fd >= 0)
    {
        close(fd);
    }
}


bool MutexFile::lock(bool wait)
{
    bool isLocked = false;

    if (fd >= 0)
    {
        lockdata.l_type = writeLock ? F_WRLCK : F_RDLCK;
        int lockWaitType = wait ? F_SETLKW : F_SETLK;

        bLocked = (fcntl(fd, lockWaitType, &lockdata) >= 0);    
    }

    return bLocked;
}


void MutexFile::unlock()
{
    if (fd >= 0)
    {
        lockdata.l_type = F_UNLCK;
        bLocked = (fcntl(fd, F_SETLKW, &lockdata) < 0);
    }
}


int MutexFile::getFileId() const
{
    return fd;
}


bool MutexFile::isLocked() const
{
    return bLocked;
}


SharedMemory& SharedMemory::instance()
{
    static SharedMemory _instance;
    return _instance;
}


SharedMemory::SharedMemory() 
    : imageReadLock("/tmp/image.lock", false)
    , imageWriteLock("/tmp/image.lock", true)
    , configReadLock("/tmp/config.lock", false)
    , configWriteLock("/tmp/config.lock", true)
{
    currentConfig.nightmode         = 2;
    currentConfig.configFilePath[0] = 0;
    currentConfig.dayNightAwb       = 9000;
    currentConfig.dayNightLum       = 6000;
    currentConfig.irCut             = true;
    currentConfig.irLed             = false;
    currentConfig.motionEnabled     = true;
    currentConfig.motionSensitivity = 60;
    currentConfig.nightDayAwb       = 1200;
    currentConfig.nightDayLum       = 2000;
    currentConfig.nightmode         = 2;
    currentConfig.osdAlpha          = 0;
    currentConfig.osdBackColor      = 0;
    currentConfig.osdEdgeColor      = 2;
    currentConfig.osdEnabled        = false;
    currentConfig.osdFrontColor     = 1;
    currentConfig.osdText[0]        = 0; 
    currentConfig.osdFontSizeHigh   = 32;
    currentConfig.osdXHigh          = 20;
    currentConfig.osdYHigh          = 24;
    currentConfig.osdFontSizeLow    = 16;
    currentConfig.osdXLow           = 10;
    currentConfig.osdYLow           = 12;
    currentConfig.videoDay          = true;

    keyImageMem  = ftok("/usr/", '1');
    keyConfigMem = ftok("/usr/", '3');
    keyImageSize = ftok("/usr/", '5');
}


size_t SharedMemory::getImageSize() 
{
    size_t imSize = 0;

    this->readMemory(keyImageSize, (void*)&imSize, sizeof(imSize));

    return imSize;
}


void* SharedMemory::lockImage()
{
    void *mem = 0;
    
    if (this->imageReadLock.lock(true))
    {       
        int shmId = shmget(keyImageMem, 0, 0);
        if (shmId != -1) 
        {
            mem = shmat(shmId, NULL, 0);
        }    
        
        if (mem <= 0)
        {
            this->imageReadLock.unlock();
        }
    }
    
    return mem;
}


void* SharedMemory::lockImage(size_t imageSize)
{
    void *mem = 0;
    
    if (this->imageWriteLock.lock(true))
    { 
        int shmId = shmget(keyImageMem, 0, 0);

        if (shmId != -1) 
        {
            size_t memLen = this->getMemorySize(keyImageMem);

            if (memLen < imageSize) 
            {
                shmctl(shmId, IPC_RMID, NULL);  
                shmId = -1;
            }
        }

        if (shmId == -1)
        {
            shmId = shmget(keyImageMem, imageSize, IPC_CREAT);
        }
        
        if (shmId != -1) 
        {
            mem = shmat(shmId, NULL, 0);
        }
        
        if (mem <= 0)
        {
            this->imageWriteLock.unlock();
        }
        else
        {
            this->writeMemory(keyImageSize, (void*)&imageSize, sizeof(imageSize));
        }
    }

    return mem;
}


void SharedMemory::unlockImage(void *mem)
{
    if (mem > 0)
    {
        shmdt(mem);

        if (this->imageWriteLock.isLocked())
        {
            this->imageWriteLock.unlock();
        }
        else if (this->imageReadLock.isLocked())
        {
            this->imageReadLock.unlock();
        }
    }
}


SharedConfig *SharedMemory::getConfig() 
{
    return &currentConfig;
}


SharedConfig* SharedMemory::readConfig()
{
    this->configReadLock.lock(true);
    this->readMemory(keyConfigMem, &currentConfig, sizeof(SharedConfig));
    this->configReadLock.unlock();

    return getConfig();
}


void SharedMemory::writeConfig() 
{
    this->configWriteLock.lock(true);
    this->writeMemory(keyConfigMem, &currentConfig, sizeof(SharedConfig));
    this->configWriteLock.unlock();
}


void SharedMemory::readMemory(key_t key, void *memory, size_t memorylenght) 
{
    void *mem = NULL;

    int shmId = shmget(key, 0, 0);
    if (shmId == -1) 
    {
        return;
    }

    mem = shmat(shmId, NULL, 0);
    memcpy(memory, mem, memorylenght);
    shmdt(mem);
}


size_t SharedMemory::getMemorySize(key_t key) 
{
    int shmId = shmget(key, 0, 0);
    struct shmid_ds buf;
    shmctl(shmId, IPC_STAT, &buf);
    int memLen = buf.shm_segsz;
    return  memLen > 0 ? (size_t) memLen : 0;
}


void SharedMemory::writeMemory(key_t key, void *memory, size_t memorylenght) 
{
    int shmId = shmget(key, 0, 0);

    if (shmId != -1) 
    {
        size_t memLen = this->getMemorySize(key);
        if (memLen != memorylenght) 
        {
            shmctl(shmId, IPC_RMID, NULL);
        }
    }

    shmId = shmget(key, memorylenght, IPC_CREAT);
    if (shmId != -1) 
    {
        void *sharedMem = shmat(shmId, NULL, 0);
        memcpy(sharedMem, memory, memorylenght);
        shmdt(sharedMem);
    }
}

