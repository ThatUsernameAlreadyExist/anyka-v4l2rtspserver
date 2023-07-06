/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaVideoEncoder.cpp
** 
**
** -------------------------------------------------------------------------*/


#include "AnykaVideoEncoder.h"
#include <string.h>
#include "logger.h"

extern "C"
{
    #include "ak_common.h"
}


AnykaVideoEncoder::AnykaVideoEncoder()
    : m_streamData({0})
{
}


AnykaVideoEncoder::~AnykaVideoEncoder()
{
    stop();
}


bool AnykaVideoEncoder::isAudioEncoder() const
{
    return false;
}


void AnykaVideoEncoder::onStart(void *device, const VideoEncodeParam &videoParams)
{
    if (device != NULL)
    {
        m_encoder = ak_venc_open(&videoParams.videoParams);

        if (m_encoder != NULL)
        {
            LOG(NOTICE)<<"ak_venc_open success";

            if (videoParams.videoParams.br_mode == BR_MODE_VBR)
            {
                if (ak_venc_set_kbps(m_encoder, videoParams.targetKbps, videoParams.maxKbps) != AK_SUCCESS)
                {
                    LOG(ERROR)<<"Error set kbps for VBR mode. Target: "<<videoParams.targetKbps << " max: " << videoParams.maxKbps;
                }

                if (videoParams.smartParams.smart_mode == 1 || videoParams.smartParams.smart_mode == 2) //1:mode of LTR, 2:mode of changing GOP length
                {
                    venc_smart_cfg smartCfg = videoParams.smartParams;
                    if (ak_venc_set_smart_config(m_encoder, &smartCfg) != AK_SUCCESS)
                    {
                        LOG(ERROR)<<"Error set smart cfg for VBR mode";
                    }
                }
            }

            m_encoderStream = ak_venc_request_stream(device, m_encoder);

            if (m_encoderStream != NULL)
            {
                LOG(NOTICE)<<"ak_venc_request_stream success";
            }
            else
            {
                LOG(ERROR)<<"ak_venc_request_stream faied";
                onStop();
            }
        }
        else
        {
            LOG(ERROR)<<"ak_venc_open failed";
        }
    }
}


void AnykaVideoEncoder::onStop()
{
    if (m_encoderStream != NULL)
	{
		ak_venc_cancel_stream(m_encoderStream);
		m_encoderStream = NULL;
	}

	if (m_encoder != NULL)
	{
		ak_venc_close(m_encoder);
		m_encoder = NULL;
	}
}


bool AnykaVideoEncoder::readNewFrameData(FrameRef *outFrame)
{
    bool retVal = false;

    size_t dataSize = 0;

    for (size_t i = 0; i < 2; ++i)
    {
        if (ak_venc_get_stream_ex(m_encoderStream, outFrame->getData(), 
                outFrame->getFullSize(), &dataSize) == AK_SUCCESS)
        {
            outFrame->setDataSize(dataSize);
            retVal = true;
            break;
        }
        else if (dataSize == 0 || !outFrame->reallocIfNeed(dataSize))
        {
            break;
        }
    }

    return retVal;
}
