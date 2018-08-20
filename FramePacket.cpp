#include "FramePacket.h"



FramePacket::FramePacket()
{
	packet_queue_init();

}


FramePacket::~FramePacket()
{
}

void FramePacket::packet_queue_init()
{
}

int FramePacket::packet_queue_put(AVPacket *pkt)
{
	m_mutex.lock();
	q.enqueue(pkt);
	m_mutex.unlock();
	return 0;
}

AVPacket* FramePacket::packet_queue_get()
{

	m_mutex.lock();
	if (q.count() > 0)
	{
		AVPacket* pkt1 = q.dequeue();
		m_mutex.unlock();
		return pkt1;
	}
	else
	{
		m_mutex.unlock();
		return NULL;
	}
	
}
