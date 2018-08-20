#pragma once

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include "FramePacket.h"
#include "log.h"


#define MAX_AUDIO_FRAME_SIZE 192000 // 

class AudioDecPlay : public QThread
{
	Q_OBJECT

public:
	AudioDecPlay(int video_stream, const AVFormatContext* pFormatCtx,QObject *parent = NULL);
	~AudioDecPlay();


	int InitDecodec(const AVFormatContext*&  pFormatCtx);

    void startPlay();

	void pause()
	{
		SDL_PauseAudio(1);// 停止callback()。
	}

	void resum()
	{
		SDL_PauseAudio(0);// 停止callback()。
	}

	static void  audio_callback(void *userdata, Uint8 *stream, int len);

	void AppendAVPacket(AVPacket *pkt)
	{
		m_frame_packet.packet_queue_put(pkt);
	}
	bool GetAVPacket(AVPacket *&pkt)
	{
		return  pkt=m_frame_packet.packet_queue_get();
	}

protected:
	public slots:
	void slot_GetPacket(AVPacket *pkt);
private:
	
	QString m_errInfo;
	AVCodecContext* m_pAudioCodecCtx;//音频解码器上下文环境
	AVCodec* m_pAudioCodec;//音频解码器
	static uint8_t* m_pAudioOutBuffer;//音频
	int m_audio_out_buffer_size;
	AVFrame* m_pOriginalAudioFrame;
	AVFrame* m_pOutputAudioFrame;
	static struct SwrContext *m_audio_convert_ctx;//(音频转码器)
	static int m_nb_channels;//音频声道数量  
	static enum AVSampleFormat m_out_sample_fmt;//输出的采样格式 16bit PCM  
	int m_out_sample_rate;//输出采样率  
	uint64_t m_out_ch_layout;//输出的声道布局：立体声  
	static int m_out_nb_samples;
	int m_nAudioInitRet;
	int m_audioStream;
	QMutex m_QueMutex;
	static FramePacket m_frame_packet;

	int InitRet;//初始化结果

	bool m_thread_finished;
	QMutex m_mutex_thread_finished;
	SDL_AudioSpec spec;
	SDL_AudioSpec wanted_spec;

	



};
