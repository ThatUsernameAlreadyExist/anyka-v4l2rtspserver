/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaEncoderBase.cpp
** 
**
** -------------------------------------------------------------------------*/


#include "AnykaEncoderBase.h"
#include <string.h>
#include "logger.h"


const size_t kDefaultMaxBufferSize = 384 * 1024;


AnykaEncoderBase::AnykaEncoderBase()
	: m_bufferSize(kDefaultMaxBufferSize)
    , m_encoder(NULL)
	, m_encoderStream(NULL)
	, m_isNewFrameAvailable(false)
{
}


bool AnykaEncoderBase::start(void *videoDevice, void *audioDevice, const encode_param &videoParams, const audio_param &audioParams)
{
    if (isAudioEncoder())
    {
        onStart(audioDevice, audioParams);
    }
    else
    {
        onStart(videoDevice, videoParams); 
    }

    return isSet();
}


void AnykaEncoderBase::onStart(void *device, const encode_param &videoParams)
{
}


void AnykaEncoderBase::onStart(void *device, const audio_param &audioParams)
{
}


void AnykaEncoderBase::stop()
{
    m_isNewFrameAvailable = false;
    m_signalFd.reset();

    m_streamDataLock.lock();

    releaseFrameData();
    m_bufferSize = 0;

    m_streamDataLock.unlock();

    onStop();
}


bool AnykaEncoderBase::isSet() const
{
    return m_encoder != NULL && m_encoderStream != NULL;
}


bool AnykaEncoderBase::encode()
{
    bool retVal = false;

	if (!m_isNewFrameAvailable && isSet()) // Encode new frame only if previous is delivered to caller.
	{
		m_streamDataLock.lock();

        // Release previous frame.
        releaseFrameData();

        // Try encode new frame.
		m_bufferSize = readNewFrameData();

		m_streamDataLock.unlock();

        if (m_bufferSize > 0)
		{
            retVal = true;
			m_isNewFrameAvailable = true;
            m_signalFd.signal();
		}
	}

	return retVal;
}


int AnykaEncoderBase::getEncodedFrameReadyFd() const
{
    return m_signalFd.getFd();
}


size_t AnykaEncoderBase::getEncodedFrameSize() const
{
    return m_bufferSize;
}


size_t AnykaEncoderBase::getEncodedFrame(char* buffer, size_t bufferSize)
{
	size_t retVal = 0;

	if (m_isNewFrameAvailable)
	{
        if (bufferSize > 0 && buffer != NULL)
        {
            m_streamDataLock.lock();

            retVal = copyNewFrameDataTo(buffer, bufferSize);

            m_streamDataLock.unlock();
        }

        m_signalFd.reset();
        m_isNewFrameAvailable = false;
	}

	return retVal;
}





