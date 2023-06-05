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
    #include "ak_drv_ir.h"
    #include "ak_drv_irled.h"
    #include "ak_vi.h"
}

#include "logger.h"


AnykaDayNight::AnykaDayNight()
    : m_videoDevice(NULL)
    , m_mode(Day)
{
}


void AnykaDayNight::start(void *videoDevice)
{
    m_videoDevice = videoDevice;
}


void AnykaDayNight::stop()
{
    m_videoDevice = NULL;
}


void AnykaDayNight::setMode(Mode mode)
{
    if (mode == Day)
    {
        setVideo(true);
        setIrLed(false);
        setIrCut(true);
    }
    else if (mode == Night)
    {
        setVideo(false);
        setIrLed(true);
        setIrCut(false);
    }
}


void AnykaDayNight::setVideo(bool isDay)
{
    if (m_videoDevice != NULL)
    {
        if (ak_vi_switch_mode(m_videoDevice, isDay ? VI_MODE_DAY : VI_MODE_NIGHT) != AK_SUCCESS)
        {
            LOG(ERROR)<<"ak_vi_switch_mode failed";
        }
    }
}


void AnykaDayNight::setIrLed(bool enable)
{
    if (ak_drv_irled_set_working_stat(enable ? 1 : 0) != AK_SUCCESS)
    {
        LOG(ERROR)<<"ak_drv_ir_set_ircut failed";  
    }
}


void AnykaDayNight::setIrCut(bool enable)
{
    if (ak_drv_ir_set_ircut(enable ? 1 : 0) != AK_SUCCESS)
    {
        LOG(ERROR)<<"ak_drv_ir_set_ircut failed";
    }
}


void AnykaDayNight::update()
{
    if (m_mode == Auto && m_videoDevice != NULL)
    {

    }
}


