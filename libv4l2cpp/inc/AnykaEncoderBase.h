/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaEncoderBase.h
** 
**
** -------------------------------------------------------------------------*/


#ifndef ANYKA_ENCODER_BASE
#define ANYKA_ENCODER_BASE


extern "C"
{
	#include "ak_global.h"
    #include "ak_venc.h"
}

#include "V4l2DummyFd.h"
#include <atomic>
#include <mutex>


class AnykaEncoderBase
{
public:
    AnykaEncoderBase();
    virtual ~AnykaEncoderBase() = default;

    bool isSet() const;

    bool start(void *videoDevice, void *audioDevice, const encode_param &videoParams, const audio_param &audioParams);
    void stop();
    bool encode();

    size_t getEncodedFrameReadyFd() const;
    size_t getEncodedFrameSize() const;
    size_t getEncodedFrame(char* buffer, size_t bufferSize);

protected:
    virtual bool isAudioEncoder() const = 0;
    virtual void onStart(void *device, const encode_param &videoParams);
    virtual void onStart(void *device, const audio_param &audioParams);
    virtual void onStop() = 0;
    virtual size_t readNewFrameData() = 0;
    virtual size_t copyNewFrameDataTo(char* buffer, size_t bufferSize) = 0;
    virtual void releaseFrameData() = 0;

protected:
    void *m_encoder;
    void *m_encoderStream;

private:
    std::atomic<size_t> m_bufferSize;
    std::atomic_bool m_isNewFrameAvailable;
    std::mutex m_streamDataLock;
    V4l2DummyFd m_signalFd;
};


#endif

