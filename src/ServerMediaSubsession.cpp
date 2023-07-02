/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ServerMediaSubsession.cpp
** 
** -------------------------------------------------------------------------*/

#include <sstream>
#include <linux/videodev2.h>

// project
#include "BaseServerMediaSubsession.h"
#include "MJPEGVideoSource.h"
#include <map>


const std::map<int, u_int8_t> kAacSamplingIndexes
{
	{96000, 0},
	{88200, 1},
	{64000, 2},
	{48000, 3},
  	{44100, 4},
	{32000, 5},
	{24000, 6},
	{22050, 7},
	{16000, 8},
	{12000, 9},
	{11025, 10},
	{8000,  11},
  	{7350,  12},
	{0,     13},
	{0, 	14},
	{0,		15}
};

// ---------------------------------
//   BaseServerMediaSubsession
// ---------------------------------
FramedSource* BaseServerMediaSubsession::createSource(UsageEnvironment& env, FramedSource* videoES, const std::string& format)
{
	FramedSource* source = NULL;
	if (format == "video/MP2T")
	{
		source = MPEG2TransportStreamFramer::createNew(env, videoES); 
	}
	else if (format == "video/H264")
	{
		source = H264VideoStreamDiscreteFramer::createNew(env, videoES);
	}
#if LIVEMEDIA_LIBRARY_VERSION_INT > 1414454400
	else if (format == "video/H265")
	{
		source = H265VideoStreamDiscreteFramer::createNew(env, videoES);
	}
#endif
	else if (format == "video/JPEG")
	{
		source = MJPEGVideoSource::createNew(env, videoES);
	}
	else 
	{
		source = videoES;
	}
	return source;
}

RTPSink*  BaseServerMediaSubsession::createSink(UsageEnvironment& env, Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, const std::string& format, V4L2DeviceSource* source)
{
	RTPSink* videoSink = NULL;
	DeviceInterface* device = source->getDevice();

	if (format == "video/MP2T")
	{
		videoSink = SimpleRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic, 90000, "video", "MP2T", 1, True, False); 
	}
	else if (format == "video/H264")
        {
		videoSink = H264VideoRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic);
	}
	else if (format == "video/VP8")
	{
		videoSink = VP8VideoRTPSink::createNew (env, rtpGroupsock,rtpPayloadTypeIfDynamic); 
	}
#if LIVEMEDIA_LIBRARY_VERSION_INT > 1414454400
	else if (format == "video/VP9")
	{
		videoSink = VP9VideoRTPSink::createNew (env, rtpGroupsock,rtpPayloadTypeIfDynamic); 
	}
	else if (format == "video/H265")
        {
		videoSink = H265VideoRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic);
	}
#endif	
	else if (format == "video/JPEG")
	{
		videoSink = JPEGVideoRTPSink::createNew (env, rtpGroupsock); 
    } 
#if LIVEMEDIA_LIBRARY_VERSION_INT >= 1596931200	
	else if (format =="video/RAW") 
	{ 
		std::string sampling;
		switch (device->getVideoFormat()) {
			case V4L2_PIX_FMT_YUV444: sampling = "YCbCr-4:4:4"; break;
			case V4L2_PIX_FMT_UYVY  : sampling = "YCbCr-4:2:2"; break;
			case V4L2_PIX_FMT_NV12  : sampling = "YCbCr-4:2:0"; break;
			case V4L2_PIX_FMT_RGB24 : sampling = "RGB"        ; break;
			case V4L2_PIX_FMT_RGB32 : sampling = "RGBA"       ; break;
			case V4L2_PIX_FMT_BGR24 : sampling = "BGR"        ; break;
			case V4L2_PIX_FMT_BGR32 : sampling = "BGRA"       ; break;
		}
		videoSink = RawVideoRTPSink::createNew(env, rtpGroupsock, rtpPayloadTypeIfDynamic, device->getWidth(), device->getHeight(), 8, sampling.c_str(),"BT709-2");
    } 
#endif	
 	else if (format.find("audio/MPEG") == 0)
    {
        videoSink = MPEG1or2AudioRTPSink::createNew(env, rtpGroupsock);
    }
	else
	{
		const int sampleRate    = device->getSampleRate();
		const u_int8_t channels = (u_int8_t)device->getChannels();

		if (format.find("audio/L16") == 0)
		{
			videoSink = SimpleRTPSink::createNew(env, rtpGroupsock, rtpPayloadTypeIfDynamic, sampleRate, "audio", "L16", channels, True, False); 
		}
		else if (format.find("audio/PCMU") == 0)
		{
			videoSink = SimpleRTPSink::createNew(env, rtpGroupsock, rtpPayloadTypeIfDynamic, sampleRate, "audio", "PCMU", channels, True, False); 
		}
		else if (format.find("audio/PCMA") == 0)
		{
			videoSink = SimpleRTPSink::createNew(env, rtpGroupsock, rtpPayloadTypeIfDynamic, sampleRate, "audio", "PCMA", channels, True, False); 
		}
		else if (format.find("audio/AAC") == 0)
		{
			// Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
			const auto it = kAacSamplingIndexes.find(sampleRate);
			const u_int8_t samplingFrequencyIndex = it != kAacSamplingIndexes.end()
				? it->second
				: 11;

			char configStr[5] = {0};
			unsigned char audioSpecificConfig[2];
			u_int8_t const audioObjectType = 2; // AAC main + 1

			audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
			audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channels<<3);
			sprintf(configStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

			videoSink = MPEG4GenericRTPSink::createNew(env, rtpGroupsock, rtpPayloadTypeIfDynamic, sampleRate, "audio", "AAC-hbr", configStr, channels); 
		}
	}

	return videoSink;
}

char const* BaseServerMediaSubsession::getAuxLine(V4L2DeviceSource* source, RTPSink* rtpSink)
{
	const char* auxLine = NULL;
	if (rtpSink) {
		std::ostringstream os; 
		if (rtpSink->auxSDPLine()) {
			os << rtpSink->auxSDPLine();
		}
		else if (source) {
			unsigned char rtpPayloadType = rtpSink->rtpPayloadType();
			DeviceInterface* device = source->getDevice();
			os << "a=fmtp:" << int(rtpPayloadType) << " " << source->getAuxLine() << "\r\n";				
			int width = device->getWidth();
			int height = device->getHeight();
			if ( (width > 0) && (height>0) ) {
				os << "a=x-dimensions:" << width << "," <<  height  << "\r\n";				
			}
		} 
		auxLine = strdup(os.str().c_str());
	}
	return auxLine;
}

