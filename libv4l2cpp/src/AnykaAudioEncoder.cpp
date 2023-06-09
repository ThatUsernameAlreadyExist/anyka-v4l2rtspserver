/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaAudioEncoder.cpp
** 
**
** -------------------------------------------------------------------------*/


#include "AnykaAudioEncoder.h"
#include <string.h>
#include "logger.h"


extern "C"
{
	#include "ak_ai.h"
    #include "ak_common.h"
}


static void* memcpySwap16(void *dest, const void *src, size_t n)
{
    uint16_t *dest16 = (uint16_t*)dest;
    const uint16_t *src16 = (const uint16_t*)src;
    const size_t n16 = n / 2;

    for (size_t i = 0; i < n16; ++i)
    {
        *dest16++ = __builtin_bswap16(*src16++);
    }

    return dest;
}


AnykaAudioEncoder::AnykaAudioEncoder()
    : m_memcpy(&memcpy)
{
    INIT_LIST_HEAD(&m_streamData);
}


AnykaAudioEncoder::~AnykaAudioEncoder()
{
    stop();
}


bool AnykaAudioEncoder::isAudioEncoder() const
{
    return true;
}


void AnykaAudioEncoder::onStart(void *device, const audio_param &audioParams)
{
    if (device != NULL)
    {
        m_encoder = ak_aenc_open(&audioParams);
        if (m_encoder != NULL)
        {
            m_memcpy = audioParams.type == AK_AUDIO_TYPE_PCM
                ? &memcpySwap16
                : &memcpy;

            ak_aenc_set_frame_default_interval(m_encoder, 40);

            m_encoderStream =  ak_aenc_request_stream(device, m_encoder);
            if (m_encoderStream != NULL)
            {
                LOG(NOTICE)<<"success open audio encoder"; 
            }
            else
            {
                LOG(ERROR)<<"ak_aenc_request_stream failed";  
                onStop();
            }
        }
        else
        {
            LOG(ERROR)<<"ak_aenc_open failed";  
        }
    }
    else
    {
        LOG(ERROR)<<"empty audio device";  
    }
}


void AnykaAudioEncoder::onStop()
{
    if (m_encoderStream != NULL)
	{
		ak_aenc_cancel_stream(m_encoderStream);
		m_encoderStream = NULL;
	}
    
    if (m_encoder != NULL)
    {
        ak_aenc_close(m_encoder);
        m_encoder = NULL;
    }
}


bool AnykaAudioEncoder::readNewFrameData(FrameRef *outFrame)
{
    bool retVal = false;

    if (isSet())
    {
        if (ak_aenc_get_stream(m_encoderStream, &m_streamData) == AK_SUCCESS)
        {
            struct aenc_entry *entry = NULL;
            struct aenc_entry *ptr   = NULL;
            size_t dataLen           = 0;

            list_for_each_entry_safe(entry, ptr, &m_streamData, list) 
            {
                if(entry) 
                {
                    dataLen += entry->stream.len;
                    if (outFrame->reallocIfNeed(dataLen))
                    {
                        retVal = true;
                        
                        m_memcpy(outFrame->getData() + outFrame->getDataSize(), entry->stream.data, entry->stream.len);
                        outFrame->setDataSize(dataLen);
                    }
                }

                ak_aenc_release_stream(entry);
            }
        }
    }

    return retVal;
}
