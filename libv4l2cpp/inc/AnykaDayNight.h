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


#include <atomic>

extern "C"
{
	#include "ak_thread.h" 
}


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
    ~AnykaDayNight();

    void start(void *videoDevice, int minDayToNightLum, int minNightToDayLum, int maxDayToNightAwb, int minNightToDayAwb);
    void stop();
    void setMode(Mode mode);
    void setVideo(bool isDay);
    void setIrLed(bool enable);
    void setIrCut(bool enable);
    void setPrintInfo(bool enable);

private:
    void setDay();
    void setNight();

    void startAutoModeThread();
    void stopAutoModeThread();
    void processAutoModeThread();
    void printCurrentAutoParams();

    static void* thread(void *arg);

private:
    void *m_videoDevice;
    bool m_printInfo;
    std::atomic<int> m_dayStatus;
    int m_minDayToNightLum;
    int m_minNightToDayLum;
    int m_maxDayToNightAwb;
    int m_minNightToDayAwb;

    ak_pthread_t m_threadId;
	std::atomic_bool m_threadStopFlag;

};


#endif

