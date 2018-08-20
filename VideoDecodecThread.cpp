#include "VideoDecodecThread.h"

VideoDecodecThread::VideoDecodecThread(int video_stream, QObject *parent, const AVFormatContext* pFormatCtx)
	: QThread(parent)
	, m_videoStream(video_stream)
{
	m_OutputPixelFormat = AV_PIX_FMT_RGB32;
	m_thread_finished = true;
	InitRet = InitDecodecForVideo(pFormatCtx);
}

VideoDecodecThread::~VideoDecodecThread()
{

	m_mutex_thread_finished.lock();
	m_thread_finished = true;
	m_mutex_thread_finished.unlock();
//���˳��߳�
	msleep(500);
}

int VideoDecodecThread::InitDecodecForVideo(const AVFormatContext*& pFormatCtx)
{


	if (m_videoStream == -1)
	{
		//m_errInfo.append("unknown videoStream Failed");
		return -1;
	}
	m_pVideoCodecCtx = pFormatCtx->streams[m_videoStream]->codec;//�����������Ļ���
	m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
	if (m_pVideoCodec == NULL)
	{
		//m_errInfo.append(",m_pVideoCodec is null");
		return -1;
	}
	//����Ƶ������
	if (avcodec_open2(m_pVideoCodecCtx, m_pVideoCodec, NULL) < 0) {
	
		return -1;
	}
	m_pOriginalVideoFrame = av_frame_alloc();
	m_pOutputVideoFrame = av_frame_alloc();

	//======================videoת��׼��==============================================================//
	//����ת����
	m_img_convert_ctx = sws_getContext(m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
		m_pVideoCodecCtx->pix_fmt, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
		m_OutputPixelFormat, SWS_BICUBIC, NULL, NULL, NULL);

	//(������Ƶ�������)
	int numBytes = avpicture_get_size(m_OutputPixelFormat, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);

	//���٣�������棩
	m_pVideoOutBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	//avpicture_fill������ptrָ���������䵽picture�ڣ�����û�п�����ֻ�ǽ�picture�ṹ�ڵ�dataָ��ָ����ptr�����ݡ�
	avpicture_fill((AVPicture *)m_pOutputVideoFrame, m_pVideoOutBuffer, m_OutputPixelFormat, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);

	//int y_size = m_pVideoCodecCtx->width * m_pVideoCodecCtx->height;
	//if (!packet)
	//{
	//	packet = (AVPacket *)malloc(sizeof(AVPacket)); //����һ��packet
	//	av_new_packet(packet, y_size); //����packet������
	//}

	//av_dump_format(pFormatCtx, 0, m_strMiedaSourcePath.toLatin1().data(), 0); //�����Ƶ��Ϣ
																			  //==================================================videoת��׼��==============================================================//
	//writeLog("��Ƶת��׼��", "��", "��");
	return 0;
}

void VideoDecodecThread::AppendAVPacket(AVPacket *pkt)
{
	m_frame_packet.packet_queue_put(pkt);
}

void VideoDecodecThread::GetAVPacket(AVPacket* &pkt)
{
	pkt=m_frame_packet.packet_queue_get();
}

void VideoDecodecThread::VideoDecodecFrame()
{
	AVPacket *packet = NULL;	
	GetAVPacket(packet);
	if (!packet)
	{
		return;
	}
	int got_picture = 0;
	int ret = avcodec_decode_video2(m_pVideoCodecCtx, m_pOriginalVideoFrame, &got_picture, packet);
	if (ret < 0) {
		return;
	}

	//���������н���������֮��õ���ͼ�����ݶ���YUV420�ĸ�ʽ��������������Ҫ���䱣���ͼƬ�ļ��������Ҫ���õ���YUV420����ת����RGB��ʽ��ת����ʽҲ��ֱ��ʹ��FFMPEG����ɣ�
	if (got_picture)
	{
		sws_scale(m_img_convert_ctx,//ͼƬת�������� 
			(uint8_t const * const *)m_pOriginalVideoFrame->data,//ԭʼ���� 
			m_pOriginalVideoFrame->linesize,//ԭʼ����  
			0, //ת�뿪ʼ�α꣬һ��Ϊ0  
			m_pVideoCodecCtx->height,//����  
			m_pOutputVideoFrame->data,//ת��������  
			m_pOutputVideoFrame->linesize);
		QImage image(m_pOutputVideoFrame->data[0], m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, QImage::Format_RGB32);
		QImage CopyImage = image.copy();
		signal_ShowImage(CopyImage);
	}
	av_free(packet);
	msleep(33);
}

void VideoDecodecThread::run()
{


	m_mutex_thread_finished.lock();
	m_thread_finished = false;
	m_mutex_thread_finished.unlock();
	
	while (1)
	{
		//�ȼ���Ƿ��߳��˳�
		m_mutex_thread_finished.lock();
		if (m_thread_finished)
		{
			m_mutex_thread_finished.unlock();
			break;
		}
		m_mutex_thread_finished.unlock();

		//����ת�뷢�ͻ����ź�
		VideoDecodecFrame();
	}
}

void VideoDecodecThread::slot_GetPacket(AVPacket *pkt)
{
	AppendAVPacket(pkt);
}
