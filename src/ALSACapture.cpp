/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ALSACapture.cpp
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** ALSA capture overide of V4l2Capture
**                                                                                    
** -------------------------------------------------------------------------*/

#include "ALSACapture.h"
#include "AnykaCameraManager.h"
#include "logger.h"


ALSACapture* ALSACapture::createNew(const ALSACaptureParameters & params) 
{ 
	ALSACapture* capture = new ALSACapture(params);
	if (capture) 
	{
		if (capture->getFd() == -1) 
		{
			delete capture;
			capture = NULL;
		}
	}
	return capture; 
}

ALSACapture::~ALSACapture()
{
	this->close();
}

void ALSACapture::close()
{
	if ((size_t)m_pcm != AnykaCameraManager::kInvalidStreamId)
	{
		AnykaCameraManager::instance().stopStream((size_t)m_pcm);
		m_pcm = (snd_pcm_t*)AnykaCameraManager::kInvalidStreamId;
	}
}
	
ALSACapture::ALSACapture(const ALSACaptureParameters & params) : m_pcm((snd_pcm_t*)AnykaCameraManager::kInvalidStreamId), m_bufferSize(0), m_periodSize(0), m_params(params)
{
	m_pcm = (snd_pcm_t*)AnykaCameraManager::instance().startStream("audio");

	LOG(NOTICE)<<"create ALSACapture with id: "<< (size_t)m_pcm;
}
			
int ALSACapture::configureFormat(snd_pcm_hw_params_t *hw_params) 
{
	LOG(NOTICE)<<"configureFormat ALSACapture with id: "<< (size_t)m_pcm;
	return 0;
}

size_t ALSACapture::read(char* buffer, size_t bufferSize)
{
	size_t retVal = 0;

	if ((size_t)m_pcm != AnykaCameraManager::kInvalidStreamId)
	{
		retVal = AnykaCameraManager::instance().getEncodedFrame((size_t)m_pcm, buffer, bufferSize);
	}

	return retVal;
}
		
int ALSACapture::getFd()
{
	return AnykaCameraManager::instance().getFd((size_t)m_pcm);
}

unsigned long ALSACapture::getBufferSize()   
{
	return AnykaCameraManager::instance().getBufferSize((size_t)m_pcm);     
}	


int ALSACapture::getSampleRate()   
{ 
	return 8000; 
}


int ALSACapture::getChannels()   
{
	 return 1;   
}


int	ALSACapture::getAudioFormat () 
{ 
	return AnykaCameraManager::instance().getFormat((size_t)m_pcm);
}
		
// Fake ALSA interface implementation.
int snd_card_next(int *card)
{
	LOG(DEBUG)<<"snd_card_next";

	if (*card == -1)
	{
		*card = 0;
		return 0;
	}

	*card = -1;

	return -1;
}

int snd_device_name_hint(int card, const char *iface, void ***hints)
{
	LOG(DEBUG)<<"snd_device_name_hint"<<std::flush;

	if (card == 0)
	{
		static char *list[2] = {NULL, NULL};
		*hints = (void**)&list;
		return 0;
	}

	return -1;
}

char *snd_device_name_get_hint(const void *hint, const char *id)
{
	LOG(DEBUG)<<"snd_device_name_get_hint";

	return NULL;
}

int snd_device_name_free_hint(void **hints)
{
	LOG(DEBUG)<<"snd_device_name_free_hint";

	return 0;
}



