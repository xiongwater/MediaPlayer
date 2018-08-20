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
//先退出线程
	msleep(500);
}

int VideoDecodecThread::InitDecodecForVideo(const AVFormatContext*& pFormatCtx)
{


	if (m_videoStream == -1)
	{
		//m_errInfo.append("unknown videoStream Failed");
		return -1;
	}
	m_pVideoCodecCtx = pFormatCtx->streams[m_videoStream]->codec;//解码器上下文环境
	m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
	if (m_pVideoCodec == NULL)
	{
		//m_errInfo.append(",m_pVideoCodec is null");
		return -1;
	}
	//打开视频解码器
	if (avcodec_open2(m_pVideoCodecCtx, m_pVideoCodec, NULL) < 0) {
	
		return -1;
	}
	m_pOriginalVideoFrame = av_frame_alloc();
	m_pOutputVideoFrame = av_frame_alloc();

	//======================video转码准备==============================================================//
	//设置转码器
	m_img_convert_ctx = sws_getContext(m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
		m_pVideoCodecCtx->pix_fmt, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
		m_OutputPixelFormat, SWS_BICUBIC, NULL, NULL, NULL);

	//(计算视频输出缓存)
	int numBytes = avpicture_get_size(m_OutputPixelFormat, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);

	//开辟（输出缓存）
	m_pVideoOutBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	//avpicture_fill函数将ptr指向的数据填充到picture内，但并没有拷贝，只是将picture结构内的data指针指向了ptr的数据。
	avpicture_fill((AVPicture *)m_pOutputVideoFrame, m_pVideoOutBuffer, m_OutputPixelFormat, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);

	//int y_size = m_pVideoCodecCtx->width * m_pVideoCodecCtx->height;
	//if (!packet)
	//{
	//	packet = (AVPacket *)malloc(sizeof(AVPacket)); //分配一个packet
	//	av_new_packet(packet, y_size); //分配packet的数据
	//}

	//av_dump_format(pFormatCtx, 0, m_strMiedaSourcePath.toLatin1().data(), 0); //输出视频信息
																			  //==================================================video转码准备==============================================================//
	//writeLog("视频转码准备", "无", "无");
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

	//基本上所有解码器解码之后得到的图像数据都是YUV420的格式，而这里我们需要将其保存成图片文件，因此需要将得到的YUV420数据转换成RGB格式，转换格式也是直接使用FFMPEG来完成：
	if (got_picture)
	{
		sws_scale(m_img_convert_ctx,//图片转码上下文 
			(uint8_t const * const *)m_pOriginalVideoFrame->data,//原始数据 
			m_pOriginalVideoFrame->linesize,//原始参数  
			0, //转码开始游标，一般为0  
			m_pVideoCodecCtx->height,//行数  
			m_pOutputVideoFrame->data,//转码后的数据  
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
		//先检测是否线程退出
		m_mutex_thread_finished.lock();
		if (m_thread_finished)
		{
			m_mutex_thread_finished.unlock();
			break;
		}
		m_mutex_thread_finished.unlock();

		//解码转码发送绘制信号
		VideoDecodecFrame();
	}
}

void VideoDecodecThread::slot_GetPacket(AVPacket *pkt)
{
	AppendAVPacket(pkt);
}
