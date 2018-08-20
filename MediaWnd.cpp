#include "MediaWnd.h"
#include <QtWidgets/QAction>
#include <QtWidgets/QHBoxLayout>


MediaWnd::MediaWnd(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_SourceChooseDialog = new videodownloaddialog(this);
	m_CentralCanvas = new QLabel(this);
	m_CentralWidget = new QWidget(this);
	m_CentralWidget->setStyleSheet("background:blue");
	this->setCentralWidget(m_CentralWidget);
	QHBoxLayout* lay = new QHBoxLayout(this);
	lay->addWidget(m_CentralCanvas);
	m_CentralWidget->setLayout(lay);
	InitUiAndControl();
	/*m_timer.setInterval(30);
	m_decodecThread = new DecodecThread(this);
	QObject::connect(&m_timer, SIGNAL(timeout()), this, SLOT(slot_ShowFlag()));
	QObject::connect(m_decodecThread, SIGNAL(sig_DecodecOnePicture(QImage, const QString )), this, SLOT(slot_GotOneDecodecPicture(QImage, const QString)));

	m_AudioPlayer = new AudioPlayer(this);
	QObject::connect(m_decodecThread, SIGNAL(sig_DecodecOneAudio(uint8_t* , int , const QString)), m_AudioPlayer, SLOT(slot_GetOneAudioBuffer(uint8_t*,int, const QString )));
	QObject::connect(m_decodecThread, SIGNAL(sig_out_audio_param(int,int,int)), m_AudioPlayer, SLOT(slot_InitSDL_Audio(int,int,int)));
*/
	/*m_QAudioPlayer = new QAudioPlayer(this);
	QObject::connect(m_decodecThread, SIGNAL(sig_DecodecOneAudio(uint8_t*, int, const QString)), m_QAudioPlayer, SLOT(slot_GetOneAudioBuffer(uint8_t*, int, const QString)));
	QObject::connect(m_decodecThread, SIGNAL(sig_out_audio_param(int, int, int)), m_QAudioPlayer, SLOT(slot_Init_QAudio(int, int, int)));
	m_QAudioPlayer->hide();*/

	m_AVReadThread = new AVReadThread(this);
	m_video_decodec = NULL;
	m_audio_decodec = NULL;
	QObject::connect(m_AVReadThread, SIGNAL(signal_creat_video_decodec(AVFormatContext*, int)), this, SLOT(slot_creat_video_decodec(AVFormatContext*, int)));
	QObject::connect(m_AVReadThread, SIGNAL(signal_creat_audio_decodec(AVFormatContext*, int)), this, SLOT(slot_creat_audio_decodec(AVFormatContext*, int)));


}

MediaWnd::~MediaWnd()
{
	//m_decodecThread->UpdateFormatContext("");
}

void MediaWnd::SetCentralImage(QImage image)
{
	QImage adjustSizeImage = image.scaled(m_CentralCanvas->size(),Qt::IgnoreAspectRatio);
	m_CentralCanvas->setPixmap(QPixmap::fromImage(adjustSizeImage));
}

void MediaWnd::InitUiAndControl()
{
	QAction* start = new QAction("START", this);
	QAction* stop = new QAction("STOP", this);
	QAction* chooseSource = new QAction("ChooseSource", this);
	this->menuBar()->addAction(start);
	this->menuBar()->addAction(stop);
	this->menuBar()->addAction(chooseSource);
	connect(start, SIGNAL(triggered()), this, SLOT(start()));
	connect(stop, SIGNAL(triggered()), this, SLOT(stop()));
	connect(chooseSource, SIGNAL(triggered()), m_SourceChooseDialog, SLOT(show()));
	connect(m_SourceChooseDialog, SIGNAL(sign_DownLoad()), this, SLOT(slot_UpdatMediaSource()));
}

void MediaWnd::AddImage(QImage image)
{
	QImage pImage;
	pImage = image.copy();
	if (!pImage.isNull())
	{
		m_ImagesQueue.enqueue(pImage);
	}
	
}

void MediaWnd::GetOutImage(QImage& tempImage)
{
	if (m_ImagesQueue.count())
	{
		QImage pImage = m_ImagesQueue.dequeue();
		if (!pImage.isNull())
		{
			tempImage = pImage.copy();
			//SetCentralImage(tempImage);
		}
		
	}	
}

void MediaWnd::OperateImageQueue(int SaveOrGetOut, QImage& image)
{
	QMutexLocker locker(&m_mutext);
	switch (SaveOrGetOut)
	{
	case 1:
		AddImage(image);
		break;
	case 2:
		GetOutImage(image);
		break;
	case 3:
		if (m_ImagesQueue.count())
		{
			m_ImagesQueue.clear();
		}
		break;
	default:
		break;
	}
}

void MediaWnd::slot_ShowFlag()
{
	QImage image;
	OperateImageQueue(2, image);
	if (!image.isNull())
	{
		SetCentralImage(image);
	}
		
}

void MediaWnd::slot_GotOneDecodecPicture(QImage srcImage, const QString strID)
{
	
	SetCentralImage(srcImage);

	/*if (m_ImagesQueue.count() > 4000)
	{
		return;
	}	
	OperateImageQueue(1, srcImage);*/
}

void MediaWnd::slot_GotOneDecodecPicture(QImage srcImage)
{
	SetCentralImage(srcImage);
}

//void MediaWnd::start()
//{
//	//m_timer.start();
//	m_AudioPlayer->Resume();
//}

//void MediaWnd::stop()
//{
//	//m_decodecThread->UpdateFormatContext("");
//
//	m_timer.stop();
//	m_AudioPlayer->Pause();
//}

void MediaWnd::slot_UpdatMediaSource()
{
	m_timer.stop();
	QString newSource = m_SourceChooseDialog->GetFilePath();
	QByteArray ba = newSource.toLatin1();
	QImage image;
	OperateImageQueue(3, image);

	m_AVReadThread->UpdateFormatContext(ba.data());
	//m_decodecThread->UpdateFormatContext(ba.data());
	//m_timer.start();
	m_SourceChooseDialog->setVisible(false);
}

void MediaWnd::slot_creat_video_decodec(AVFormatContext *pFormatCtx, int stream)
{
	if (m_video_decodec)
	{
		//这里先释放自己线程

	}
	m_video_decodec = new VideoDecodecThread(stream, this, pFormatCtx);
	connect(m_AVReadThread, SIGNAL(signal_send_video_packet(AVPacket*)), m_video_decodec, SLOT(slot_GetPacket(AVPacket*)));
	connect(m_video_decodec, SIGNAL(signal_ShowImage(QImage)), this, SLOT(slot_GotOneDecodecPicture(QImage)));
	m_video_decodec->start();
}

void MediaWnd::slot_creat_audio_decodec(AVFormatContext *pFormatCtx, int stream)
{
	
	m_audio_decodec = new AudioDecPlay(stream, pFormatCtx,this);

	connect(m_AVReadThread, SIGNAL(signal_send_audio_packet(AVPacket*)), m_audio_decodec, SLOT(slot_GetPacket(AVPacket*)));

	m_audio_decodec->startPlay();
}