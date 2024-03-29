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
#include "AnykaOsd.h"
#include "AnykaVideoEncoder.h"
#include "AnykaMotionDetector.h"
#include "AnykaDayNight.h"
#include "ConfigFile.h"
#include "FrameBuffer.h"
#include "SharedMemory.h"


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

#define SND_PCM_FORMAT_AAC 1999

enum StreamId
{
	VideoHigh = 0,
	VideoLow,
	AudioHigh,
	AudioLow,
	STREAMS_COUNT
};

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
	int getSampleRate(size_t streamId);
	int getChannels(size_t streamId);   
	int getFd(size_t streamId) const;
    size_t getBufferSize(size_t streamId) const;
	FrameRef getEncodedFrame(size_t streamId);

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
	
	VideoEncodeParam getVideoEncodeParams(size_t streamId);
	audio_param getAudioEncodeParams(size_t streamId);
	VideoEncodeParam getJpegEncodeParams();

	void processThread();
	bool processEncoding();
	bool processJpeg();
	void processSharedConfig();
	void processMotionDetection();
	void writeMotionDetectionFlag(bool isMotionDetected);
	static void* thread(void *arg);

	void initFromConfig(const SharedConfig *sharedConf);
	void startOsd(const SharedConfig *sharedConf);
	void startMotionDetection(const SharedConfig *sharedConf);
	bool startDayNight(const SharedConfig *sharedConf);
	void flipImage(const SharedConfig *sharedConf);
	void updateCurrentSharedConfig(const SharedConfig *sharedConf);
	void abortIfNeed();

private:
	void *m_videoDevice;
	void *m_audioDevice;
	ak_pthread_t m_threadId;
	std::atomic_bool m_threadStopFlag;
	AnykaStream m_streams[STREAMS_COUNT];
	ReadOnlyConfigSection m_config[STREAMS_COUNT];
	ReadOnlyConfigSection m_mainConfig;
	AnykaVideoEncoder m_jpegEncoder;
	AnykaOsd m_osd;
	AnykaMotionDetector m_motionDetect;
	AnykaDayNight m_dayNight;
	SharedConfig m_currentSharedConfig;
	int m_sharedConfUpdateCounter;
	int m_maxSharedConfUpdateCounter;
	bool m_lastMotionDetected;
	int m_motionCounter;
	int m_maxMotionCounter;
	int m_motionDetectionFd;
	flock m_motionDetectionLock;
	bool m_abortOnError;
	bool m_preferSharedConfig;

};


#endif

