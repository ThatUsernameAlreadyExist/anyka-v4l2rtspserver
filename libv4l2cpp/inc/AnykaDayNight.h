/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaDayNight.h
** 
**
** -------------------------------------------------------------------------*/


#ifndef ANYKA_DAY_NIGHT
#define ANYKA_DAY_NIGHT


class AnykaDayNight
{
public:
    enum Mode
    {
        Night = 0,
        Day = 1,
        Auto = 2,
        Disabled = 3
    };

public:
    AnykaDayNight();

    void start(void *videoDevice);
    void stop();
    void setMode(Mode mode);
    void setVideo(bool isDay);
    void setIrLed(bool enable);
    void setIrCut(bool enable);

    void update();

private:
    void *m_videoDevice;
    Mode m_mode;

};


#endif

