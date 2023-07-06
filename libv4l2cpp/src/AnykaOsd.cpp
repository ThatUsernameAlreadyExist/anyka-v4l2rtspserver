/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaOsd.h
** 
**
** -------------------------------------------------------------------------*/



extern "C"
{
	#include "ak_osd.h"
    #include "ak_common.h"
    #include "ak_vpss.h"
}

#include "AnykaOsd.h"
#include "logger.h"
#include <string.h>


static time_t getSysSeconds()
{
    struct timespec curTime = {0};

    return clock_gettime(CLOCK_MONOTONIC, &curTime) == 0
        ? curTime.tv_sec
        : 0;
}


static size_t ascToShort(unsigned short *dest, const char *src)
{
 	size_t count = 0;

    while (*src) 
    {
		if (*src < 0x80)
        {
	        *dest = *src;
	        dest++;
	        src++;
			count++;
		} 
        else if (*src < 0xA0)
        {
			src++;
		} 
        else if (*(src + 1) < 0xA0) 
        {
			src++;
		} 
        else 
        {
			*dest = ((unsigned short)(*(src + 1) << 8)) | *src;
	        dest++;
	        src += 2;
			count++;
		}
    }

	return count;
}


AnykaOsd::AnykaOsd()
    : m_isSet(false)
    , m_lastUpdateSeconds(0)
{
}


bool AnykaOsd::start(void *videoDevice, const std::string &fontPath, int fontSize)
{
    stop();

    if (videoDevice != NULL && fontPath.size() > 0 && ak_osd_set_font_file(fontSize, fontPath.c_str()) == AK_SUCCESS)	
    {
        LOG(NOTICE)<<"ak_osd_set_font_file success\n";

        if (ak_osd_init(videoDevice) == AK_SUCCESS)
        {
            LOG(NOTICE)<<"ak_osd_init success\n";
           
            m_isSet = true;
        }
        else
        {
            LOG(ERROR)<<"ak_osd_init failed\n";
        }
    }

    return m_isSet;
}


void AnykaOsd::stop()
{
    if (m_isSet)
    {
	    ak_osd_destroy();
        m_isSet = false;
        m_lastUpdateSeconds = 0;
    }
}


void AnykaOsd::update()
{
    if (m_isSet && m_osdMask.size() > 0)
    {
        const time_t seconds  = getSysSeconds();

        if (seconds != m_lastUpdateSeconds)
        {
            m_lastUpdateSeconds = seconds;

            time_t currTime;
            time(&currTime);
            struct tm *currDate = localtime(&currTime);

            if (strftime(m_osd, kOsdMaxSize, m_osdMask.c_str(), currDate) > 0)
            {
                renderText(0, m_osd);
                renderText(1, m_osd);
            }
        }
    }
}


void AnykaOsd::renderText(int channel, const char *text)
{
    const size_t len = ascToShort(m_osdConverted, text);

    if (ak_osd_draw_str(channel, 0, 0, 0, m_osdConverted, len) != AK_SUCCESS) 
    {
        LOG(ERROR)<<"ak_osd_draw_str failed";
    }
}


void AnykaOsd::setOsdText(const std::string &text)
{
    m_osdMask = text;
}


void AnykaOsd::setColor(int frontColot, int backColor, int edgeColor, int alpha)
{
    ak_osd_set_color(frontColot, backColor);
    ak_osd_set_edge_color(edgeColor);
    ak_osd_set_alpha(alpha);
}


void AnykaOsd::setPos(void *videoDevice, int fontSizeHigh, int fontSizeLow, int xHigh, int yHigh, int xLow, int yLow)
{
    const int fSize[2] = {fontSizeHigh, fontSizeLow};
    const int posX[2]  = {(xHigh / 2) * 2, (xLow / 2) * 2};
    const int posY[2]  = {(yHigh / 2) * 2, (yLow / 2) * 2};

    for (int i = 0; i < 2; ++i) 
    {
		int maxW = 0;
        int maxH = 0;

		if (ak_osd_get_max_rect(i, &maxW, &maxH) == AK_SUCCESS)
        {
            ak_osd_set_font_size(i, fSize[i]);

            const int width  = maxW;
		    const int height = fSize[i] * 2;

            if (width > 0 && ak_osd_set_rect(videoDevice, i, 0, posX[i], posY[i], width, height) == AK_SUCCESS)
            {
                LOG(NOTICE)<<"ak_osd_set_rect success: "<< posX[i] <<" - "<< posY[i] << " - " << width << " - " << height;
            }
            else
            {
                LOG(ERROR)<<"ak_osd_set_rect failed";
            }
		}
	}
}


