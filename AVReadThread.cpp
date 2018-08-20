#include "AVReadThread.h"

AVReadThread::AVReadThread(QObject *parent)
	: QThread(parent)
{
	//初始化FFMPEG  调用了这个才能正常使用编码器和解码器
	av_register_all();
	avformat_network_init();//支持网络流  
							//分配一个AVFormatContext，FFMPEG[所有的操作]都要通过这个AVFormatContext来进行
	pFormatCtx = avformat_alloc_context();

}

AVReadThread::~AVReadThread()
{
	OperateRunFunStarted(true);
	//给一定的时间让线程退出，这里后续修改
	msleep(500);
}

void AVReadThread::Init()
{

	av_register_all();
	avformat_network_init();//支持网络流  
	pFormatCtx = avformat_alloc_context();
}

void AVReadThread::OpenSource()
{
	if (m_strMiedaSourcePath.isEmpty())
	{
		//资源路径为空
		return;
	}

	QByteArray byteArry = m_strMiedaSourcePath.toLatin1();
	char* charSource = byteArry.data();

	if (!pFormatCtx)
	{
		pFormatCtx = avformat_alloc_context();
	}
	//AVIOInterruptCB cb = { interrupt_callback, this };
	//pFormatCtx->interrupt_callback = cb;
	//主要是这个！！设置Opts
	AVDictionary* opts = NULL;
	av_dict_set(&opts, "stimeout", "10000000", 0);// 该函数是微秒 意思是10秒后没拉取到流就代表超时	 //avformat_open_input（）默认是阻塞的，用户可以通过设置“ic->flags |= AVFMT_FLAG_NONBLOCK; ”设置成非阻塞（通常是不推荐的）；或者是设置timeout设置超时时间；或者是设置interrupt_callback定义返回机制。

		if (avformat_open_input(&pFormatCtx, charSource, NULL, &opts) != 0) {
			//	m_nInitRet = 1;
			m_errInfo = " open media source Failed";
			return;
		}
	//根据打开的文件寻找其流信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		m_errInfo = "find stream info Failed";
		return;
	}

	m_videoStream = -1;
	m_audioStream = -1;

	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			m_videoStream = i;
		}
		else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			m_audioStream = i;
		}

	}

	signal_creat_audio_decodec(pFormatCtx, m_audioStream);
	signal_creat_video_decodec(pFormatCtx, m_videoStream);
}

void AVReadThread::UpdateFormatContext(const char* mieda_source)
{
	OperateRunFunStarted(true);//通知线程结束
	while (1)
	{
		if (IsRunEnd())
		{
			break;
		}
		msleep(30);
	}

	QString tempStrSource = mieda_source;
	if (tempStrSource.isEmpty())
	{
		m_errInfo = " mieda_source is empty";
		return;
	}

	m_strMiedaSourcePath = mieda_source;

	if (pFormatCtx)
	{
		avformat_close_input(&pFormatCtx);
	}
	OpenSource();
	start();
}

bool AVReadThread::IsRunEnd()
{
	mtx_endrun.lock();
	bool is_end = m_is_end_run;
	mtx_endrun.unlock();
	return is_end;
}

void AVReadThread::OperateRunFunStarted(bool isEnd)
{
	mtx_endrun.lock();
	m_is_end_run = isEnd;
	mtx_endrun.unlock();
}

void AVReadThread::run()
{

	//通知app创建视频解码器
	//通知app创建音频解码器

	msleep(300);
	OperateRunFunStarted(false);
	//packet = av_packet_alloc();//相当于c++里面封装的构造函数 new了一个出来。

	while (1)
	{
		packet = av_packet_alloc();//相当于c++里面封装的构造函数 new了一个出来。
	
		if (IsRunEnd())
		{
			break;
		}

	    msleep(10);

		if (av_read_frame(pFormatCtx, packet) < 0)
		{
			//break; 
			continue;
		}

		int got_picture = 0;
		int got_frame = 0;

		if (packet->stream_index == m_videoStream)
		{

			emit signal_send_video_packet(packet);
		}
		else if (packet->stream_index == m_audioStream)
		{

			emit signal_send_audio_packet(packet);
			//msleep(30);

		}
		else
		{
			av_free_packet(packet);//清空里面填充的数据
		}

	}
}
