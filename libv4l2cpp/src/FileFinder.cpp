/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** FileFinder.h
**
**
** -------------------------------------------------------------------------*/


#include "FileFinder.h"


FileFinder::FileFinder()
    : m_info({0})
    , m_needClear(false)
{}


FileFinder::~FileFinder()
{
    clear();
}


std::vector<const char*> FileFinder::findByMask(const std::string &pathMask)
{
    std::vector<const char*> retVal;

    clear();

    if (pathMask.size() > 0)
    {
        if (glob(pathMask.c_str(), 0, NULL, &m_info) == 0)
        {
            for (int i = 0; i < m_info.gl_pathc; ++i)
            {
                retVal.push_back(m_info.gl_pathv[i]);
            }
        }
    }

    return retVal;
}


void FileFinder::clear()
{
    if (m_needClear)
    {
        globfree(&m_info);
        m_needClear = false;
    }

    m_info.gl_pathc = 0;
    m_info.gl_pathv = NULL;
    m_info.gl_offs  = 0;
}


