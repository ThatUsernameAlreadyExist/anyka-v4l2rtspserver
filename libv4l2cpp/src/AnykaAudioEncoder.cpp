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


AnykaAudioEncoder::AnykaAudioEncoder()
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
            if (audioParams.type == AK_AUDIO_TYPE_AAC) 
            {
                struct aenc_attr attr;
                attr.aac_head = AENC_AAC_SAVE_FRAME_HEAD;
                ak_aenc_set_attr(m_encoder, &attr); 
            }

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


size_t AnykaAudioEncoder::readNewFrameData()
{
    size_t retVal = 0;

    if (ak_aenc_get_stream(m_encoderStream, &m_streamData) == AK_SUCCESS)
    {
        struct aenc_entry *entry = NULL;
	    struct aenc_entry *ptr = NULL;

        list_for_each_entry_safe(entry, ptr, &m_streamData, list) 
        {
            if(entry) 
            {
                retVal += entry->stream.len;
            }
        }
    }

    return retVal;
}


size_t AnykaAudioEncoder::copyNewFrameDataTo(char* buffer, size_t bufferSize)
{
    size_t retVal = 0;

    struct aenc_entry *entry = NULL;
    struct aenc_entry *ptr   = NULL;
    size_t pos = 0;

    list_for_each_entry_safe(entry, ptr, &m_streamData, list) 
    {
        if(entry) 
        {
            const size_t toCopy = std::min(entry->stream.len, bufferSize - pos);
            memcpy(buffer + pos, entry->stream.data, toCopy);

            retVal += toCopy;

            if (retVal >= bufferSize)
            {
                break;
            }
        }
    }

    return retVal;
}


void AnykaAudioEncoder::releaseFrameData()
{
    struct aenc_entry *entry = NULL;
    struct aenc_entry *ptr = NULL;

    list_for_each_entry_safe(entry, ptr, &m_streamData, list) 
    {
		ak_aenc_release_stream(entry);
    }
}





