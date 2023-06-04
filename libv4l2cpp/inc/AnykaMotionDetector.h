/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaMotionDetector.h
** 
**
** -------------------------------------------------------------------------*/


#ifndef ANYKA_MOTION_DETECTOR
#define ANYKA_MOTION_DETECTOR


#include <string>


class AnykaMotionDetector
{
public:
    AnykaMotionDetector();

    bool start(void *videoDevice, int sensitivity, int fps, int x, int y, int width, int height);  // Position(x,y,w,h) - in percent [0-100]  
    void stop();

    bool detect();

private:
    bool m_isSet;
    bool m_lastDetectState;
    time_t m_lastCheckTime;

};


#endif

