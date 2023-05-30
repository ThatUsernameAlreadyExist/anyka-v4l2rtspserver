/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaCameraManager.cpp
** 
** -------------------------------------------------------------------------*/


#include "AnykaCameraManager.h"
#include "logger.h"
#include <linux/videodev2.h>
#include "SharedMemory.h"
#include "AnykaAudioEncoder.h"
#include <algorithm>
#include <map>

extern "C"
{
	#include "ak_vi.h"
	#include "ak_ai.h"
	#include "ak_common.h"
}

const size_t AnykaCameraManager::kInvalidStreamId = (size_t)-1;



std::map<std::string, StreamId> kStreamNames
{
	{"video0", VideoHigh},
	{"video1", VideoLow},
	{"audio0", AudioHigh},
	{"audio1", AudioLow},
};

AnykaCameraManager::AnykaStream::AnykaStream()
	: encoder(NULL)
	, isActivated(false)
{
}


AnykaCameraManager::AnykaCameraManager()
	: m_videoDevice(NULL)
	, m_threadId(0)
	, m_threadStopFlag(false)
{
	LOG(DEBUG)<<"AnykaCameraManager construct";

	ak_print_set_level(LOG_LEVEL_NOTICE);

	m_streams[VideoHigh].encoder = new AnykaVideoEncoder();
	m_streams[VideoLow].encoder  = new AnykaVideoEncoder();
	m_streams[AudioHigh].encoder = new AnykaAudioEncoder();
	m_streams[AudioLow].encoder  = new AnykaAudioEncoder();

	initVideoDevice();
	initAudioDevice();
}


AnykaCameraManager::~AnykaCameraManager()
{
	stopThread();

	for (size_t i = 0; i < STREAMS_COUNT; ++i)
	{
		delete m_streams[i].encoder;
	}

	ak_ai_close(m_audioDevice);
	ak_vi_close(m_videoDevice);

	LOG(DEBUG)<<"AnykaCameraManager destruct"<<std::flush;
}


AnykaCameraManager& AnykaCameraManager::instance()
{
	static AnykaCameraManager _instance;
	return _instance;
}


size_t AnykaCameraManager::startStream(const std::string &name)
{
	size_t streamId = kInvalidStreamId;

	if (m_videoDevice != NULL && name.size() > 0)
	{
		const auto it = kStreamNames.find(name);

		if (it != kStreamNames.end())
		{
			const auto id = it->second;
			if (!m_streams[id].isActivated)
			{
				m_streams[id].isActivated = true;

				if (restartThread())
				{
					streamId = id;

					LOG(NOTICE)<<"success start stream with id: "<<streamId<<" for device: "<<name;
				}	
			}
			else
			{
				LOG(WARN)<<"already active device: "<<name;
			}
		}
		else
		{
			LOG(ERROR)<<"invalid device: "<<name;
		}
	}
	else
	{
		LOG(ERROR)<<"invalid params";
	}

	return streamId;
}


void AnykaCameraManager::stopStream(size_t streamId)
{
	if (m_streams[streamId].isActivated)
	{
		m_streams[streamId].isActivated = false;

		restartThread();

		LOG(DEBUG)<<"success stop stream with id: "<<streamId;
	}
}


int AnykaCameraManager::getFd(size_t streamId) const
{
	return streamId < STREAMS_COUNT
		? m_streams[streamId].encoder->getEncodedFrameReadyFd()
		: -1;
}


unsigned int AnykaCameraManager::getFormat(size_t streamId)
{
	if (streamId == VideoHigh) return V4L2_PIX_FMT_HEVC;
	else if (streamId == VideoLow) return V4L2_PIX_FMT_H264;
	else if (streamId == AudioHigh || streamId == AudioLow) return 1777; // TODO
	
	return -1;
}


unsigned int AnykaCameraManager::getWidth(size_t streamId)
{
	if (streamId == VideoHigh) return 1080;
	else if (streamId == VideoLow) return 640;
	
	return 0;
}


unsigned int AnykaCameraManager::getHeight(size_t streamId)
{
	if (streamId == VideoHigh) return 1080;
	else if (streamId == VideoLow) return 360;
	
	return 0;
}


size_t AnykaCameraManager::getBufferSize(size_t streamId) const
{
	return streamId < STREAMS_COUNT
		? m_streams[streamId].encoder->getEncodedFrameSize()
		: 0;
}


size_t AnykaCameraManager::getEncodedFrame(size_t streamId, char* buffer, size_t bufferSize)
{
	return streamId < STREAMS_COUNT
		? m_streams[streamId].encoder->getEncodedFrame(buffer, bufferSize)
		: 0;
}


bool AnykaCameraManager::initVideoDevice()
{
	if (ak_vi_match_sensor("/etc/jffs2/isp_f23_mipi2lane.conf") == AK_SUCCESS)
	{
		LOG(NOTICE)<<"ak_vi_match_sensor success";
		m_videoDevice = ak_vi_open(VIDEO_DEV0);
	}
	else
	{
		LOG(ERROR)<<"ak_vi_match_sensor failed";
	}
}


bool AnykaCameraManager::setVideoParams()
{
	struct video_channel_attr attr;
	memset(&attr, 0, sizeof(struct video_channel_attr));

	// TODO: read from config file

	attr.res[VIDEO_CHN_MAIN].width  = 1920;
	attr.res[VIDEO_CHN_MAIN].height = 1080;
	attr.res[VIDEO_CHN_SUB].width   = 640;
	attr.res[VIDEO_CHN_SUB].height  = 360;

	attr.res[VIDEO_CHN_MAIN].max_width  = 1920;
	attr.res[VIDEO_CHN_MAIN].max_height = 1080;
	attr.res[VIDEO_CHN_SUB].max_width   = 640;
	attr.res[VIDEO_CHN_SUB].max_height  = 480;
	
	attr.crop.left = 0;
	attr.crop.top = 0;
	attr.crop.width = 1920;
	attr.crop.height = 1080;

	bool retVal = false;

	if (ak_vi_set_channel_attr(m_videoDevice, &attr) == AK_SUCCESS && ak_vi_set_fps(m_videoDevice, 25) == AK_SUCCESS) 
	{
		LOG(NOTICE)<<"ak_vi_set_channel_attr && ak_vi_set_fps success";
		
		retVal = true;
	}
	else
	{
		LOG(ERROR)<<"ak_vi_set_channel_attr failed";
	}

	return retVal;
}


bool AnykaCameraManager::startVideoCapture()
{
	return ak_vi_capture_on(m_videoDevice) == AK_SUCCESS;
}


bool AnykaCameraManager::stopVideoCapture()
{
	return ak_vi_capture_off(m_videoDevice) == AK_SUCCESS;
}


bool AnykaCameraManager::initAudioDevice()
{
	struct pcm_param ai_param = {0};
	ai_param.sample_bits = 16; // Only 16 supported.
	ai_param.channel_num = 1;
	ai_param.sample_rate = 8000;

    m_audioDevice = ak_ai_open(&ai_param);
    if (m_audioDevice != NULL)
    {
		LOG(NOTICE)<<"ak_ai_open success";
		if (ak_ai_set_source(m_audioDevice, AI_SOURCE_MIC) == AK_SUCCESS)
		{
			LOG(NOTICE)<<"ak_ai_set_source success";
		}
		else
		{
			LOG(ERROR)<<"ak_ai_set_source faied";
			ak_ai_close(m_audioDevice);
			m_audioDevice = NULL;
		}
	}
	else
	{
		LOG(ERROR)<<"ak_ai_open faied";
	}

	return m_audioDevice != NULL;
}


bool AnykaCameraManager::setAudioParams()
{
	ak_ai_set_aec(m_audioDevice, AUDIO_FUNC_DISABLE);
	ak_ai_set_nr_agc(m_audioDevice, AUDIO_FUNC_DISABLE);
	ak_ai_set_resample(m_audioDevice, AUDIO_FUNC_DISABLE);
	ak_ai_set_volume(m_audioDevice, 10); // 0 - 12, 0 - mute
	ak_ai_clear_frame_buffer(m_audioDevice);

	int interval = -1;
	int sampleRate = 8000;
	int type = AK_AUDIO_TYPE_PCM; // TODO: from config
	switch (type) 
    {
        case AK_AUDIO_TYPE_AAC:
            interval = ((1024 *1000) / sampleRate); /* 1k data in 1 second */
            break;
        case AK_AUDIO_TYPE_AMR:
            interval = AMR_FRAME_INTERVAL;
            break;
        case AK_AUDIO_TYPE_PCM_ALAW:	/* G711, alaw */
        case AK_AUDIO_TYPE_PCM_ULAW:	/* G711, ulaw */
            interval = AUDIO_DEFAULT_INTERVAL;
            break;
        case AK_AUDIO_TYPE_PCM:
            interval = AUDIO_DEFAULT_INTERVAL;
            break;
        case AK_AUDIO_TYPE_MP3:
            if (sampleRate >= 8000 && sampleRate <= 24000) 
            {
                interval = 576*1000/sampleRate;
            } 
            else 
            { // sample_rate =32000 or 44100 or 48000
                interval = 1152*1000/sampleRate;
            }
            break;
        default:	
            interval = AUDIO_DEFAULT_INTERVAL;
            break;
	}

	if (ak_ai_set_frame_interval(m_audioDevice, interval) != AK_SUCCESS)
	{
		LOG(WARN)<<"ak_ai_set_frame_interval faied";
	}

	return true;
}


bool AnykaCameraManager::startAudioCapture()
{
	return ak_ai_start_capture(m_audioDevice) == AK_SUCCESS;
}


bool AnykaCameraManager::stopAudioCapture()
{
	return ak_ai_stop_capture(m_audioDevice) == AK_SUCCESS;
}


bool AnykaCameraManager::start()
{
	bool retVal = true;

	if (std::any_of(std::begin(m_streams), std::end(m_streams), 
			[](const AnykaStream &strm) { return strm.isActivated.load(); }))
	{
		if (setVideoParams() && startVideoCapture() && setAudioParams() && startAudioCapture())
		{
			for (size_t i = 0; i < STREAMS_COUNT; ++i)
			{
				if (m_streams[i].isActivated)
				{
					retVal = m_streams[i].encoder->start(m_videoDevice, m_audioDevice, getVideoEncodeParams(i), getAudioEncodeParams(i)) || retVal;
				}
			}
		}

		if (retVal)
		{
			if (!m_jpegEncoder.start(m_videoDevice, NULL, getJpegEncodeParams(), getAudioEncodeParams(0)))
			{
				LOG(ERROR)<<"can't init jpeg stream";
			}
		}
		else
		{
			LOG(ERROR)<<"can't start streams";
			stop();
		}
	}

	return retVal;
}


void AnykaCameraManager::stop()
{
	for (size_t i = 0; i < STREAMS_COUNT; ++i)
	{
		if (m_streams[i].isActivated)
		{
			m_streams[i].encoder->stop();
		}
	}

	m_jpegEncoder.stop();

	stopVideoCapture();
	stopAudioCapture();
}
	

void AnykaCameraManager::stopThread()
{
	if (m_threadId != 0)
	{
		m_threadStopFlag = true;
		ak_thread_join(m_threadId);
		m_threadId = 0;
		m_threadStopFlag = false;
	}
}


bool AnykaCameraManager::restartThread()
{
	stopThread();

	return m_videoDevice != NULL && ak_thread_create(&m_threadId, AnykaCameraManager::thread, NULL, 100*1024, 90) == AK_SUCCESS;
}


encode_param AnykaCameraManager::getVideoEncodeParams(size_t streamId)
{
	encode_param param = {0};

	if (streamId == VideoHigh)
	{
		param.width   		= 1920;
		param.height  		= 1080;
		param.minqp   		= 20;
		param.maxqp   		= 51;
		param.fps     		= 25;
		param.goplen  		= param.fps * 2;
		param.bps     		= 1500;
		param.profile 		= PROFILE_MAIN;
		param.use_chn 		= ENCODE_MAIN_CHN;
		param.enc_grp 		= ENCODE_MAINCHN_NET;
		param.br_mode 		= BR_MODE_CBR;
		param.enc_out_type  = HEVC_ENC_TYPE;
	}
	else if (streamId == VideoLow)
	{
		param.width   		= 640;
		param.height  		= 360;
		param.minqp   		= 20;
		param.maxqp   		= 51;
		param.fps     		= 25;
		param.goplen  		= param.fps * 2;
		param.bps     		= 500;
		param.profile 		= PROFILE_MAIN;
		param.use_chn 		= ENCODE_SUB_CHN;
		param.enc_grp 		= ENCODE_SUBCHN_NET;
		param.br_mode 		= BR_MODE_CBR;
		param.enc_out_type 	= H264_ENC_TYPE;
	}

	return param;
}


audio_param AnykaCameraManager::getAudioEncodeParams(size_t streamId)
{
	audio_param retVal = {AK_AUDIO_TYPE_UNKNOWN, 0};

	if (streamId == AudioHigh)
	{
		retVal.channel_num = 1;
		retVal.sample_bits = 16;
		retVal.sample_rate = 8000;
		retVal.type = AK_AUDIO_TYPE_PCM;
	}
	else if (streamId == AudioLow)
	{
		retVal.channel_num = 1;
		retVal.sample_bits = 16;
		retVal.sample_rate = 8000;
		retVal.type = AK_AUDIO_TYPE_PCM;
	}

	return retVal;
}


encode_param AnykaCameraManager::getJpegEncodeParams()
{
	encode_param param 	= getVideoEncodeParams(VideoLow); // Use jpeg from low stream.

	param.fps 			= 1;
	param.enc_out_type 	= MJPEG_ENC_TYPE;
	param.enc_grp 		= ENCODE_PICTURE;

	return param;
}


void AnykaCameraManager::processThread()
{
	if (start())
	{
		while (!m_threadStopFlag)
		{
			const bool isStreamsEncoded = processEncoding();
			const bool isJpegEncoded    = processJpeg();

			if (isStreamsEncoded || isJpegEncoded)
			{
				ak_sleep_ms(1);
			}
			else
			{
				ak_sleep_ms(10);
			}
		}

		stop();
	}
}


bool AnykaCameraManager::processEncoding()
{
	bool retVal = false;

	for (size_t i = 0; i < STREAMS_COUNT; ++i)
	{
		retVal = m_streams[i].encoder->encode() || retVal;
	}

	return retVal;
}


bool AnykaCameraManager::processJpeg()
{
	bool retVal = m_jpegEncoder.encode();

	if (retVal)
	{
		const size_t imageSize = m_jpegEncoder.getEncodedFrameSize();

		void *outPtr = SharedMemory::instance().lockImage(imageSize);

		if (outPtr != NULL)
		{
			m_jpegEncoder.getEncodedFrame((char*)outPtr, imageSize);

			SharedMemory::instance().unlockImage(outPtr);
		}
	}

	return retVal;
}


void* AnykaCameraManager::thread(void*)
{
	AnykaCameraManager& camMan = AnykaCameraManager::instance();

	LOG(DEBUG)<<"anyka  thread started";

	camMan.processThread();

	LOG(DEBUG)<<"anyka thread stopped";

	ak_thread_exit();

	return NULL;
}