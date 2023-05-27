/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaCameraManager.h
** 
** -------------------------------------------------------------------------*/

#ifndef ANYKA_CAMERA_MANAGER
#define ANYKA_CAMERA_MANAGER

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "AnykaVideoEncoder.h"

extern "C"
{
	#include "ak_global.h"
	#include "ak_thread.h" 
}

#ifndef V4L2_PIX_FMT_VP8
#define V4L2_PIX_FMT_VP8  v4l2_fourcc('V', 'P', '8', '0')
#endif
#ifndef V4L2_PIX_FMT_VP9
#define V4L2_PIX_FMT_VP9  v4l2_fourcc('V', 'P', '9', '0')
#endif
#ifndef V4L2_PIX_FMT_HEVC
#define V4L2_PIX_FMT_HEVC  v4l2_fourcc('H', 'E', 'V', 'C')
#endif

class AnykaCameraManager
{
public:
	const static size_t kInvalidStreamId;

public:
	static AnykaCameraManager& instance();
	~AnykaCameraManager();

	size_t startStream(const std::string &name);
	void stopStream(size_t streamId);

	unsigned int getFormat(size_t streamId);
	unsigned int getWidth(size_t streamId);      
	unsigned int getHeight(size_t streamId);     
	size_t getFd(size_t streamId) const;
    size_t getBufferSize(size_t streamId) const;
    size_t getEncodedFrame(size_t streamId, char* buffer, size_t bufferSize);

private:
	AnykaCameraManager();
	AnykaCameraManager(const AnykaCameraManager&) = delete;
	AnykaCameraManager& operator=(const AnykaCameraManager&) = delete;

private:
	struct AnykaStream
	{
		AnykaStream();
		AnykaEncoderBase *encoder;
		std::atomic_bool isActivated;
	};

	bool initVideoDevice();
	bool setVideoParams();
	bool startVideoCapture();
	bool stopVideoCapture();

	bool initAudioDevice();
	bool setAudioParams();
	bool startAudioCapture();
	bool stopAudioCapture();

	bool start();
	void stop();

	void stopThread();
	bool restartThread();
	
	encode_param getVideoEncodeParams(size_t streamId);
	audio_param getAudioEncodeParams(size_t streamId);
	encode_param getJpegEncodeParams();

	void processThread();
	bool processEncoding();
	bool processJpeg();
	static void* thread(void *arg);


private:
	void *m_videoDevice;
	void *m_audioDevice;
	ak_pthread_t m_threadId;
	std::atomic_bool m_threadStopFlag;
	AnykaStream m_streams[3]; // High, Low, Audio
	AnykaVideoEncoder m_jpegEncoder;

};


#endif

