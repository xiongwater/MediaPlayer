#include "AudioDecPlay.h"



//��̬��Ա����

uint8_t* AudioDecPlay::m_pAudioOutBuffer = NULL;
SwrContext *  AudioDecPlay::m_audio_convert_ctx = NULL;//(��Ƶת����)
int AudioDecPlay::m_out_nb_samples = 0;
FramePacket AudioDecPlay::m_frame_packet;
int AudioDecPlay::m_nb_channels;//��Ƶ��������  
AVSampleFormat AudioDecPlay::m_out_sample_fmt;//����Ĳ�����ʽ 16bit PCM  

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
	//������Ƶ���Ͳ�����Ƶ������
	m_pAudioCodecCtx = pFormatCtx->streams[m_audioStream]->codec;
	m_pAudioCodec = avcodec_find_decoder(m_pAudioCodecCtx->codec_id);

	if (m_pAudioCodec == NULL)
	{
		m_errInfo.append(",m_pAudioCodec is null");
		return -1;
	}
	//����Ƶ������
	if (avcodec_open2(m_pAudioCodecCtx, m_pAudioCodec, NULL) < 0)
	{
		m_errInfo.append(",open audio avcodec Failed");
		return -1;
	}

	/*m_pOriginalAudioFrame = av_frame_alloc();
	m_pOutputAudioFrame = av_frame_alloc();*/

	//=================================================================��Ƶת��׼��====================================//
	enum AVSampleFormat in_sample_fmt = m_pAudioCodecCtx->sample_fmt;//����Ĳ�����ʽ  
	m_out_sample_rate = m_pAudioCodecCtx->sample_rate;//����Ĳ�����    ��Ƶ��������ָ¼���豸��һ�����ڶ������źŵĲ�������������Ƶ��Խ�������Ļ�ԭ��Խ��ʵԽ��Ȼ��
	uint64_t in_ch_layout = m_pAudioCodecCtx->channel_layout;//�������������  ������ָ������¼�ƻ򲥷�ʱ�ڲ�ͬ�ռ�λ�òɼ���طŵ��໥��������Ƶ�źţ�����������Ҳ��������¼��ʱ����Դ������ط�ʱ��Ӧ��������������
	//m_out_ch_layout = in_ch_layout;
	m_out_ch_layout = AV_CH_LAYOUT_STEREO;
	m_audio_convert_ctx = swr_alloc();//�����Ƶת����									
	swr_alloc_set_opts(m_audio_convert_ctx, m_out_ch_layout, m_out_sample_fmt, m_out_sample_rate, in_ch_layout, in_sample_fmt, m_out_sample_rate, 0, NULL);  //��ʼ����Ƶת����
	swr_init(m_audio_convert_ctx);
	m_nb_channels = av_get_channel_layout_nb_channels(m_out_ch_layout);//��ȡ�������� 
	m_out_nb_samples = m_pAudioCodecCtx->frame_size;//���Ϊm_out_nb_samples==0�����ʾ��Ƶ������֧����ÿ��������??���ղ�ͬ�����Ĳ�����#define AV_CODEC_CAP_VARIABLE_FRAME_SIZE��1 << 16��

	if (m_out_nb_samples)
	{
		int SamplesSize = av_samples_get_buffer_size(NULL, m_nb_channels, m_out_nb_samples, m_out_sample_fmt, 1);
		m_pAudioOutBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);//���ڴ洢ת�����Ƶ֡����
		//emit sig_out_audio_param(m_out_sample_rate, m_nb_channels, m_out_nb_samples);//��ʼ��SDL��Ƶ�������
		writeLog("��Ƶת��׼��", "OK", "��");
	}
	else
	{
		//Ϊ0��ʾÿ����������ȷ����
		m_pAudioOutBuffer = (uint8_t *)av_malloc(1024);//�洢pcm����  test															
		//emit sig_out_audio_param(m_out_sample_rate, m_nb_channels, 1024);
		writeLog("��Ƶת��׼��", "OK1024", "��");
	}
	//======================��Ƶת��׼��====================================//

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

	wanted_spec.freq = m_out_sample_rate;// audioCodecCtx->sample_rate; 44100 ------------ ������
	wanted_spec.format = AUDIO_S16SYS;//��ʽ�������SDL���ǽ�����ʲô��ʽ�� ��S16SYS���еġ�S�������з��š���16��ʾÿ��������16λ����SYS����ʾ˳��ȡ���������ڵ�ϵͳ������avcodec_decode_audio2��������Ƶ�ĸ�ʽ��
	wanted_spec.channels = m_nb_channels;// audioCodecCtx->channels; 2----Ƶ������ƵƵ����������
	wanted_spec.silence = 0;//��Ĭ�����Ǳ�ʾ��Ĭ�ļ�ֵ��������Ƶ���з��ŵģ�0��Ȼ��ͨ����ֵ��
	wanted_spec.samples = m_out_nb_samples == 0 ? 1024 : m_out_nb_samples;
	//wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;   //1024 ����������SDL��Ҫ�������Ƶʱ�������ǵ���Ƶ�������Ĵ�С������ļ�ֵ��512��8192֮�䡣 ffplayʹ��1024��
	wanted_spec.callback = audio_callback;//���̲߳�����										  //wanted_spec.callback = audio_callback_ex;
	wanted_spec.userdata = m_pAudioCodecCtx;//�ص������ĵ�һ���Զ������
	if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
	{


		const char* ss = SDL_GetError();
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		return;
	}

	SDL_UnlockAudio();//��ʼ����
	SDL_PauseAudio(0);
}

void AudioDecPlay::audio_callback(void *userdata, Uint8 *stream, int len)
{
	static const int Max_audio_data_size = (MAX_AUDIO_FRAME_SIZE * 3) / 2;
	static uint8_t audio_buf[Max_audio_data_size];//������
	static unsigned int audio_buf_size = 0;//��������С
	static unsigned int audio_buf_index = 0;//��ǰӦ��ȡ�����������׵�ַ
	static AVFrame* pOriginalAudioFrame = av_frame_alloc();
	static AVFrame* pOutputAudioFrame = av_frame_alloc();

	AVCodecContext* pAudioCodecCtx = (AVCodecContext*)userdata;

	int len1;//���ڼ�¼ÿ��ȡ�������С
	int audio_data_size;//������󳤶�
	SDL_memset(stream, 0, len);

	AVPacket *packet = NULL;
	//ι���˲�����ѭ��^o^
	bool to_out = false;
	while (len > 0)
	{

		if (to_out)
		{
			break;
		}
		//������λ�ó������ʱ����ʾ����Ӧ�ø�����
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
						//��Ƶ��ʽת��(��Ƶ�ز���)  
						swr_convert(m_audio_convert_ctx,//��Ƶת��������  
							&m_pAudioOutBuffer,//�������  
							(m_out_nb_samples ? m_out_nb_samples : 1024),//MAX_AUDIO_FRAME_SIZE,//(m_out_nb_samples? m_out_nb_samples:1024),//ÿ�������С  //ÿ�������Ĵ�С
							(const uint8_t **)pOriginalAudioFrame->data,//��������  
							pOriginalAudioFrame->nb_samples);//����  

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
							break;//������仺��ѭ��
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
		/*  �鿴stream���ÿռ䣬����һ��copy�������ݣ�ʣ�µ��´μ���copy */
		len1 = audio_buf_size - audio_buf_index;//����ʣ���ȡ���ݴ�С
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
	//		swr_convert(m_audio_convert_ctx,//��Ƶת��������  
	//			&m_pAudioOutBuffer,//�������  
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
