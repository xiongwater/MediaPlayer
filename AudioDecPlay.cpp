#include "AudioDecPlay.h"



//静态成员声明

uint8_t* AudioDecPlay::m_pAudioOutBuffer = NULL;
SwrContext *  AudioDecPlay::m_audio_convert_ctx = NULL;//(音频转码器)
int AudioDecPlay::m_out_nb_samples = 0;
FramePacket AudioDecPlay::m_frame_packet;
int AudioDecPlay::m_nb_channels;//音频声道数量  
AVSampleFormat AudioDecPlay::m_out_sample_fmt;//输出的采样格式 16bit PCM  

AudioDecPlay::AudioDecPlay(int video_stream, const AVFormatContext* pFormatCtx, QObject *parent)
	: QThread(parent)
	, m_audioStream(video_stream)
{
	//m_out_ch_layout=
	m_nAudioInitRet = InitDecodec(pFormatCtx);
}

AudioDecPlay::~AudioDecPlay()
{
}

int AudioDecPlay::InitDecodec(const AVFormatContext*& pFormatCtx)
{
	if (m_audioStream == -1)
	{
		m_errInfo.append("unknown audioStream Failed");
		return -1;
	}
	//根据音频类型查找音频解码器
	m_pAudioCodecCtx = pFormatCtx->streams[m_audioStream]->codec;
	m_pAudioCodec = avcodec_find_decoder(m_pAudioCodecCtx->codec_id);

	if (m_pAudioCodec == NULL)
	{
		m_errInfo.append(",m_pAudioCodec is null");
		return -1;
	}
	//打开音频解码器
	if (avcodec_open2(m_pAudioCodecCtx, m_pAudioCodec, NULL) < 0)
	{
		m_errInfo.append(",open audio avcodec Failed");
		return -1;
	}

	/*m_pOriginalAudioFrame = av_frame_alloc();
	m_pOutputAudioFrame = av_frame_alloc();*/

	//=================================================================音频转码准备====================================//
	enum AVSampleFormat in_sample_fmt = m_pAudioCodecCtx->sample_fmt;//输入的采样格式  
	m_out_sample_rate = m_pAudioCodecCtx->sample_rate;//输入的采样率    音频采样率是指录音设备在一秒钟内对声音信号的采样次数，采样频率越高声音的还原就越真实越自然。
	uint64_t in_ch_layout = m_pAudioCodecCtx->channel_layout;//输入的声道布局  声道是指声音在录制或播放时在不同空间位置采集或回放的相互独立的音频信号，所以声道数也就是声音录制时的音源数量或回放时相应的扬声器数量。
	//m_out_ch_layout = in_ch_layout;
	m_out_ch_layout = AV_CH_LAYOUT_STEREO;
	m_audio_convert_ctx = swr_alloc();//获得音频转码器									
	swr_alloc_set_opts(m_audio_convert_ctx, m_out_ch_layout, m_out_sample_fmt, m_out_sample_rate, in_ch_layout, in_sample_fmt, m_out_sample_rate, 0, NULL);  //初始化音频转码器
	swr_init(m_audio_convert_ctx);
	m_nb_channels = av_get_channel_layout_nb_channels(m_out_ch_layout);//获取声道个数 
	m_out_nb_samples = m_pAudioCodecCtx->frame_size;//如果为m_out_nb_samples==0，则表示音频编码器支持在每个呼叫中??接收不同数量的采样。#define AV_CODEC_CAP_VARIABLE_FRAME_SIZE（1 << 16）

	if (m_out_nb_samples)
	{
		int SamplesSize = av_samples_get_buffer_size(NULL, m_nb_channels, m_out_nb_samples, m_out_sample_fmt, 1);
		m_pAudioOutBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);//用于存储转码后音频帧数据
		//emit sig_out_audio_param(m_out_sample_rate, m_nb_channels, m_out_nb_samples);//初始化SDL音频所需参数
		writeLog("音频转码准备", "OK", "无");
	}
	else
	{
		//为0表示每个采样数不确定？
		m_pAudioOutBuffer = (uint8_t *)av_malloc(1024);//存储pcm数据  test															
		//emit sig_out_audio_param(m_out_sample_rate, m_nb_channels, 1024);
		writeLog("音频转码准备", "OK1024", "无");
	}
	//======================音频转码准备====================================//

	//////////////////////////////

	return 0;
}

void AudioDecPlay::startPlay()
{
	SDL_LockAudio();

	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return;
	}

	wanted_spec.freq = m_out_sample_rate;// audioCodecCtx->sample_rate; 44100 ------------ 采样率
	wanted_spec.format = AUDIO_S16SYS;//格式：这告诉SDL我们将给它什么格式。 “S16SYS”中的“S”代表“有符号”，16表示每个样本长16位，“SYS”表示顺序取决于您所在的系统。这是avcodec_decode_audio2给我们音频的格式。
	wanted_spec.channels = m_nb_channels;// audioCodecCtx->channels; 2----频道：音频频道的数量。
	wanted_spec.silence = 0;//沉默：这是表示沉默的价值。由于音频是有符号的，0当然是通常的值。
	wanted_spec.samples = m_out_nb_samples == 0 ? 1024 : m_out_nb_samples;
	//wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;   //1024 样本：这是SDL在要求更多音频时给予我们的音频缓冲区的大小。这里的价值在512到8192之间。 ffplay使用1024。
	wanted_spec.callback = audio_callback;//多线程不可用										  //wanted_spec.callback = audio_callback_ex;
	wanted_spec.userdata = m_pAudioCodecCtx;//回调函数的第一个自定义参数
	if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
	{


		const char* ss = SDL_GetError();
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		return;
	}

	SDL_UnlockAudio();//开始播放
	SDL_PauseAudio(0);
}

void AudioDecPlay::audio_callback(void *userdata, Uint8 *stream, int len)
{
	static const int Max_audio_data_size = (MAX_AUDIO_FRAME_SIZE * 3) / 2;
	static uint8_t audio_buf[Max_audio_data_size];//缓存区
	static unsigned int audio_buf_size = 0;//缓存区大小
	static unsigned int audio_buf_index = 0;//当前应当取出缓存所在首地址
	static AVFrame* pOriginalAudioFrame = av_frame_alloc();
	static AVFrame* pOutputAudioFrame = av_frame_alloc();

	AVCodecContext* pAudioCodecCtx = (AVCodecContext*)userdata;

	int len1;//用于记录每次取出缓存大小
	int audio_data_size;//缓存最大长度
	SDL_memset(stream, 0, len);

	AVPacket *packet = NULL;
	//喂饱了才跳出循环^o^
	bool to_out = false;
	while (len > 0)
	{

		if (to_out)
		{
			break;
		}
		//当索引位置超过最大时，表示缓存应该更新了
		if (audio_buf_index >= audio_buf_size)
		{
			audio_buf_size = 0;
		
			while (1)
			{
				packet = m_frame_packet.packet_queue_get();
				/////////////////////////////
				if (!packet)
				{
					to_out = true;
					break;
				}

				int got_frame = 0;

				if (avcodec_decode_audio4(pAudioCodecCtx, pOriginalAudioFrame, &got_frame, packet) >= 0)
				{
					if (got_frame)
					{
						int auto_size = swr_get_out_samples(m_audio_convert_ctx, pOriginalAudioFrame->nb_samples);

						//audio_pts = packet->pts*av_q2d(m_pAudioCodecCtx->time_base);
						//音频格式转换(音频重采样)  
						swr_convert(m_audio_convert_ctx,//音频转换上下文  
							&m_pAudioOutBuffer,//输出缓存  
							(m_out_nb_samples ? m_out_nb_samples : 1024),//MAX_AUDIO_FRAME_SIZE,//(m_out_nb_samples? m_out_nb_samples:1024),//每次输出大小  //每个声道的大小
							(const uint8_t **)pOriginalAudioFrame->data,//输入数据  
							pOriginalAudioFrame->nb_samples);//输入  

						int audio_out_buffer_size = 0;
						if (m_out_nb_samples)
						{
							audio_out_buffer_size = av_samples_get_buffer_size(NULL, m_nb_channels, m_out_nb_samples, m_out_sample_fmt, 1);
						}
						else
						{
							audio_out_buffer_size = 1024;
						}
						if (audio_buf_size + audio_out_buffer_size >= Max_audio_data_size)
						{
							break;//跳出填充缓存循环
						}
						memcpy(audio_buf + audio_buf_size, m_pAudioOutBuffer, audio_out_buffer_size);
						audio_buf_size += audio_out_buffer_size;
					}
				}
				if (packet)
				{
					av_free_packet(packet);
				}	
			}
			/////////////////////
			audio_buf_index = 0;
		}
		/*  查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy */
		len1 = audio_buf_size - audio_buf_index;//缓存剩余可取数据大小
		if (len1 > len) {
			len1 = len;
		}
		SDL_MixAudio(stream, (uint8_t *)audio_buf + audio_buf_index, len1, SDL_MIX_MAXVOLUME);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}




	//static AVFrame* pOriginalAudioFrame = av_frame_alloc();
	//static AVFrame* pOutputAudioFrame = av_frame_alloc();
	//AVCodecContext* pAudioCodecCtx = (AVCodecContext*)userdata;
	//SDL_memset(stream, 0, len);
	//AVPacket *packet = NULL;
	//packet = m_frame_packet.packet_queue_get();
	//if (!packet)
	//{
	//	return;
	//}

	//int got_frame = 0;

	//if (avcodec_decode_audio4(pAudioCodecCtx, pOriginalAudioFrame, &got_frame, packet) >= 0)
	//{
	//	if (got_frame)
	//	{
	//		int auto_size = swr_get_out_samples(m_audio_convert_ctx, pOriginalAudioFrame->nb_samples);

	//
	//		swr_convert(m_audio_convert_ctx,//音频转换上下文  
	//			&m_pAudioOutBuffer,//输出缓存  
	//			(m_out_nb_samples ? m_out_nb_samples : 1024),
	//			(const uint8_t **)pOriginalAudioFrame->data,
	//			pOriginalAudioFrame->nb_samples);

	//		int audio_out_buffer_size = 0;
	//		if (m_out_nb_samples)
	//		{
	//			audio_out_buffer_size = av_samples_get_buffer_size(NULL, m_nb_channels, m_out_nb_samples, m_out_sample_fmt, 1);
	//		}
	//		else
	//		{
	//			audio_out_buffer_size = 1024;
	//		}
	//		SDL_MixAudio(stream, m_pAudioOutBuffer, len>audio_out_buffer_size ? audio_out_buffer_size : len, SDL_MIX_MAXVOLUME);
	//	}
	//	
	//}
	//	av_free_packet(packet);
}


void AudioDecPlay::slot_GetPacket(AVPacket *pkt)
{
	AppendAVPacket(pkt);
}
