/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaMotionDetector.h
** 
**
** -------------------------------------------------------------------------*/


#include "AnykaMotionDetector.h"

extern "C"
{
    #include "ak_common.h"
    #include "ak_md.h"
}

#include "logger.h"


const time_t kCheckIntervalMs = 250;


static time_t getCheckTime()
{
    struct timespec curTime = {0};

    return clock_gettime(CLOCK_MONOTONIC, &curTime) == 0
        ? curTime.tv_sec * 1000 / kCheckIntervalMs + curTime.tv_nsec / 1e6 / kCheckIntervalMs
        : 0;
}


AnykaMotionDetector::AnykaMotionDetector()
    : m_isSet(false)
    , m_lastDetectState(false)
    , m_lastCheckTime(0)
{
}


bool AnykaMotionDetector::start(void *videoDevice, int sensitivity, int fps, int x, int y, int width, int height)
{
    stop();

    if (ak_md_init(videoDevice) == AK_SUCCESS)
    {
        LOG(NOTICE)<<"ak_md_init success";

        bool isAreaSet = false;

        int mdWidth = 0;
        int mdHeight = 0;

        if (ak_md_get_dimension_max(&mdWidth, &mdHeight) == AK_SUCCESS)
        {
            LOG(NOTICE)<<"ak_md_get_dimension_max success: "<<mdWidth <<" x "<<mdHeight;

            const int startX = std::min(x * mdWidth / 100, mdWidth - 1);
            const int startY = std::min(y * mdHeight / 100, mdHeight - 1);
            const int endX   = std::min(startX + mdWidth * width / 100, mdWidth);
            const int endY   = std::min(startY + mdHeight * height / 100, mdHeight);

            LOG(NOTICE)<<"Real MD area: "<<startX<<"-"<<endX<<" x "<<startY<<"-"<<endY;

            if (startX == 0 && startY == 0 && endX == mdWidth && endY == mdHeight)
            {
                // Use whole image as detection area.
                if (ak_md_set_global_sensitivity(sensitivity) == AK_SUCCESS)
                {
                    LOG(NOTICE)<<"ak_md_set_global_sensitivity success: "<<sensitivity;
                    isAreaSet = true;
                }
                else
                {
                    LOG(ERROR)<<"ak_md_set_global_sensitivity failed";
                }
            }
            else
            {
                int motionArea[mdWidth * mdHeight];

                // Set default minimum sensitivity.
                for (int y = 0; y < mdWidth * mdHeight; ++y)
                {
                    motionArea[y] = 1;
                }

                for (int y = startY; y < endY; ++y)
                {
                    const int line = y * mdWidth;
                    for (int x = startX; x < endX; ++x)
                    {
                        motionArea[x + line] = sensitivity;
                    }
                }

                if (ak_md_set_area_sensitivity(mdWidth, mdHeight, motionArea) == AK_SUCCESS)
                {
                    LOG(NOTICE)<<"ak_md_set_area_sensitivity success";
                    isAreaSet = true;
                }
                else
                {
                    LOG(ERROR)<<"ak_md_set_area_sensitivity failed";
                }
            }
        }

        if (isAreaSet)
        {
            if (ak_md_set_fps(fps) != AK_SUCCESS)
            {
                LOG(ERROR) << "ak_md_set_fps failed";
            }

            if (ak_md_enable(1) == AK_SUCCESS)
            {
                m_isSet = true;
                LOG(NOTICE) << "ak_md_enable success";
            }
            else
            {
                LOG(ERROR) << "ak_md_enable failed";
            }
        }

        if (!m_isSet)
        {
            ak_md_destroy();
        }
	}
    else
    {
        LOG(ERROR)<<"ak_md_init failed";
    }

    return m_isSet;
}  


void AnykaMotionDetector::stop()
{
    if (m_isSet)
    {
        m_isSet = false;
        ak_md_enable(0);
        ak_md_destroy();
    }

    m_lastDetectState = false;
}


bool AnykaMotionDetector::detect()
{
    if (m_isSet)
    {
        const time_t curTime = getCheckTime();
        
        if (curTime != m_lastCheckTime)
        {
            m_lastCheckTime = curTime;
            int detectTime = 0;
            m_lastDetectState = ak_md_get_result(&detectTime, NULL, 0) == 1;
        }
    }

    return m_lastDetectState;
}



