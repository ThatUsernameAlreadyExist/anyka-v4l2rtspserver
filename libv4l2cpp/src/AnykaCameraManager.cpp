/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AnykaCameraManager.cpp
** 
** -------------------------------------------------------------------------*/


#include "AnykaCameraManager.h"
#include <alsa/asoundlib.h>
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
const std::string kConfigSensor      = "sensor";
const std::string kConfigWidth       = "width";
const std::string kConfigHeight      = "height";
const std::string kConfigSampleRate  = "samplerate";
const std::string kConfigChannels    = "channels";
const std::string kConfigVolume      = "volume";
const std::string kConfigCodec	     = "codec";
const std::string kConfigMinQp	     = "minqp";
const std::string kConfigMaxQp	     = "maxqp";
const std::string kConfigFps	     = "fps";
const std::string kConfigGopLen	     = "goplen";
const std::string kConfigBps	     = "bps";
const std::string kConfigProfile     = "profile";
const std::string kConfigBrMode      = "brmode";
const std::string kConfigJpgFps      = "jpegfps";

const std::map<int, int> kAkCodecToFormatMap
{
	{H264_ENC_TYPE, 			V4L2_PIX_FMT_H264},
	{HEVC_ENC_TYPE, 			V4L2_PIX_FMT_HEVC},
	{AK_AUDIO_TYPE_AAC, 		SND_PCM_FORMAT_AAC},
	{AK_AUDIO_TYPE_PCM, 		SND_PCM_FORMAT_S16_LE},
	{AK_AUDIO_TYPE_PCM_ALAW, 	SND_PCM_FORMAT_A_LAW},
	{AK_AUDIO_TYPE_PCM_ULAW, 	SND_PCM_FORMAT_MU_LAW}
};

const size_t kMainAudio = AudioHigh;
const size_t kMainVideo = VideoHigh;

const std::map<std::string, std::string> kDefaultConfig[STREAMS_COUNT]
{
	// VideoHigh
	{
		{kConfigSensor    , "/etc/jffs2/isp_f23_mipi2lane.conf"},
		{kConfigWidth     , "1920"},
		{kConfigHeight    , "1080"},
		{kConfigCodec	  , std::to_string(HEVC_ENC_TYPE)}, // 2
		{kConfigMinQp	  , "20"}, 
		{kConfigMaxQp	  , "51"}, 
		{kConfigFps	   	  , "25"},
		{kConfigGopLen	  , "50"}, 
		{kConfigBps	   	  , "1500"},
		{kConfigProfile   , std::to_string(PROFILE_MAIN)}, // 0
		{kConfigBrMode    , std::to_string(BR_MODE_CBR)}, // 0
		{kConfigJpgFps    , "1"},
	},

	// VideoLow
	{
		{kConfigWidth     , "640"},
		{kConfigHeight    , "360"},
		{kConfigCodec	  , std::to_string(H264_ENC_TYPE)}, // 0
		{kConfigMinQp	  , "20"}, 
		{kConfigMaxQp	  , "51"}, 
		{kConfigFps	   	  , "25"},
		{kConfigGopLen	  , "50"}, 
		{kConfigBps	   	  , "500"},
		{kConfigProfile   , std::to_string(PROFILE_MAIN)}, // 0
		{kConfigBrMode    , std::to_string(BR_MODE_CBR)}, // 0
	},

	// AudioHigh
	{
		{kConfigSampleRate, "8000"},
		{kConfigChannels  , "1"},
		{kConfigVolume    , "10"},
		{kConfigCodec	  , std::to_string(AK_AUDIO_TYPE_AAC)}, // 4
	},

	// AudioLow
	{
		{kConfigSampleRate, "8000"},
		{kConfigChannels  , "1"},
		{kConfigCodec	  , std::to_string(AK_AUDIO_TYPE_PCM_ALAW)}, // 17
	},
};


std::map<std::string, StreamId> kStreamNames
{
	{"video0", VideoHigh},
	{"video1", VideoLow},
	{"audio0", AudioHigh},
	{"audio1", AudioLow},
};


static void updateDefaultConfig(const std::shared_ptr<ConfigFile> &config)
{
	for (size_t i = 0; i < STREAMS_COUNT; ++i)
	{
		for (const auto &it: kDefaultConfig[i])
		{
			if (!config->hasValue(i, it.first))
			{
				LOG(NOTICE)<<"use default config: "<<i<<") "<<it.first<<"="<<it.second;
				config->setValue(i, it.first, it.second);
			}
		}
	}
}


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

	std::shared_ptr<ConfigFile> config = std::make_shared<ConfigFile>(); // TODO: file path

	updateDefaultConfig(config);

	for (size_t i = 0; i < STREAMS_COUNT; ++i)
	{
		m_streams[i].encoder = new AnykaVideoEncoder();
		m_config[i].init(config, i);
	}

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
	unsigned int retVal = -1;

	if (streamId < STREAMS_COUNT)
	{
		auto it = kAkCodecToFormatMap.find(m_config[streamId].getValue(kConfigCodec, -1));
		if (it != kAkCodecToFormatMap.end())
		{
			retVal = it->second;
		}
	}

	return retVal;
}


unsigned int AnykaCameraManager::getWidth(size_t streamId)
{
	return streamId < STREAMS_COUNT
		? m_config[streamId].getValue<unsigned int>(kConfigWidth, 0)
		: 0;
}


unsigned int AnykaCameraManager::getHeight(size_t streamId)
{
	return streamId < STREAMS_COUNT
		? m_config[streamId].getValue<unsigned int>(kConfigHeight, 0)
		: 0;
}


int AnykaCameraManager::getSampleRate(size_t streamId)
{
	return streamId < STREAMS_COUNT
		? m_config[streamId].getValue<unsigned int>(kConfigSampleRate, 0)
		: 0;
}


int AnykaCameraManager::getChannels(size_t streamId)
{
	return streamId < STREAMS_COUNT
		? m_config[streamId].getValue<unsigned int>(kConfigChannels, 0)
		: 0;
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
	if (ak_vi_match_sensor(m_config[kMainVideo].getValue(kConfigSensor).c_str()) == AK_SUCCESS)
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

	attr.res[VIDEO_CHN_MAIN].width  = m_config[VideoHigh].getValue(kConfigWidth,  0);
	attr.res[VIDEO_CHN_MAIN].height = m_config[VideoHigh].getValue(kConfigHeight, 0);
	attr.res[VIDEO_CHN_SUB].width   = m_config[VideoLow].getValue(kConfigWidth,   0);
	attr.res[VIDEO_CHN_SUB].height  = m_config[VideoLow].getValue(kConfigHeight,  0);

	attr.res[VIDEO_CHN_MAIN].max_width  = 1920;
	attr.res[VIDEO_CHN_MAIN].max_height = 1080;
	attr.res[VIDEO_CHN_SUB].max_width   = 640;
	attr.res[VIDEO_CHN_SUB].max_height  = 480;
	
	attr.crop.left = 0;
	attr.crop.top = 0;
	attr.crop.width = 1920;
	attr.crop.height = 1080;

	bool retVal = false;

	if (ak_vi_set_channel_attr(m_videoDevice, &attr) == AK_SUCCESS && 
		ak_vi_set_fps(m_videoDevice, m_config[kMainVideo].getValue(kConfigFps, 0)) == AK_SUCCESS) 
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
	ai_param.channel_num = m_config[kMainAudio].getValue(kConfigChannels,   0);
	ai_param.sample_rate = m_config[kMainAudio].getValue(kConfigSampleRate, 0);

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

	const int volume = m_config[kMainAudio].getValue(kConfigVolume, 10); //0 - 12, 0 - mute
	ak_ai_set_volume(m_audioDevice, volume);
	ak_ai_clear_frame_buffer(m_audioDevice);

	int interval = -1;
	int sampleRate = m_config[kMainAudio].getValue(kConfigSampleRate, 8000);
	int type = m_config[kMainAudio].getValue(kConfigCodec, AK_AUDIO_TYPE_AAC);
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

	param.width   		= m_config[streamId].getValue(kConfigWidth, 0);
	param.height  		= m_config[streamId].getValue(kConfigHeight, 0);
	param.minqp   		= m_config[streamId].getValue(kConfigMinQp, 0);
	param.maxqp   		= m_config[streamId].getValue(kConfigMaxQp, 0);
	param.fps     		= m_config[streamId].getValue(kConfigFps, 0);
	param.goplen  		= m_config[streamId].getValue(kConfigGopLen, 0);
	param.bps     		= m_config[streamId].getValue(kConfigBps, 0);
	param.profile 		= (profile_mode) m_config[streamId].getValue(kConfigProfile, 0);
	param.use_chn 		= streamId == VideoHigh ? ENCODE_MAIN_CHN : ENCODE_SUB_CHN;
	param.enc_grp 		= streamId == VideoHigh ? ENCODE_MAINCHN_NET : ENCODE_SUBCHN_NET;
	param.br_mode 		= (bitrate_ctrl_mode) m_config[streamId].getValue(kConfigBrMode, 0);
	param.enc_out_type  = (encode_output_type) m_config[streamId].getValue(kConfigCodec, 0);

	return param;
}


audio_param AnykaCameraManager::getAudioEncodeParams(size_t streamId)
{
	audio_param retVal = {AK_AUDIO_TYPE_UNKNOWN, 0};

	retVal.channel_num = m_config[streamId].getValue(kConfigChannels, 0);
	retVal.sample_bits = 16;
	retVal.sample_rate = m_config[streamId].getValue(kConfigSampleRate, 0);
	retVal.type        = (ak_audio_type)m_config[streamId].getValue(kConfigCodec, 0);

	return retVal;
}


encode_param AnykaCameraManager::getJpegEncodeParams()
{
	encode_param param 	= getVideoEncodeParams(VideoLow); // Use jpeg from low stream.

	param.fps 			= m_config[kMainVideo].getValue(kConfigJpgFps, 1);
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