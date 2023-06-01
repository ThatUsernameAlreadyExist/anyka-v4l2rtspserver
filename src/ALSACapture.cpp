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
	if (deviceId != AnykaCameraManager::kInvalidStreamId)
	{
		AnykaCameraManager::instance().stopStream(deviceId);
		deviceId = AnykaCameraManager::kInvalidStreamId;
	}
}
	
ALSACapture::ALSACapture(const ALSACaptureParameters & params) : deviceId(AnykaCameraManager::kInvalidStreamId)
{
	deviceId = AnykaCameraManager::instance().startStream(params.m_devName);

	LOG(NOTICE)<<"create ALSACapture device: "<<params.m_devName<<" with id: "<< deviceId;
}
			
int ALSACapture::configureFormat(snd_pcm_hw_params_t *hw_params) 
{
	LOG(NOTICE)<<"configureFormat ALSACapture with id: "<< deviceId;
	return 0;
}

size_t ALSACapture::read(char* buffer, size_t bufferSize)
{
	size_t retVal = 0;

	if (deviceId != AnykaCameraManager::kInvalidStreamId)
	{
		retVal = AnykaCameraManager::instance().getEncodedFrame(deviceId, buffer, bufferSize);
	}

	return retVal;
}
		
int ALSACapture::getFd()
{
	return AnykaCameraManager::instance().getFd(deviceId);
}

unsigned long ALSACapture::getBufferSize()   
{
	return AnykaCameraManager::instance().getBufferSize(deviceId);     
}	


int ALSACapture::getSampleRate()   
{ 
	return AnykaCameraManager::instance().getSampleRate(deviceId); 
}


int ALSACapture::getChannels()   
{
	return AnykaCameraManager::instance().getChannels(deviceId); 
}

int	ALSACapture::getAudioFormat () 
{ 
	return AnykaCameraManager::instance().getFormat(deviceId);
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



