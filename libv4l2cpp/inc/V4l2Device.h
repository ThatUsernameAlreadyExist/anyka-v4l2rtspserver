/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2Device.h
** 
** V4L2 wrapper 
**
** -------------------------------------------------------------------------*/


#ifndef V4L2_DEVICE
#define V4L2_DEVICE

#include <string>
#include <list>
#include <linux/videodev2.h>
#include <fcntl.h>
#include "AnykaCameraManager.h"


enum V4l2IoType
{
	IOTYPE_READWRITE,
	IOTYPE_MMAP
};

// ---------------------------------
// V4L2 Device parameters
// ---------------------------------
struct V4L2DeviceParameters 
{
	V4L2DeviceParameters(const char* devname, const std::list<unsigned int> & formatList, unsigned int width, unsigned int height, int fps, V4l2IoType ioType = IOTYPE_MMAP, int verbose = 0, int openFlags = O_RDWR | O_NONBLOCK) : 
		m_devName(devname), m_formatList(formatList), m_width(width), m_height(height), m_fps(fps), m_iotype(ioType), m_verbose(verbose), m_openFlags(openFlags) {}

	V4L2DeviceParameters(const char* devname, unsigned int format, unsigned int width, unsigned int height, int fps, V4l2IoType ioType = IOTYPE_MMAP, int verbose = 0, int openFlags = O_RDWR | O_NONBLOCK) : 
		m_devName(devname), m_width(width), m_height(height), m_fps(fps), m_iotype(ioType), m_verbose(verbose), m_openFlags(openFlags) {
			if (format) {
				m_formatList.push_back(format);
			}
	}
		
	std::string m_devName;
	std::list<unsigned int> m_formatList;
	unsigned int m_width;
	unsigned int m_height;
	int m_fps;			
	V4l2IoType m_iotype;
	int m_verbose;
	int m_openFlags;
};

// ---------------------------------
// V4L2 Device
// ---------------------------------
class V4l2Device
{		
	friend class V4l2Capture;
	friend class V4l2Output;
	
	public:
		V4l2Device(const V4L2DeviceParameters&  params);		
		virtual ~V4l2Device();
		bool init();
		virtual bool isReady();
		virtual bool start();
		virtual bool stop();
	
		unsigned int getBufferSize();
		unsigned int getFormat();
		unsigned int getWidth();
		unsigned int getHeight();
		int          getFd();
		void         queryFormat();
			
		int setFormat(unsigned int format, unsigned int width, unsigned int height);
		int setFps(int fps);

		static std::string fourcc(unsigned int format);
		static unsigned int fourcc(const char* format);

		size_t readInternal(char* buffer, size_t bufferSize);
		FrameRef readInternal();
	
	private:
		V4L2DeviceParameters m_params;
		size_t streamId;
};


#endif
