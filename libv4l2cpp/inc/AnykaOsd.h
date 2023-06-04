/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaOsd.h
** 
**
** -------------------------------------------------------------------------*/


#ifndef ANYKA_OSD
#define ANYKA_OSD


#include <string>


class AnykaOsd
{
public:
    AnykaOsd();

    bool start(void *videoDevice, const std::string &fontPath, int fontSize);   
    void stop();

    void setOsdText(const std::string &text);
    void setColor(int frontColot, int backColor, int edgeColor, int alpha);
    void setPos(void *videoDevice, int fontSize, int x, int y, int lowHighMultiplier);

    void update();

private:
    void renderText(int channel, const char *text);

private:
    static const size_t kOsdMaxSize = 256;

private:
    bool m_isSet;
    time_t m_lastUpdateSeconds;
    unsigned short m_osdConverted[kOsdMaxSize];
    char m_osd[kOsdMaxSize];
    std::string m_osdMask;
};


#endif

