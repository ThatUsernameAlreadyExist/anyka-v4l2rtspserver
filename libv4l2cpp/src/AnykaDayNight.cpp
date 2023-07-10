/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaDayNight.cpp
** 
**
** -------------------------------------------------------------------------*/

#include "AnykaDayNight.h"

extern "C"
{
    #include "ak_common.h"
    #include "ak_vpss.h"
    #include "ak_vi.h"
}

#include <cstdio>
#include "logger.h"


#define IRCUT_A_FILE_NAME    "/sys/user-gpio/gpio-ircut_a"
#define IRCUT_B_FILE_NAME    "/sys/user-gpio/gpio-ircut_b"
#define IRLED_FILE_NAME      "/sys/user-gpio/ir-led"
#define IRCUR_STATUS_FILE    "/var/run/ircut"
#define VIDEODAY_STATUS_FILE "/var/run/vday"
#define LUM_VALUE_FILE       "/var/run/lum"
#define AWB_VALUE_FILE       "/var/run/awb"


static bool writeIntToFile(const char *name, int value)
{
    bool retVal = false;

    FILE *pFile = std::fopen(name, "wb");

    if (pFile != NULL)
    {
        retVal = fprintf(pFile, "%d", value) > 0; 
        std::fclose(pFile);
    }  

    return retVal;
}


static bool writeFlagToFile(const char *name, bool enable)
{
    return writeIntToFile(name, enable ? 1 : 0);
}


AnykaDayNight::AnykaDayNight()
    : m_videoDevice(NULL)
    , m_printInfo(false)
    , m_dayStatus(1)
    , m_minDayToNightLum(6000)
    , m_minNightToDayLum(2000)
    , m_maxDayToNightAwb(90000)
    , m_minNightToDayAwb(1200)
    , m_threadId(0)
    , m_threadStopFlag(false)
{
}


AnykaDayNight::~AnykaDayNight()
{
    stop();
}


void AnykaDayNight::start(void *videoDevice, int minDayToNightLum, int minNightToDayLum, int maxDayToNightAwb, int minNightToDayAwb)
{
    stop();

    m_minDayToNightLum = minDayToNightLum;
    m_minNightToDayLum = minNightToDayLum;
    m_maxDayToNightAwb = maxDayToNightAwb;
    m_minNightToDayAwb = minNightToDayAwb;
    m_videoDevice      = videoDevice;
}


void AnykaDayNight::stop()
{
    stopAutoModeThread();
    m_videoDevice = NULL;
}


void AnykaDayNight::setMode(Mode mode)
{
    stopAutoModeThread();

    if (mode == Auto)
    {
        setDay();

        startAutoModeThread();
    }
    else if (mode == Day)
    {
        setDay();
    }
    else if (mode == Night)
    {
        setNight();
    }
}


void AnykaDayNight::setDay()
{
    setVideoDay(true);
    setIrLed(false);
    setIrCut(true);
    m_dayStatus = 1;
}


void AnykaDayNight::setNight()
{
    setVideoDay(false);
    setIrLed(true);
    setIrCut(false);
    m_dayStatus = 0;
}


bool AnykaDayNight::setVideoDay(bool isDay)
{
    bool retVal = false;

    if (m_videoDevice != NULL)
    {
        if (ak_vi_switch_mode(m_videoDevice, isDay ? VI_MODE_DAY : VI_MODE_NIGHT) == AK_SUCCESS)
        {
            retVal = true;
            writeFlagToFile(VIDEODAY_STATUS_FILE, isDay);
        }
        else
        {
            LOG(ERROR)<<"ak_vi_switch_mode failed";
        }
    }

    return retVal;
}


void AnykaDayNight::setIrLed(bool enable)
{
    if (!writeFlagToFile(IRLED_FILE_NAME, enable))
    {
        LOG(ERROR)<<"write "<<IRLED_FILE_NAME<<" failed";  
    }
}


void AnykaDayNight::setIrCut(bool enable)
{
    bool isSuccess = enable
        ? writeFlagToFile(IRCUT_B_FILE_NAME, true) && writeFlagToFile(IRCUT_A_FILE_NAME, false)
        : writeFlagToFile(IRCUT_A_FILE_NAME, true) && writeFlagToFile(IRCUT_B_FILE_NAME, false);

    ak_sleep_ms(10);

    isSuccess = writeFlagToFile(IRCUT_A_FILE_NAME, false) && writeFlagToFile(IRCUT_B_FILE_NAME, false) && isSuccess;

    if (isSuccess)
    {
        writeFlagToFile(IRCUR_STATUS_FILE, enable);
    }
    else
    {
        LOG(ERROR)<<"write "<<IRCUT_A_FILE_NAME<<" "<<IRCUT_B_FILE_NAME<<" failed";
    }
}


void AnykaDayNight::setPrintInfo(bool enable)
{
    m_printInfo = enable;
}


void AnykaDayNight::resetCurrentAutoStatus()
{
    m_dayStatus = -1;
}


void AnykaDayNight::startAutoModeThread()
{
    stopAutoModeThread();

    if (m_videoDevice != NULL)
    {
        if (ak_thread_create(&m_threadId, AnykaDayNight::thread, this, ANYKA_THREAD_MIN_STACK_SIZE, 10) != AK_SUCCESS)
        {
            LOG(ERROR)<<"Create Day/Night thread failed"; 
        }
    }
}


void AnykaDayNight::stopAutoModeThread()
{
    if (m_threadId != 0)
	{
		m_threadStopFlag = true;
		ak_thread_join(m_threadId);
		m_threadId = 0;
		m_threadStopFlag = false;
	}
}


void AnykaDayNight::printCurrentAutoParams()
{
    if (m_printInfo)
    {
        struct vpss_isp_awb_stat_info infoPre;
        struct vpss_isp_awb_stat_info infoCur;

        if (ak_vpss_isp_get_awb_stat_info(m_videoDevice, &infoPre) == AK_SUCCESS &&
            ak_vpss_isp_get_awb_stat_info(m_videoDevice, &infoCur) == AK_SUCCESS) 
        {
            writeIntToFile(LUM_VALUE_FILE, ak_vpss_isp_get_cur_lumi());

            int maxAwb = 0;
            for (size_t i = 0; i < 10; i++) 
            {
                const int totalCnt = (infoPre.total_cnt[i] + infoCur.total_cnt[i]) / 2;
                if (totalCnt > maxAwb)
                {
                    maxAwb = totalCnt;
                }
            }

            writeIntToFile(AWB_VALUE_FILE, maxAwb);
        }
        else
        {
            LOG(ERROR)<<"ak_vpss_isp_get_awb_stat_info failed";
        }
    }
}


void AnykaDayNight::processAutoModeThread()
{
    ak_vpss_isp_clean_auto_day_night_param();

    // In NIGHT mode: if 'cur lum' < 'm_minNightToDayLum' && 'max cur awb' > 'm_minNightToDayAwb' => transit to DAY
    // In DAY mode  : if 'cur lum' > 'm_minDayToNightLum" && 'max cur awb' < 'm_maxDayToNightAwb' => transit to NIGHT

    struct ak_auto_day_night_threshold threshold;
    threshold.day_to_night_lum = m_minDayToNightLum;
    threshold.night_to_day_lum = m_minNightToDayLum;
    threshold.lock_time = LOCK_TIME;
    threshold.quick_switch_mode = 0;
    
    for (size_t i = 0; i < NIGHT_ARRAY_NUM; i++)
    {
        threshold.night_cnt[i] = m_minNightToDayAwb;
    }

    for (size_t i = 0; i < DAY_ARRAY_NUM; i++) 
    {
        threshold.day_cnt[i] = m_maxDayToNightAwb;
    }

    ak_vpss_isp_set_auto_day_night_param(&threshold);

    while (!m_threadStopFlag)
    {
        const int oldDayStatus = m_dayStatus;
        const int newDayStatus = ak_vpss_isp_get_auto_day_night_level(oldDayStatus);

        if (newDayStatus != AK_FAILED)
        {          
            printCurrentAutoParams();

            if (oldDayStatus != newDayStatus)
            {
                if (newDayStatus == 1)
                {
                    setDay();
                }
                else
                {
                    setNight();
                }
            }
        }
        else
        {
            LOG(ERROR)<<"ak_vpss_isp_get_auto_day_night_level failed";
        }

		ak_sleep_ms(100);
	}
}


void* AnykaDayNight::thread(void *arg)
{
    AnykaDayNight *ptr = static_cast<AnykaDayNight*>(arg);

    LOG(DEBUG)<<"AnykaDayNight thread started";

    ptr->processAutoModeThread();

	ak_thread_exit();

    LOG(DEBUG)<<"AnykaDayNight thread stopped";

	return NULL;
}




