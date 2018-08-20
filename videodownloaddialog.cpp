#include "videodownloaddialog.h"
#include<QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtWidgets/QFileDialog>
videodownloaddialog::videodownloaddialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	m_BeginTime=QDateTime::currentDateTime();
	m_EndTime=m_BeginTime;
	SetBeginTime(m_BeginTime);
	SetEndTime(m_EndTime);
	connect(ui.tbOk,SIGNAL(clicked()),this,SIGNAL(sign_DownLoad()));
	connect(ui.tbCancel,SIGNAL(clicked()),this,SLOT(hide()));
	connect(ui.btnscan,SIGNAL(clicked()),this,SLOT(slot_FileDialog()));
	this->setWindowTitle(QString::fromLocal8Bit("ѡ����Ƶ��Դ"));

}

videodownloaddialog::~videodownloaddialog()
{

}
void videodownloaddialog::SetBeginTime( const QDateTime& datatime )
{
	ui.BeginTimeEdit->setDateTime(datatime);
}

const QDateTime& videodownloaddialog::GetBeginTime()
{
	m_BeginTime=ui.BeginTimeEdit->dateTime();
	return m_BeginTime;
}

void videodownloaddialog::SetEndTime( const QDateTime& datatime )
{
	ui.EndTimeEdit->setDateTime(datatime);
}

const QDateTime& videodownloaddialog::GetEndTime()
{
	m_EndTime=ui.EndTimeEdit->dateTime();
	return m_EndTime;
}

void videodownloaddialog::SetFilePath( const QString& path )
{
	ui.pathLineEdit->setText(path);	
}


const QString& videodownloaddialog::GetFilePath()
{
	m_currentFilePath= ui.pathLineEdit->text();
	return m_currentFilePath;
}


void videodownloaddialog::slot_FileDialog()
{
	m_currentFilePath=GetFilePath();
	if(m_currentFilePath.isEmpty())
	{
		m_currentFilePath=QApplication::applicationFilePath();
	}

	QString filepath=QFileDialog::getSaveFileName(
		this,//�Ի��򸸶���  
		tr("ѡ��·��"),  
		m_currentFilePath, //Ĭ�ϴ�·�� 
		//tr("Images (*.png *.bmp *.jpg *.tif *.GIF)")); //ѡ��·��  
		0,//����
		0//ѡ��
		);
	if(!filepath.isEmpty())
	{
		SetFilePath(filepath);
	}

}