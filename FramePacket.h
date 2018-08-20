
#pragma once
#include <QtCore/QQueue>
#include <QtCore/QMutex>
extern "C"
{
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_types.h>
#include <SDL_name.h>
#include <SDL_main.h>
#include <SDL_config.h>
}

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

class FramePacket
{
public:
	FramePacket();
	~FramePacket();

	QQueue<AVPacket*> q;
	QMutex m_mutex;





	void packet_queue_init();
	//包入队
	int packet_queue_put(AVPacket *pkt);
	
	//包出队
	AVPacket* packet_queue_get();




	AVPacketList *first_pkt;
	AVPacketList *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
	
};

