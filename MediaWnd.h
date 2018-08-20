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
///由于我们建立的是C++的工程
///编译的时候使用的C++的编译器编译
///而FFMPEG是C的库
///因此这里需要加上extern "C"
///否则会提示各种未定义

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
	VideoDecodecThread* m_video_decodec;//视频解码器
	AudioDecPlay* m_audio_decodec;


private:
	void SetCentralImage(QImage image);
	void InitUiAndControl();
	void AddImage(QImage image);
	void GetOutImage(QImage &tempImage);

	//1表示 添加  2表示取出
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
