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


void AnykaVideoEncoder::onStart(void *device, const encode_param &videoParams)
{
    if (device != NULL)
    {
        m_encoder = ak_venc_open(&videoParams);

        if (m_encoder != NULL)
        {
            LOG(NOTICE)<<"ak_venc_open success";
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


size_t AnykaVideoEncoder::readNewFrameData()
{
    return ak_venc_get_stream(m_encoderStream, &m_streamData) == AK_SUCCESS
        ? m_streamData.len
        : 0;
}


size_t AnykaVideoEncoder::copyNewFrameDataTo(char* buffer, size_t bufferSize)
{	
    size_t retVal = 0;

    if (m_streamData.len > 0 && m_streamData.data != NULL)
    {
        retVal = std::min(m_streamData.len, bufferSize);
        memcpy(buffer, m_streamData.data, retVal);
    }

    return retVal;
}


void AnykaVideoEncoder::releaseFrameData()
{
    if (m_streamData.len > 0 && m_streamData.data != NULL && m_encoderStream != NULL)
    {
        ak_venc_release_stream(m_encoderStream, &m_streamData);
        m_streamData = {0};
    }
}






