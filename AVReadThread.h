#pragma once

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtGui/QImage>
#include <QtCore/QQueue>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"  
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"    
}

class AVReadThread : public QThread
{
	Q_OBJECT

public:
	AVReadThread(QObject *parent);
	~AVReadThread();

	void Init();//初始化环境
	void OpenSource();//打开资源并发出创建相应的解码转码器信号
	void UpdateFormatContext(const char* mieda_source);

	bool IsRunEnd();
	void OperateRunFunStarted(bool isEnd);



	QString m_strMiedaSourcePath;//Media Source Path
	QString m_errInfo;
	AVFormatContext *pFormatCtx;//Media Context
	AVPacket *packet;//解码前数据包
	int m_videoStream ;
	int m_audioStream ;

	bool m_is_end_run;
	QMutex mtx_endrun;

protected:
	virtual void run();

signals:
	void signal_creat_video_decodec(AVFormatContext *pFormatCtx, int stream);
	void signal_creat_audio_decodec(AVFormatContext *pFormatCtx, int stream);
	void signal_send_audio_packet(AVPacket* packet);
	void signal_send_video_packet(AVPacket* packet);

};
