#include "AVReadThread.h"

AVReadThread::AVReadThread(QObject *parent)
	: QThread(parent)
{
	//��ʼ��FFMPEG  �����������������ʹ�ñ������ͽ�����
	av_register_all();
	avformat_network_init();//֧��������  
							//����һ��AVFormatContext��FFMPEG[���еĲ���]��Ҫͨ�����AVFormatContext������
	pFormatCtx = avformat_alloc_context();

}

AVReadThread::~AVReadThread()
{
	OperateRunFunStarted(true);
	//��һ����ʱ�����߳��˳�����������޸�
	msleep(500);
}

void AVReadThread::Init()
{

	av_register_all();
	avformat_network_init();//֧��������  
	pFormatCtx = avformat_alloc_context();
}

void AVReadThread::OpenSource()
{
	if (m_strMiedaSourcePath.isEmpty())
	{
		//��Դ·��Ϊ��
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
	//��Ҫ�������������Opts
	AVDictionary* opts = NULL;
	av_dict_set(&opts, "stimeout", "10000000", 0);// �ú�����΢�� ��˼��10���û��ȡ�����ʹ���ʱ	 //avformat_open_input����Ĭ���������ģ��û�����ͨ�����á�ic->flags |= AVFMT_FLAG_NONBLOCK; �����óɷ�������ͨ���ǲ��Ƽ��ģ�������������timeout���ó�ʱʱ�䣻����������interrupt_callback���巵�ػ��ơ�

		if (avformat_open_input(&pFormatCtx, charSource, NULL, &opts) != 0) {
			//	m_nInitRet = 1;
			m_errInfo = " open media source Failed";
			return;
		}
	//���ݴ򿪵��ļ�Ѱ��������Ϣ
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
	OperateRunFunStarted(true);//֪ͨ�߳̽���
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

	//֪ͨapp������Ƶ������
	//֪ͨapp������Ƶ������

	msleep(300);
	OperateRunFunStarted(false);
	//packet = av_packet_alloc();//�൱��c++�����װ�Ĺ��캯�� new��һ��������

	while (1)
	{
		packet = av_packet_alloc();//�൱��c++�����װ�Ĺ��캯�� new��һ��������
	
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
			av_free_packet(packet);//���������������
		}

	}
}
