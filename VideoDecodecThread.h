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
	virtual void run();//�߳�ִ��
private:
	AVCodecContext *m_pVideoCodecCtx;//��Ƶ�����������Ļ���
	AVCodec *m_pVideoCodec;//��Ƶ������
	uint8_t *m_pVideoOutBuffer;//��Ƶ
	int m_video_out_buffer_size;//��������С
	AVFrame*  m_pOriginalVideoFrame;//�洢�������ԭʼͼ��֡
	AVFrame* m_pOutputVideoFrame;
	struct SwsContext *m_img_convert_ctx;//(ͼ��ת����)
	int m_videoFPS;//Frames per Second			  
	enum AVPixelFormat m_OutputPixelFormat; //video format param
	int m_videoStream;//��Ƶ����������
	QMutex m_QueMutex;
	FramePacket m_frame_packet;

	int InitRet;//��ʼ�����

	bool m_thread_finished;
	QMutex m_mutex_thread_finished;
signals:
	void signal_ShowImage(QImage SourceImage);
	public slots:
	void slot_GetPacket(AVPacket *pkt);
};
