/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2Device.cpp
** 
** -------------------------------------------------------------------------*/


#include "V4l2Device.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "logger.h"


std::string V4l2Device::fourcc(unsigned int format) {
	char formatArray[] = { (char)(format&0xff), (char)((format>>8)&0xff), (char)((format>>16)&0xff), (char)((format>>24)&0xff), 0 };
	return std::string(formatArray, strlen(formatArray));
}

unsigned int V4l2Device::fourcc(const char* format) {
	char fourcc[4];
	memset(&fourcc, 0, sizeof(fourcc));
	if (format != NULL)
	{
		strncpy(fourcc, format, 4);	
	}
	return v4l2_fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);	
}

// -----------------------------------------
//    V4L2Device
// -----------------------------------------
V4l2Device::V4l2Device(const V4L2DeviceParameters& params) 
	: m_params(params)
	, streamId(AnykaCameraManager::kInvalidStreamId)
{
}

V4l2Device::~V4l2Device() 
{
		LOG(ERROR)<<"DESTR V4l2Device MAN 1"<<std::flush;
	stop();
			LOG(ERROR)<<"DESTR V4l2Device MAN 2"<<std::flush;
}


bool V4l2Device::init()
{
	streamId = AnykaCameraManager::instance().startStream(m_params.m_devName);
	LOG(DEBUG)<<"init stream id: " <<streamId<<"\n";

	return streamId != AnykaCameraManager::kInvalidStreamId;
}


size_t V4l2Device::readInternal(char* buffer, size_t bufferSize)  
{
	return AnykaCameraManager::instance().getEncodedFrame(streamId, buffer, bufferSize);
}


FrameRef V4l2Device::readInternal()  
{
	return AnykaCameraManager::instance().getEncodedFrame(streamId);
}


int V4l2Device::setFormat(unsigned int format, unsigned int width, unsigned int height)
{
	LOG(DEBUG)<<"V4l2Device setFormat";
	return 0;
}


int V4l2Device::setFps(int fps)
{
	LOG(DEBUG)<<"V4l2Device setFps";
	return 0;
}


bool V4l2Device::isReady() 
{ 
	return streamId != AnykaCameraManager::kInvalidStreamId; 
}


bool V4l2Device::start()   
{
	LOG(DEBUG)<<"start stream id: " <<streamId<<"\n";
	return streamId != AnykaCameraManager::kInvalidStreamId;
}


bool V4l2Device::stop()    
{
	if (streamId != AnykaCameraManager::kInvalidStreamId)
	{
		AnykaCameraManager::instance().stopStream(streamId);
		streamId = AnykaCameraManager::kInvalidStreamId;
	}
}


unsigned int V4l2Device::getBufferSize()
{ 
	return AnykaCameraManager::instance().getBufferSize(streamId);
}


unsigned int V4l2Device::getFormat()     
{
	LOG(DEBUG)<<"V4l2Device::getFormat";
	return AnykaCameraManager::instance().getFormat(streamId);     
}


unsigned int V4l2Device::getWidth()      
{
	LOG(DEBUG)<<"V4l2Device::getWidth";
	return AnykaCameraManager::instance().getWidth(streamId);  
}


unsigned int V4l2Device::getHeight()     
{
	LOG(DEBUG)<<"V4l2Device::getHeight";
	return AnykaCameraManager::instance().getHeight(streamId);    
}


int V4l2Device::getFd()         
{
	LOG(DEBUG)<<"V4l2Device::getFd " <<streamId<<"\n";
	return AnykaCameraManager::instance().getFd(streamId); 
}


void V4l2Device::queryFormat()   
{
	LOG(DEBUG)<<"V4l2Device queryFormat";
};