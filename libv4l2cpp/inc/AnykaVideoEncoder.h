/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaVideoEncoder.h
** 
**
** -------------------------------------------------------------------------*/


#ifndef ANYKA_VIDEO_ENCODER
#define ANYKA_VIDEO_ENCODER


extern "C"
{
	#include "ak_global.h"
    #include "ak_venc.h"
}

#include "AnykaEncoderBase.h"


class AnykaVideoEncoder: public AnykaEncoderBase
{
public:
    AnykaVideoEncoder();
    ~AnykaVideoEncoder();

protected:
    bool isAudioEncoder() const override;
    void onStart(void *device, const VideoEncodeParam &videoParams) override;
    void onStop() override;
    bool readNewFrameData(FrameRef *outFrame) override;

private:
    video_stream m_streamData;
};


#endif

