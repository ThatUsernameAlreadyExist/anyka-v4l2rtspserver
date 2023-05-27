/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2DummyFd.h
** 
**
** -------------------------------------------------------------------------*/


#include "V4l2DummyFd.h"
#include <unistd.h>
#include <fcntl.h>


V4l2DummyFd::V4l2DummyFd()
    : m_fd{0, 0}
{
    pipe2(m_fd, O_CLOEXEC | O_NONBLOCK);
}


V4l2DummyFd::~V4l2DummyFd()
{
    if (m_fd[0] != 0)
    {
        close(m_fd[0]);
    }

    if (m_fd[1] != 0)
    {
        close(m_fd[1]);
    }
}


bool V4l2DummyFd::isSet() const
{
    return m_fd[0] != 0 && m_fd[1] != 0;
}


bool V4l2DummyFd::signal()
{
    return isSet() && write(m_fd[1], "1", 1) > 0;
}


void V4l2DummyFd::reset() const
{
    if (isSet())
    {
        char buff;
        read(m_fd[0], &buff, 1);
    }
}


int V4l2DummyFd::getFd() const
{
    return m_fd[0];
}

