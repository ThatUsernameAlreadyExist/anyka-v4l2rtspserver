/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2DummyFd.h
** 
**
** -------------------------------------------------------------------------*/

#ifndef V4L2_DUMMY_FD
#define V4L2_DUMMY_FD


class V4l2DummyFd
{

public:
    V4l2DummyFd();
    virtual ~V4l2DummyFd();

    bool signal();
    void reset() const;

    bool isSet() const;
    int getFd() const;


private:
    int m_fd[2];

};

#endif
