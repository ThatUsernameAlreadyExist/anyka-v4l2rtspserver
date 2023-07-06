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

#include "FrameBuffer.h"
#include "V4l2DummyFd.h"
#include <atomic>
#include <list>
#include <mutex>

struct VideoEncodeParam
{
    encode_param videoParams;
    venc_smart_cfg smartParams;
    int maxKbps;
    int targetKbps;
};


class AnykaEncoderBase
{
public:
    AnykaEncoderBase();
    virtual ~AnykaEncoderBase() = default;

    bool isSet() const;

    bool start(void *videoDevice, void *audioDevice, const VideoEncodeParam &videoParams, const audio_param &audioParams);
    void stop();
    bool encode();

    int getEncodedFrameReadyFd() const;
    FrameRef getEncodedFrame();

protected:
    virtual bool isAudioEncoder() const = 0;
    virtual void onStart(void *device, const VideoEncodeParam &videoParams);
    virtual void onStart(void *device, const audio_param &audioParams);
    virtual void onStop() = 0;
    virtual bool readNewFrameData(FrameRef *outFrame) = 0;

protected:
    void *m_encoder;
    void *m_encoderStream;

private:
    V4l2DummyFd m_signalFd;
    FrameBuffer m_frameBuffer;
    FrameRef m_freeFrame;
    mutable std::mutex m_encodedFramesLock;
    std::list<FrameRef> m_encodedFrames;
};


#endif

