/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** SharemMemory.h
** 
**
** -------------------------------------------------------------------------*/

#ifndef SHARED_MEMORY
#define SHARED_MEMORY

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/sem.h>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>

#define MAX_STR_SIZE 256

struct SharedConfig 
{
    int nightmode;
    int osdFontSizeHigh;
    int osdFontSizeLow;
    int osdXHigh;
    int osdYHigh;
    int osdXLow;
    int osdYLow;
    int osdFrontColor;
    int osdBackColor;
    int osdEdgeColor;
    int osdAlpha;
    int motionSensitivity;
    int dayNightLum;
    int nightDayLum;
    int dayNightAwb;
    int nightDayAwb;
    int imageFlip;
    bool osdEnabled;
    bool motionEnabled;
    bool irLed;
    bool irCut;
    bool videoDay;
    char osdText[MAX_STR_SIZE];
    char configFilePath[MAX_STR_SIZE];
};


class MutexFile
{
public:
    MutexFile(const char *name, bool isWriteLock);
    ~MutexFile();

    bool lock(bool wait);
    void unlock();
    int getFileId() const;
    bool isLocked() const; // Return only current state (if previously we call lock() on this object).

public:
    int fd;
    struct flock lockdata;
    bool writeLock; 
    bool bLocked;
};


class SharedMemory 
{
public:
    static SharedMemory& instance();

    SharedConfig* getConfig();
    SharedConfig* readConfig();
    void writeConfig();

    size_t getImageSize();
    void* getImage();
    void* lockImage();                 // Lock image for reading.
    void* lockImage(size_t imageSize); // Lock image for writing.
    void unlockImage(void *mem);

private:
    SharedMemory();
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    void readMemory(key_t key, void *memory, size_t memorylenght);
    void writeMemory(key_t key, void *memory, size_t memorylenght);
    size_t getMemorySize(key_t key);
    
private:
    struct SharedConfig currentConfig;
    key_t keyImageMem;
    key_t keyImageSize;
    key_t keyConfigMem;

    MutexFile imageReadLock;
    MutexFile imageWriteLock;
    MutexFile configReadLock;
    MutexFile configWriteLock;
};


#endif
