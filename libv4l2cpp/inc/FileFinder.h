/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** FileFinder.h
**
**
** -------------------------------------------------------------------------*/

#ifndef FILE_FINDER
#define FILE_FINDER

#include <glob.h>
#include <string>
#include <vector>

class FileFinder
{
public:
    FileFinder();
    ~FileFinder();

    // Note: the result is valid only until the next function call or the end of the FileFinder object lifetime.
    // Not thread-safe.
    std::vector<const char*> findByMask(const std::string &pathMask);

private:
    void clear();

private:
    glob_t m_info;
    bool   m_needClear;

};

#endif
