#pragma once

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtGui/QImage>
#include <QtCore/QQueue>

#include "FramePacket.h"

class VideoDecodecThread : public QThread
{
	Q_OBJECT

public:
	VideoDecodecThread(int video_stream,QObject *parent,const AVFormatContext* pFormatCtx = NULL);
	~VideoDecodecThread();

	int InitDecodecForVideo( const AVFormatContext*&  pFormatCtx);
	void AppendAVPacket(AVPacket *pkt);
	void GetAVPacket(AVPacket *&pkt);
	void VideoDecodecFrame();
protected:
	virtual void run();//线程执行
private:
	AVCodecContext *m_pVideoCodecCtx;//视频解码器上下文环境
	AVCodec *m_pVideoCodec;//视频解码器
	uint8_t *m_pVideoOutBuffer;//视频
	int m_video_out_buffer_size;//输出缓存大小
	AVFrame*  m_pOriginalVideoFrame;//存储解码出的原始图像帧
	AVFrame* m_pOutputVideoFrame;
	struct SwsContext *m_img_convert_ctx;//(图像转码器)
	int m_videoFPS;//Frames per Second			  
	enum AVPixelFormat m_OutputPixelFormat; //video format param
	int m_videoStream;//视频流类型索引
	QMutex m_QueMutex;
	FramePacket m_frame_packet;

	int InitRet;//初始化结果

	bool m_thread_finished;
	QMutex m_mutex_thread_finished;
signals:
	void signal_ShowImage(QImage SourceImage);
	public slots:
	void slot_GetPacket(AVPacket *pkt);
};
