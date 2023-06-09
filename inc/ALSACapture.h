/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ALSACapture.h
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** ALSA capture overide of V4l2Capture
**                                                                                    
** -------------------------------------------------------------------------*/

#pragma once

#include <list>

#include <alsa/asoundlib.h>
#include "logger.h"

#include "DeviceInterface.h"
#include "AnykaCameraManager.h"

struct ALSACaptureParameters 
{
	ALSACaptureParameters(const char* devname, const std::list<snd_pcm_format_t> & formatList, unsigned int sampleRate, unsigned int channels, int verbose) : 
		m_devName(devname), m_formatList(formatList), m_sampleRate(sampleRate), m_channels(channels), m_verbose(verbose) {
			
	}
		
	std::string      m_devName;
	std::list<snd_pcm_format_t> m_formatList;		
	unsigned int     m_sampleRate;
	unsigned int     m_channels;
	int              m_verbose;
};

class ALSACapture  : public DeviceInterface
{
	public:
		static ALSACapture* createNew(const ALSACaptureParameters & params) ;
		virtual ~ALSACapture();
		void close();
	
	protected:
		ALSACapture(const ALSACaptureParameters & params);
		int configureFormat(snd_pcm_hw_params_t *hw_params);
			
	public:
		virtual FrameRef read();
		virtual int getFd();
		virtual unsigned long getBufferSize();
		
		virtual int getSampleRate();
		virtual int getChannels  ();
		virtual int getAudioFormat ();
		virtual std::list<int> getAudioFormatList() { return std::list<int>(); }

	private:
		size_t deviceId;
};



