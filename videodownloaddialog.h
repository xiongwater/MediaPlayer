#ifndef VIDEODOWNLOADDIALOG_H
#define VIDEODOWNLOADDIALOG_H
#include "ui_videodownloaddialog.h"
#include <QtWidgets/QDialog>

class QDateTime;
class videodownloaddialog : public QDialog
{
	Q_OBJECT

public:
	videodownloaddialog(QWidget *parent = 0);
	~videodownloaddialog();
	void SetBeginTime(const QDateTime& datatime);
	const QDateTime& GetBeginTime();
	void SetEndTime(const QDateTime& datatime);
	const QDateTime& GetEndTime();
	void SetFilePath(const QString&  path);
	const QString& GetFilePath();
	public slots:
		void slot_FileDialog();
signals:
		void sign_DownLoad();
private:
	QString m_currentFilePath;
	QDateTime m_BeginTime;
	QDateTime m_EndTime;
	//QFileDialog m_fileDialog;
//protected:
//	void paintEvent(QPaintEvent *);
private:
	Ui::videodownloaddialog ui;
};

#endif // VIDEODOWNLOADDIALOG_H
