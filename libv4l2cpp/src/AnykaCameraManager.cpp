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
#include "AnykaAudioEncoder.h"
#include <algorithm>
#include <map>

extern "C"
{
	#include "ak_vi.h"
	#include "ak_ai.h"
	#include "ak_ao.h"
	#include "ak_common.h"
}

const size_t AnykaCameraManager::kInvalidStreamId = (size_t)-1;
const std::string kConfigSensor      	 = "sensor";
const std::string kConfigWidth       	 = "width";
const std::string kConfigHeight      	 = "height";
const std::string kConfigSampleRate  	 = "samplerate";
const std::string kConfigSampleInterval  = "sampleinterval";
const std::string kConfigChannels    	 = "channels";
const std::string kConfigVolume      	 = "volume";
const std::string kConfigCodec	     	 = "codec";
const std::string kConfigMinQp	     	 = "minqp";
const std::string kConfigMaxQp	     	 = "maxqp";
const std::string kConfigFps	     	 = "fps";
const std::string kConfigGopLen	     	 = "goplen";
const std::string kConfigBps	     	 = "bps";
const std::string kConfigProfile     	 = "profile";
const std::string kConfigBrMode      	 = "brmode";
const std::string kConfigJpgFps      	 = "jpegfps";
const std::string kConfigJpgStreamId     = "jpegstream";
const std::string kConfigOsdFontPath	 = "osdfontpath";
const std::string kConfigOsdOrigFontSize = "origosdfontsize";
const std::string kConfigOsdFontSize     = "osdfontsize";
const std::string kConfigOsdX			 = "osdx";
const std::string kConfigOsdY			 = "osdy";
const std::string kConfigOsdText		 = "osdtext";
const std::string kConfigOsdFrontColor   = "osdfrontcolor";
const std::string kConfigOsdBackColor    = "osdbackcolor";
const std::string kConfigOsdEdgeColor    = "osdedgecolor";
const std::string kConfigOsdAlpha        = "osdalpha";
const std::string kConfigOsdEnabled      = "osdenabled";
const std::string kConfigMdEnabled		 = "mdenabled";
const std::string kConfigMdSensitivity   = "mdsens";
const std::string kConfigMdFps		     = "mdfps";
const std::string kConfigMdX		     = "mdx";
const std::string kConfigMdY		     = "mdy";
const std::string kConfigMdWidth		 = "mdwidth";
const std::string kConfigMdHeight		 = "mdheight";
const std::string kConfigDayNightMode    = "daynight";
const std::string kConfigIrLed    		 = "irled";
const std::string kConfigIrCut    		 = "ircut";
const std::string kConfigVideoDay 		 = "videoday";
const std::string kConfigDayNightInfo    = "daynightinfo";
const std::string kConfigDayNightLum     = "daynightlum";
const std::string kConfigNightDayLum     = "nightdaylum";
const std::string kConfigDayNightAwb     = "daynightawb";
const std::string kConfigNightDayAwb     = "nightdayawb";
const std::string kConfigAbortOnError    = "abortonerror";
const std::string kConfigSmartMode		 = "smartmode";
const std::string kConfigSmartGopLen     = "smartgoplen";
const std::string kConfigSmartQuality    = "smartquality";
const std::string kConfigSmartStatic     = "smartstatic";
const std::string kConfigMaxKbps    	 = "maxkbps";
const std::string kConfigTargetKbps      = "targetkbps";


const std::map<int, int> kAkCodecToFormatMap
{
	{H264_ENC_TYPE, 			V4L2_PIX_FMT_H264},
	{HEVC_ENC_TYPE, 			V4L2_PIX_FMT_HEVC},
	{AK_AUDIO_TYPE_AAC, 		SND_PCM_FORMAT_AAC},
	{AK_AUDIO_TYPE_PCM, 		SND_PCM_FORMAT_S16_LE},
	{AK_AUDIO_TYPE_PCM_ALAW, 	SND_PCM_FORMAT_A_LAW},
	{AK_AUDIO_TYPE_PCM_ULAW, 	SND_PCM_FORMAT_MU_LAW}
};


const std::map<std::string, std::string> kDefaultMainConfig
{
	{kConfigSensor    	    , "/etc/jffs2/isp_f23_mipi2lane.conf"},
	{kConfigFps	   	  	    , "25"},
	{kConfigJpgFps          , "1"},
	{kConfigJpgStreamId     , "1"},
	{kConfigVolume    	    , "10"},
	{kConfigChannels  	    , "1"},
	{kConfigSampleRate	    , "8000"},
	{kConfigSampleInterval  , std::to_string(AUDIO_DEFAULT_INTERVAL)},
	{kConfigOsdFontPath     , "/usr/local/ak_font_16.bin"},
	{kConfigOsdOrigFontSize , "16"},
	{kConfigOsdFrontColor   , "1"},
	{kConfigOsdBackColor   	, "0"},
	{kConfigOsdEdgeColor   	, "2"},
	{kConfigOsdAlpha   		, "0"},
	{kConfigOsdEnabled      , "1"},
	{kConfigOsdText   	    , "%H:%M:%S %d.%m.%Y"},
	{kConfigMdEnabled       , "1"},
	{kConfigMdSensitivity   , "60"}, // 1 - 100
	{kConfigMdFps      		, "10"},
	{kConfigMdX      	    , "0"},
	{kConfigMdY             , "0"},
	{kConfigMdWidth         , "100"},
	{kConfigMdHeight        , "100"},
	{kConfigDayNightMode    , "2"},
	{kConfigIrLed    		, "0"},
	{kConfigIrCut    		, "1"},
	{kConfigVideoDay		, "1"},
	{kConfigDayNightInfo    , "0"},
	{kConfigDayNightLum		, "6000"},
	{kConfigNightDayLum		, "2000"},
	{kConfigDayNightAwb		, "90000"},
	{kConfigNightDayAwb		, "1200"},
	{kConfigAbortOnError    , "0"},
};


const std::map<std::string, std::string> kDefaultConfig[STREAMS_COUNT]
{
	// VideoHigh
	{
		{kConfigWidth       , "1920"},
		{kConfigHeight      , "1080"},
		{kConfigCodec	    , std::to_string(HEVC_ENC_TYPE)}, // 2
		{kConfigMinQp	    , "20"}, 
		{kConfigMaxQp	    , "51"}, 
		{kConfigFps	   	    , "25"},
		{kConfigGopLen	    , "50"}, 
		{kConfigBps	   	    , "1500"},
		{kConfigProfile     , std::to_string(PROFILE_HEVC_MAIN)}, // 0
		{kConfigBrMode      , std::to_string(BR_MODE_CBR)}, // 0
		{kConfigSmartMode   , "0"},
		{kConfigSmartGopLen	, "300"},
		{kConfigSmartQuality, "100"},
		{kConfigSmartStatic , "550"},
		{kConfigMaxKbps     , "1000"},
		{kConfigTargetKbps	, "600"},
		{kConfigOsdFontSize , "32"},
		{kConfigOsdX   	    , "20"},
		{kConfigOsdY   	    , "24"},
	},

	// VideoLow
	{
		{kConfigWidth       , "640"},
		{kConfigHeight      , "360"},
		{kConfigCodec	    , std::to_string(H264_ENC_TYPE)}, // 0
		{kConfigMinQp	    , "20"}, 
		{kConfigMaxQp	    , "51"}, 
		{kConfigFps	   	    , "25"},
		{kConfigGopLen	    , "50"}, 
		{kConfigBps	   	    , "500"},
		{kConfigProfile     , std::to_string(PROFILE_MAIN)}, // 0
		{kConfigBrMode      , std::to_string(BR_MODE_CBR)}, // 0
		{kConfigSmartMode   , "0"},
		{kConfigSmartGopLen	, "300"},
		{kConfigSmartQuality, "100"},
		{kConfigSmartStatic , "550"},
		{kConfigMaxKbps     , "500"},
		{kConfigTargetKbps	, "300"},
		{kConfigOsdFontSize , "16"},
		{kConfigOsdX   	    , "10"},
		{kConfigOsdY   	    , "12"},
	},

	// AudioHigh
	{
		{kConfigSampleRate, "8000"},
		{kConfigChannels  , "1"},
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

const int kSharedConfUpdateCount = 100;
const int kMaxMotionCount = 200;
const char* kDefaultConfigName = "anykacam.ini";
const FrameRef kEmptyFrameRef;
const size_t kMaxVideoBufferSize = 512 * 1024;
const size_t kMaxAudioBufferSize = 8 * 1024;

static void updateDefaultConfigSection(const std::shared_ptr<ConfigFile> &config, 
	const std::map<std::string, std::string> &defConfig, const std::string &section)
{
	for (const auto &it: defConfig)
	{
		if (!config->hasValue(section, it.first))
		{
			LOG(NOTICE)<<"default config: "<<section<<") "<<it.first<<"="<<it.second;
			config->setValue(section, it.first, it.second);
		}
	}
}


static void updateDefaultConfig(const std::shared_ptr<ConfigFile> &config)
{
	for (size_t i = 0; i < STREAMS_COUNT; ++i)
	{
		updateDefaultConfigSection(config,  kDefaultConfig[i], std::to_string(i));
	}

	updateDefaultConfigSection(config,  kDefaultMainConfig, std::string());
}


static void clearAudioOutput()
{
	struct pcm_param param = {0};
	param.sample_bits = 16;
	param.channel_num = AUDIO_CHANNEL_MONO;
	param.sample_rate = 8000;

	void *handle = ak_ao_open(&param);
	if (handle != NULL) 
	{
		ak_ao_enable_speaker(handle, AUDIO_FUNC_ENABLE);
		ak_ao_set_resample(handle, AUDIO_FUNC_DISABLE);
		ak_ao_set_volume(handle, 6);
		ak_ao_clear_frame_buffer(handle);
		ak_ao_close(handle);
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
	, m_currentSharedConfig({0})
	, m_sharedConfUpdateCounter(-1)
	, m_lastMotionDetected(false)
	, m_motionCounter(0)
	, m_motionDetectionFd(-1)
	, m_abortOnError(false)
{
	LOG(DEBUG)<<"AnykaCameraManager construct";

	ak_print_set_level(LOG_LEVEL_NOTICE);

	const SharedConfig *newSharedConfig = SharedMemory::instance().readConfig();
	memcpy(&m_currentSharedConfig, newSharedConfig, sizeof(m_currentSharedConfig));

	std::shared_ptr<ConfigFile> config = std::make_shared<ConfigFile>(
		newSharedConfig->configFilePath[0] != 0
			? newSharedConfig->configFilePath
			: kDefaultConfigName);

	updateDefaultConfig(config);

	for (size_t i = 0; i < STREAMS_COUNT; ++i)
	{
		if (i == VideoHigh || i == VideoLow)
		{
			m_streams[i].encoder = new AnykaVideoEncoder();
		}
		else
		{
			m_streams[i].encoder = new AnykaAudioEncoder();
		}

		m_config[i].init(config, i);
	}

	m_mainConfig.init(config, std::string());

	m_abortOnError = m_mainConfig.getValue(kConfigAbortOnError, 0) != 0;

	clearAudioOutput();
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
	return streamId < AudioHigh
		? kMaxVideoBufferSize
		: streamId < STREAMS_COUNT
			? kMaxAudioBufferSize
			: 0;
}


FrameRef AnykaCameraManager::getEncodedFrame(size_t streamId)
{
	return streamId < STREAMS_COUNT
		? m_streams[streamId].encoder->getEncodedFrame()
		: kEmptyFrameRef;
}


bool AnykaCameraManager::initVideoDevice()
{
	if (ak_vi_match_sensor(m_mainConfig.getValue(kConfigSensor).c_str()) == AK_SUCCESS)
	{
		LOG(NOTICE)<<"ak_vi_match_sensor success";
		m_videoDevice = ak_vi_open(VIDEO_DEV0);

		if (m_videoDevice == NULL)
		{
			abortIfNeed();
		}
	}
	else
	{
		LOG(ERROR)<<"ak_vi_match_sensor failed";

		abortIfNeed();
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
		ak_vi_set_fps(m_videoDevice, m_mainConfig.getValue(kConfigFps, 0)) == AK_SUCCESS) 
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
	ai_param.channel_num = m_mainConfig.getValue(kConfigChannels,   0);
	ai_param.sample_rate = m_mainConfig.getValue(kConfigSampleRate, 0);

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
			LOG(ERROR)<<"ak_ai_set_source failed";
			ak_ai_close(m_audioDevice);
			m_audioDevice = NULL;

			abortIfNeed();
		}
	}
	else
	{
		LOG(ERROR)<<"ak_ai_open failed";

		abortIfNeed();
	}

	return m_audioDevice != NULL;
}


bool AnykaCameraManager::setAudioParams()
{
	ak_ai_set_aec(m_audioDevice, AUDIO_FUNC_ENABLE);
	ak_ai_set_nr_agc(m_audioDevice, AUDIO_FUNC_ENABLE);
	ak_ai_set_resample(m_audioDevice, AUDIO_FUNC_DISABLE);

	const int volume = m_mainConfig.getValue(kConfigVolume, 10); //0 - 12, 0 - mute
	ak_ai_set_volume(m_audioDevice, volume);
	ak_ai_clear_frame_buffer(m_audioDevice);

	const int interval = m_mainConfig.getValue(kConfigSampleInterval, AUDIO_DEFAULT_INTERVAL);

	if (ak_ai_set_frame_interval(m_audioDevice, interval) != AK_SUCCESS)
	{
		LOG(WARN)<<"ak_ai_set_frame_interval failed";
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

			startDayNight();
			startOsd();
			startMotionDetection();
		}
		else
		{
			LOG(ERROR)<<"can't start streams";
			stop();
		}
	}

	return retVal;
}


void AnykaCameraManager::startOsd()
{
	if (m_mainConfig.getValue(kConfigOsdEnabled, 0) != 0)
	{
		if (m_osd.start(m_videoDevice, m_mainConfig.getValue(kConfigOsdFontPath), m_mainConfig.getValue(kConfigOsdOrigFontSize, 0)))
		{
			m_osd.setOsdText(m_mainConfig.getValue(kConfigOsdText));
			m_osd.setPos(m_videoDevice,  
				m_config[VideoHigh].getValue(kConfigOsdFontSize, 0), m_config[VideoLow].getValue(kConfigOsdFontSize, 0),
				m_config[VideoHigh].getValue(kConfigOsdX, 0), m_config[VideoHigh].getValue(kConfigOsdY, 0),
				m_config[VideoLow].getValue(kConfigOsdX, 0),  m_config[VideoLow].getValue(kConfigOsdY, 0));

			m_osd.setColor(m_mainConfig.getValue(kConfigOsdFrontColor, 0), m_mainConfig.getValue(kConfigOsdBackColor, 0), 
				m_mainConfig.getValue(kConfigOsdEdgeColor, 0), m_mainConfig.getValue(kConfigOsdAlpha, 0));
		}
		else
		{
			LOG(ERROR)<<"can't init OSD";
		}
	}
}


void AnykaCameraManager::startMotionDetection()
{
	if (m_mainConfig.getValue(kConfigMdEnabled, 0) != 0)
	{
		if (!m_motionDetect.start(m_videoDevice, 
				m_mainConfig.getValue(kConfigMdSensitivity, 0), m_mainConfig.getValue(kConfigMdFps, 0),
				m_mainConfig.getValue(kConfigMdX, 0), m_mainConfig.getValue(kConfigMdY, 0), 
				m_mainConfig.getValue(kConfigMdWidth, 0), m_mainConfig.getValue(kConfigMdHeight, 0)))
		{
			LOG(ERROR)<<"can't init Motion detection";
		}
	}
}


void AnykaCameraManager::startDayNight()
{
	m_dayNight.start(m_videoDevice, 
		m_mainConfig.getValue(kConfigDayNightLum, 0),
		m_mainConfig.getValue(kConfigNightDayLum, 0),
		m_mainConfig.getValue(kConfigDayNightAwb, 0),
		m_mainConfig.getValue(kConfigNightDayAwb, 0));

	m_dayNight.setPrintInfo(m_mainConfig.getValue(kConfigDayNightInfo, 0) == 1);

	const AnykaDayNight::Mode mode = (AnykaDayNight::Mode)m_mainConfig.getValue(kConfigDayNightMode, 0);
	
	m_dayNight.setMode(mode);

	if (mode == AnykaDayNight::Disabled)
	{
		m_dayNight.setIrCut(m_mainConfig.getValue(kConfigIrCut, 0) == 1);
		m_dayNight.setIrLed(m_mainConfig.getValue(kConfigIrLed, 0) == 1);
		m_dayNight.setVideo(m_mainConfig.getValue(kConfigVideoDay, 0) == 1);
	}
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
	m_dayNight.stop();
	m_osd.stop();
	m_motionDetect.stop();

	stopVideoCapture();
	stopAudioCapture();

	if (m_motionDetectionFd != -1)
	{
		close(m_motionDetectionFd);
		m_motionDetectionFd = -1;
	}

	m_lastMotionDetected = false;
	m_motionCounter = 0;
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

	return m_videoDevice != NULL && ak_thread_create(&m_threadId, AnykaCameraManager::thread, NULL, ANYKA_THREAD_MIN_STACK_SIZE, 90) == AK_SUCCESS;
}


VideoEncodeParam AnykaCameraManager::getVideoEncodeParams(size_t streamId)
{
	VideoEncodeParam param = {0};

	param.videoParams.width   		= m_config[streamId].getValue(kConfigWidth, 0);
	param.videoParams.height  		= m_config[streamId].getValue(kConfigHeight, 0);
	param.videoParams.minqp   		= m_config[streamId].getValue(kConfigMinQp, 0);
	param.videoParams.maxqp   		= m_config[streamId].getValue(kConfigMaxQp, 0);
	param.videoParams.fps     		= m_config[streamId].getValue(kConfigFps, 0);
	param.videoParams.goplen  		= m_config[streamId].getValue(kConfigGopLen, 0);
	param.videoParams.bps     		= m_config[streamId].getValue(kConfigBps, 0);
	param.videoParams.profile 		= (profile_mode) m_config[streamId].getValue(kConfigProfile, 0);
	param.videoParams.use_chn 		= streamId == VideoHigh ? ENCODE_MAIN_CHN : ENCODE_SUB_CHN;
	param.videoParams.enc_grp 		= streamId == VideoHigh ? ENCODE_MAINCHN_NET : ENCODE_SUBCHN_NET;
	param.videoParams.br_mode 		= (bitrate_ctrl_mode) m_config[streamId].getValue(kConfigBrMode, 0);
	param.videoParams.enc_out_type  = (encode_output_type) m_config[streamId].getValue(kConfigCodec, 0);

	param.smartParams.smart_mode	     =  m_config[streamId].getValue(kConfigSmartMode, 0);
	param.smartParams.smart_goplen       =  m_config[streamId].getValue(kConfigSmartGopLen, 0);
	param.smartParams.smart_quality      =  m_config[streamId].getValue(kConfigSmartQuality, 0);
	param.smartParams.smart_static_value =  m_config[streamId].getValue(kConfigSmartStatic, 0);

	param.maxKbps    = m_config[streamId].getValue(kConfigMaxKbps, 0);
	param.targetKbps = m_config[streamId].getValue(kConfigTargetKbps, 0);

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


VideoEncodeParam AnykaCameraManager::getJpegEncodeParams()
{
	VideoEncodeParam param 	= getVideoEncodeParams(m_mainConfig.getValue(kConfigJpgStreamId, 1) == 0
		? VideoHigh
		: VideoLow);

	param.videoParams.fps 			= m_mainConfig.getValue(kConfigJpgFps, 1);
	param.videoParams.enc_out_type 	= MJPEG_ENC_TYPE;
	param.videoParams.enc_grp 		= ENCODE_PICTURE;
	param.smartParams.smart_mode    = 0;

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

			m_osd.update();
			processMotionDetection();
			processSharedConfig();
		}

		stop();
	}
	else
	{
		abortIfNeed();
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
		const FrameRef frame = m_jpegEncoder.getEncodedFrame();
		if (frame.isSet())
		{
			void *outPtr = SharedMemory::instance().lockImage(frame.getDataSize());

			if (outPtr != NULL)
			{
				memcpy(outPtr, frame.getData(), frame.getDataSize());

				SharedMemory::instance().unlockImage(outPtr);
			}
		}
	}

	return retVal;
}


void AnykaCameraManager::processSharedConfig()
{
	if (m_sharedConfUpdateCounter++ > kSharedConfUpdateCount)
	{
		m_sharedConfUpdateCounter = 0;

		const SharedConfig *newSharedConfig = SharedMemory::instance().readConfig();

		if (newSharedConfig->nightmode   != m_currentSharedConfig.nightmode ||
			newSharedConfig->dayNightAwb != m_currentSharedConfig.dayNightAwb ||
			newSharedConfig->dayNightLum != m_currentSharedConfig.dayNightLum ||
			newSharedConfig->nightDayAwb != m_currentSharedConfig.nightDayAwb ||
			newSharedConfig->nightDayLum != m_currentSharedConfig.nightDayLum)
		{
			m_dayNight.start(m_videoDevice, newSharedConfig->dayNightLum, newSharedConfig->nightDayLum,
				newSharedConfig->dayNightAwb, newSharedConfig->nightDayAwb);

			m_dayNight.setMode((AnykaDayNight::Mode)newSharedConfig->nightmode);
		}

		if (newSharedConfig->motionEnabled != m_currentSharedConfig.motionEnabled ||
			newSharedConfig->motionSensitivity != m_currentSharedConfig.motionSensitivity)
		{
			if (newSharedConfig->motionEnabled)
			{
				m_motionDetect.start(m_videoDevice, newSharedConfig->motionSensitivity,
					m_mainConfig.getValue(kConfigMdFps, 0),
					m_mainConfig.getValue(kConfigMdX, 0), m_mainConfig.getValue(kConfigMdY, 0), 
					m_mainConfig.getValue(kConfigMdWidth, 0), m_mainConfig.getValue(kConfigMdHeight, 0));
			}
			else
			{
				m_motionDetect.stop();
			}
		}

		if (newSharedConfig->irCut != m_currentSharedConfig.irCut)
		{
			m_dayNight.setIrCut(newSharedConfig->irCut);
		}

		if (newSharedConfig->irLed != m_currentSharedConfig.irLed)
		{
			m_dayNight.setIrLed(newSharedConfig->irLed);
		}

		if (newSharedConfig->videoDay != m_currentSharedConfig.videoDay)
		{
			m_dayNight.setVideo(newSharedConfig->videoDay);
		}

		if (newSharedConfig->osdEnabled 	 != m_currentSharedConfig.osdEnabled ||
			newSharedConfig->osdAlpha 		 != m_currentSharedConfig.osdAlpha ||
			newSharedConfig->osdBackColor 	 != m_currentSharedConfig.osdBackColor ||
			newSharedConfig->osdEdgeColor 	 != m_currentSharedConfig.osdEdgeColor ||
			newSharedConfig->osdFrontColor 	 != m_currentSharedConfig.osdFrontColor ||
			newSharedConfig->osdFontSizeHigh != m_currentSharedConfig.osdFontSizeHigh ||
			newSharedConfig->osdXHigh 	     != m_currentSharedConfig.osdXHigh ||
			newSharedConfig->osdYHigh		 != m_currentSharedConfig.osdYHigh ||
			newSharedConfig->osdFontSizeLow  != m_currentSharedConfig.osdFontSizeLow ||
			newSharedConfig->osdXLow	     != m_currentSharedConfig.osdXLow ||
			newSharedConfig->osdYLow		 != m_currentSharedConfig.osdYLow ||
			strncmp(newSharedConfig->osdText, m_currentSharedConfig.osdText, MAX_STR_SIZE) != 0)
		{
			if (newSharedConfig->osdEnabled && newSharedConfig->osdText[0] != 0)
			{
				if (m_osd.start(m_videoDevice, m_mainConfig.getValue(kConfigOsdFontPath), m_mainConfig.getValue(kConfigOsdOrigFontSize, 0)))
				{
					m_osd.setOsdText(newSharedConfig->osdText);
					m_osd.setPos(m_videoDevice, 
						newSharedConfig->osdFontSizeHigh, newSharedConfig->osdFontSizeLow,
						newSharedConfig->osdXHigh, newSharedConfig->osdYHigh,
						newSharedConfig->osdXLow, newSharedConfig->osdYLow);

					m_osd.setColor(newSharedConfig->osdFrontColor, newSharedConfig->osdBackColor, 
						newSharedConfig->osdEdgeColor, newSharedConfig->osdAlpha);
				}
			}
			else
			{
				m_osd.stop();
			}
		}

		memcpy(&m_currentSharedConfig, newSharedConfig, sizeof(m_currentSharedConfig));
	}
}


void AnykaCameraManager::processMotionDetection()
{
	if (m_motionDetect.detect())
	{
		if (!m_lastMotionDetected)
		{
			writeMotionDetectionFlag(true);
			m_lastMotionDetected = true;
		}

		m_motionCounter = 0;
	}
	else if (m_lastMotionDetected && m_motionCounter++ > kMaxMotionCount)
	{
		m_lastMotionDetected = false;
		m_motionCounter = 0;
		writeMotionDetectionFlag(false);
	}
}


void AnykaCameraManager::writeMotionDetectionFlag(bool isMotionDetected)
{   
    if (m_motionDetectionFd == -1)
    {
        m_motionDetectionFd = open("/tmp/rec_control", O_WRONLY | O_CREAT, 0666);
        
        m_motionDetectionLock.l_type = F_WRLCK;
        m_motionDetectionLock.l_whence = SEEK_SET;
        m_motionDetectionLock.l_start = 0;
        m_motionDetectionLock.l_len = 0;
        m_motionDetectionLock.l_pid = getpid();
        
        LOG(INFO) << "Open motion control file /tmp/rec_control: " << m_motionDetectionFd << "\n";
    }
    
    if (m_motionDetectionFd != -1)
    {
        m_motionDetectionLock.l_type = F_WRLCK;
        if (fcntl(m_motionDetectionFd, F_SETLKW, &m_motionDetectionLock) < 0)
        {
            LOG(ERROR) << "Lock motion control file /tmp/rec_control FAILED\n";
        }
        else
        {
            if (lseek(m_motionDetectionFd, 0, SEEK_SET) == 0)
            {
                char val = isMotionDetected ? '1' : '0';
                if (write(m_motionDetectionFd, &val, 1) != 1)
                {
                    LOG(ERROR) << "Write to motion control file /tmp/rec_control FAILED\n";
                }
                else
                {
                    fdatasync(m_motionDetectionFd);
                }
            }
            else
            {
                LOG(ERROR) << "Seek motion control file /tmp/rec_control FAILED\n";
            }
            
            m_motionDetectionLock.l_type = F_UNLCK;
            if (fcntl(m_motionDetectionFd, F_SETLK, &m_motionDetectionLock) < 0)
            {
                LOG(ERROR) << "UnLock motion control file /tmp/rec_control FAILED\n";
            }
        }
    }
}


void* AnykaCameraManager::thread(void*)
{
	AnykaCameraManager& camMan = AnykaCameraManager::instance();

	LOG(DEBUG)<<"AnykaCameraManager thread started";

	camMan.processThread();

	LOG(DEBUG)<<"AnykaCameraManager thread stopped";

	ak_thread_exit();

	return NULL;
}


void AnykaCameraManager::abortIfNeed()
{
	if (m_abortOnError)
	{
		LOG(ERROR) << "Terminate program on error. Config param '" << kConfigAbortOnError <<"' set to 1\n";
		fflush(stdout);
		abort();
	}
}