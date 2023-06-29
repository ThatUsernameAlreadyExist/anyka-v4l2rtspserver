/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaVideoEncoder.h
** 
**
** -------------------------------------------------------------------------*/


#ifndef ANYKA_AUDIO_ENCODER
#define ANYKA_AUDIO_ENCODER


extern "C"
{
	#include "ak_global.h"
    #include "ak_aenc.h"
}

#include "AnykaEncoderBase.h"


class AnykaAudioEncoder: public AnykaEncoderBase
{
public:
    AnykaAudioEncoder();
    ~AnykaAudioEncoder();

protected:
    bool isAudioEncoder() const override;
    void onStart(void *device, const audio_param &audioParams) override;
    void onStop() override;
    bool readNewFrameData(FrameRef *outFrame) override;

private:
	struct list_head m_streamData;
    typedef void* (*MemcpyFunc)(void*, const void*, size_t);
    MemcpyFunc m_memcpy;
};


#endif

