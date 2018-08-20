#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include "ui_FFMPEG_LEARN.h"
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include "DecodecThread.h"
#include "videodownloaddialog.h"
#include "AudioPlayer.h"
#include"QAudioPlayer.h"


#include "AVReadThread.h"
#include "VideoDecodecThread.h"
#include <AudioDecPlay.h>
///�������ǽ�������C++�Ĺ���
///�����ʱ��ʹ�õ�C++�ı���������
///��FFMPEG��C�Ŀ�
///���������Ҫ����extern "C"
///�������ʾ����δ����

#include<iostream>
using namespace std;


class MediaWnd : public QMainWindow
{
	Q_OBJECT

public:
	MediaWnd(QWidget *parent = Q_NULLPTR);
	~MediaWnd();

private:
	Ui::FFMPEG_LEARNClass ui;

	QTimer m_timer;
	QWidget* m_CentralWidget;
	QLabel* m_CentralCanvas;
	QQueue<QImage> m_ImagesQueue;
	QMutex m_mutext;
	DecodecThread* m_decodecThread;
	videodownloaddialog* m_SourceChooseDialog;
	AudioPlayer* m_AudioPlayer;
	QAudioPlayer* m_QAudioPlayer;

	AVReadThread* m_AVReadThread;
	VideoDecodecThread* m_video_decodec;//��Ƶ������
	AudioDecPlay* m_audio_decodec;


private:
	void SetCentralImage(QImage image);
	void InitUiAndControl();
	void AddImage(QImage image);
	void GetOutImage(QImage &tempImage);

	//1��ʾ ���  2��ʾȡ��
	void OperateImageQueue(int SaveOrGetOut, QImage& image);
	public slots:
	void slot_ShowFlag();
	void slot_GotOneDecodecPicture(QImage srcImage, const QString strID);
	void slot_GotOneDecodecPicture(QImage srcImage);
//	void start();
//	void stop();
	void slot_UpdatMediaSource();

	void slot_creat_video_decodec(AVFormatContext *pFormatCtx, int stream);
	void slot_creat_audio_decodec(AVFormatContext *pFormatCtx, int stream);

};
