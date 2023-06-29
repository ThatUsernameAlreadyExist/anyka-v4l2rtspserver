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
const FrameRef kEmptyFrameRef;


AnykaEncoderBase::AnykaEncoderBase()
    : m_encoder(NULL)
	, m_encoderStream(NULL)
	, m_freeFrame(m_frameBuffer.getFreeFrame())
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
    m_signalFd.reset();

    m_encodedFramesLock.lock();
    m_encodedFrames.clear();
    m_encodedFramesLock.unlock();

    m_frameBuffer.clear();
    m_freeFrame = m_frameBuffer.getFreeFrame();

    onStop();
}


bool AnykaEncoderBase::isSet() const
{
    return m_encoder != NULL && m_encoderStream != NULL;
}


bool AnykaEncoderBase::encode()
{
    bool retVal = false;

	if (isSet())
	{
        // Try encode new frame.
        if (readNewFrameData(&m_freeFrame))
        {
            m_encodedFramesLock.lock();

            const bool needSignal = m_encodedFrames.size() == 0;

            m_encodedFrames.push_back(m_freeFrame);

            if (m_encodedFrames.size() > 10)
            {
                LOG(WARN)<<"Encoded buffer overflow: "<<m_encodedFrames.size()<<"\n";
            }

            m_encodedFramesLock.unlock();

            if (needSignal)
            {
                m_signalFd.signal();
            }

            m_freeFrame = m_frameBuffer.getFreeFrame();
            retVal = true;
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
    m_encodedFramesLock.lock();

    const size_t retVal = m_encodedFrames.size() > 0
        ? m_encodedFrames.front().getDataSize()
        : kDefaultMaxBufferSize;

    m_encodedFramesLock.unlock();

    return retVal;
}


size_t AnykaEncoderBase::getEncodedFrame(char* buffer, size_t bufferSize)
{
	size_t retVal = 0;

    const FrameRef frame = getEncodedFrame();

    if (frame.isSet())
    {
        retVal = std::min(frame.getDataSize(), bufferSize);
        memcpy(buffer, frame.getData(), retVal);
    }

	return retVal;
}


FrameRef AnykaEncoderBase::getEncodedFrame()
{
    m_encodedFramesLock.lock();

    const FrameRef retVal = m_encodedFrames.size() > 0
        ? m_encodedFrames.front()
        : kEmptyFrameRef;

    if (m_encodedFrames.size() > 0)
    {
        m_encodedFrames.pop_front();

        if (m_encodedFrames.size() == 0)
        {
            m_signalFd.reset();
        }
    }

    m_encodedFramesLock.unlock();

    return retVal;
}



