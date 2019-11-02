#include "packet_queue.h"

/* ��������ֶεĳ�ʼֵ��������mutex��cond */
//PacketQueue::PacketQueue(queue<pair<AVPacket, int>> q, int nb_pkts, int byte_size,
//	int64_t dur, bool abort_req, int seri, bool suc) : pq(q), nb_packets(nb_pkts), 
//	size(byte_size), duration(dur), abort_request(abort_req), serial(seri), success(suc)
//{
//	mutex = SDL_CreateMutex();
//	if (!mutex) 
//	{
//		av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
//		success = false;
//	}
//	cond = SDL_CreateCond();
//	if (!cond) 
//	{
//		av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
//		success = false;
//	}
//}

PacketQueue::PacketQueue(int nb_pkts, int byte_size,
	int64_t dur, bool abort_req, int seri, bool suc) : nb_packets(nb_pkts),
	size(byte_size), duration(dur), abort_request(abort_req), serial(seri), success(suc)
{
	mutex = SDL_CreateMutex();
	if (!mutex)
	{
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
		success = false;
	}
	cond = SDL_CreateCond();
	if (!cond)
	{
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
		success = false;
	}
}

/* ����ԭ��Ӧ��ͬʱ���忽�����캯���Ϳ�����ֵ���������������pqһ��ֻ��һ�������ݲ����� */
PacketQueue::~PacketQueue()
{
	packet_queue_flush();
	SDL_DestroyMutex(mutex);
	SDL_DestroyCond(cond);
}

void PacketQueue::packet_queue_init()
{
	mutex = SDL_CreateMutex();
	if (!mutex) 
	{
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
	}
	cond = SDL_CreateCond();
	if (!cond) 
	{
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
	}
	abort_request = 1; //��δ���ö���
	success = true;
}

/* ���캯���Ƿ�ɹ�ִ�� */
bool PacketQueue::is_construct()
{
	return success;
}

bool PacketQueue::is_abort()
{
	return abort_request;
}

int PacketQueue::get_serial()
{
	return serial;
}

int PacketQueue::get_nb_packets()
{
	return nb_packets;
}

int PacketQueue::get_size()
{
	return size;
}

int64_t PacketQueue::get_duration()
{
	return duration;
}

/* ������ʼ���ö��� */
void PacketQueue::packet_queue_start()
{
	SDL_LockMutex(mutex);
	abort_request = false; //�������Ҫ
	packet_queue_put_private(&flush_pkt); //������һ��flush_pkt����Ҫ������Ϊ���������������ݵġ��ֽ硱���
	SDL_UnlockMutex(mutex);
}

/* ��ֹ���У�ʵ����ֻ�ǽ���ֹ����Ϊ��1�������ź� */
void PacketQueue::packet_queue_abort()
{
	SDL_LockMutex(mutex);
	abort_request = 1;
	SDL_CondSignal(cond);  //�����ź�
	SDL_UnlockMutex(mutex);
}

/* ���PacketQueue�����нڵ��Լ�����ر�����ֵ���� */
//void PacketQueue::packet_queue_flush()
//{
//	SDL_LockMutex(mutex);
//	//not sure
//	while (!pq.empty())
//	{
//		av_packet_unref(&pq.back().first);
//		pq.pop();
//	}
//	nb_packets = 0;
//	size = 0;
//	duration = 0;
//	SDL_UnlockMutex(mutex);
//}

void PacketQueue::packet_queue_flush()
{
	MyAVPacketList *pkt, *pkt1;

	SDL_LockMutex(mutex);
	for (pkt = first_pkt; pkt; pkt = pkt1) 
	{
		pkt1 = pkt->next;
		av_packet_unref(&pkt->pkt);
		av_freep(&pkt);
	}
	last_pkt = NULL;
	first_pkt = NULL;
	nb_packets = 0;
	size = 0;
	duration = 0;
	SDL_UnlockMutex(mutex);
}

/* ��������������һ�£��������ⲿ���� */
void PacketQueue::packet_queue_destroy()
{
	packet_queue_flush();
	SDL_DestroyMutex(mutex);
	SDL_DestroyCond(cond);
}

/* ����յ�packet�������packet��ζ�����Ľ�����һ������Ƶ��ȡ��ɵ�ʱ������packet������ˢ���õ��������л�������֡ */
int PacketQueue::packet_queue_put_nullpacket(int stream_index)
{
	//�ȴ���һ���յ�packet��Ȼ�����packet_queue_put
	AVPacket pkt1, *pkt = &pkt1;
	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;
	pkt->stream_index = stream_index;
	return packet_queue_put(pkt);
}

/* ��packetд��PacketQueue */
int PacketQueue::packet_queue_put(AVPacket * pkt)
{
	int ret;

	SDL_LockMutex(mutex);
	ret = packet_queue_put_private(pkt); //����
	SDL_UnlockMutex(mutex);

	//д��ʧ����packet���ü�1������flush_pkt��ȫ�־�̬������һֱ��ʹ�ã��ʲ����ͷ�
	if (pkt != &flush_pkt && ret < 0)
		av_packet_unref(pkt);

	return ret;
}

/* ��PacketQueue�л�ȡpacket��
   block: �������Ƿ���Ҫ��û�ڵ��ȡ������������ȴ���
   ����ֵ����ֹ��<0����packet��=0����packet��> 0��
   AVPacket: �����������MyAVPacketList.pkt
   serial: �����������MyAVPacketList.serial */
//int PacketQueue::packet_queue_get(AVPacket * pkt, bool block, int * serial)
//{
//	int ret;
//
//	SDL_LockMutex(mutex);
//
//	while (true)
//	{
//		//��ֹʱ����ѭ��
//		if (abort_request)
//		{
//			ret = -1;
//			break;
//		}
//
//		//������������
//		if (!pq.empty())
//		{
//			//ȡ��packet������ǳ����
//			*pkt = pq.front().first;
//			//�����Ҫ���serial����serial���
//			if (serial)
//				*serial = pq.front().second;
//			pq.pop();	//����ԭ����packet
//
//			//����ͳ��������Ӧ����
//			nb_packets--;
//			size -= pkt->size + sizeof(AVPacket) + sizeof(int);
//			duration -= pkt->duration;
//
//			ret = 1;
//			break;
//		}
//		else if (!block) //������û�����ݣ��ҷ��������ã�������ѭ��
//		{
//			ret = 0;
//			break;
//		}
//		else //������û�����ݣ�����������
//		{
//			//����û��break����ѭ������һ���������ڻ������������(���������������)�ظ���������ȡ���ڵ�
//			SDL_CondWait(cond, mutex);
//		}
//	}
//	SDL_UnlockMutex(mutex);
//
//	return ret;
//}

int PacketQueue::packet_queue_get(AVPacket * pkt, bool block, int * serial)
{
	MyAVPacketList *pkt1;
	int ret;

	SDL_LockMutex(mutex);

	while (true) 
	{
		//��ֹʱ����ѭ��
		if (abort_request) 
		{
			ret = -1;
			break;
		}

		pkt1 = first_pkt;
		//������������
		if (pkt1) 
		{
			//�ڶ����ڵ��Ϊ��ͷ������ڶ����ڵ�Ϊ�գ����βΪ��
			first_pkt = pkt1->next;
			if (!first_pkt)
				last_pkt = NULL;

			//����ͳ��������Ӧ����
			nb_packets--;
			size -= pkt1->pkt.size + sizeof(*pkt1);
			duration -= pkt1->pkt.duration;

			//����AVPacket�����﷢��һ��AVPacket�ṹ�忽����AVPacket��dataֻ������ָ�룬��ΪAVPacket�Ƿ�����ջ�ڴ��ϵģ�����ʹ�õ��ںŽ���ǳ���������Բ����ͷŴӶ�����ȡ�������packet
			*pkt = pkt1->pkt;

			//�����Ҫ���serial����serial���
			if (serial)
				*serial = pkt1->serial;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else if (!block) 
		{//������û�����ݣ��ҷ��������ã�������ѭ��
			ret = 0;
			break;
		}
		else 
		{//������û�����ݣ�����������
			SDL_CondWait(cond, mutex);//����û��break��forѭ������һ���������ڻ������������(���������������)�ظ���������ȡ���ڵ�
		}
	}
	SDL_UnlockMutex(mutex);
	return ret;
}

/* ������ʵ��д��PacketQueue�ĺ��� */
//int PacketQueue::packet_queue_put_private(AVPacket * pkt)
//{
//	AVPacket pkt1;
//	int pkt_serial;
//
//	//�������ֹ�������ʧ��
//	if (abort_request)
//		return -1;
//
//	/*����AVPacket(ǳ������AVPacket.data���ڴ沢û�п�������pkt1��pkt��dataָ��ͬһ���ڴ棬Ҫ��סAVPacket�ṹֻ��������
//	  �����ǲ�����data���ڴ�ռ�ģ�������������û���������ü������������ݹ۲����ڴ沢������Ϊav_read_frame���ͷţ�����ԭ��)*/
//	pkt1 = *pkt;
//
//	//����������flush_pkt����Ҫ���Ӷ��е����кţ������ֲ���������������
//	if (pkt == &flush_pkt)
//		serial++;
//	//�ö������кű�ǽڵ�
//	pkt_serial = serial;
//
//	/* ��packet��� */
//	pq.push(make_pair(pkt1, pkt_serial));
//
//	/* �������Բ��������ӽڵ�����cache��С��cache��ʱ����
//	   ע��pkt1.size��ָ���ʵ�����ݵĴ�С�����Ի�Ҫ����AVPacket�����С */
//	nb_packets++;
//	size += pkt1.size + sizeof(AVPacket) + sizeof(int);
//	duration += pkt1.duration;
//
//	//�����źţ�������ǰ�������������ˣ�֪ͨ�ȴ��еĶ��߳̿���ȡ������
//	SDL_CondSignal(cond);
//	
//	return 0;
//}

int PacketQueue::packet_queue_put_private(AVPacket * pkt)
{
	MyAVPacketList *pkt1;

	//�������ֹ�������ʧ��
	if (abort_request)
		return -1;

	//����ڵ��ڴ�
	pkt1 = (MyAVPacketList*)av_malloc(sizeof(MyAVPacketList));
	//�����ڴ�ʧ��
	if (!pkt1)
		return -1;
	/*����AVPacket������ǳ������AVPacket.data���ڴ沢û�п�������pkt1->pkt��pkt��dataָ��ͬһ���ڴ棬Ҫ��סAVPacket�ṹֻ��������
	  �����ǲ�����data���ڴ�ռ�ģ�������������û���������ü������������ݹ۲����ڴ沢������Ϊav_read_frame���ͷţ�
	  ��Ϊav_read_frameû�е���av_packet_ref�����ǵ���av_init_packet�����ü�����ָ��ָ��NULL��ͬʱ���ںܶ�ʱ��AVPacket��
	  ֱ����ջ�Ϸ����ڴ�ģ����Բ���Ҫ�ֹ��ͷ�*/
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	//����������flush_pkt����Ҫ���Ӷ��е����кţ������ֲ���������������
	if (pkt == &flush_pkt)
		serial++;
	//�ö������кű�ǽڵ�
	pkt1->serial = serial;

	/*���в��������last_pktΪ�գ�˵�������ǿյģ������ڵ�Ϊ��ͷ�����򣬶��������ݣ�����ԭ��β��nextΪ�����ڵ㡣��󽫶�βָ�������ڵ�
	  ��Ϊq->first_pkt��q->last_pkt��pkt1ָ��ͬһ���ڴ棬����������ͷ�pkt1ָ����ڴ棡*/
	if (!last_pkt)
		first_pkt = pkt1;
	else
		last_pkt->next = pkt1;
	last_pkt = pkt1; //ֻ��һ��pktʱͷβ�ڵ�����ͬ��

	//�������Բ��������ӽڵ�����cache��С��cache��ʱ��
	nb_packets++;
	size += pkt1->pkt.size + sizeof(*pkt1);
	duration += pkt1->pkt.duration;
	/* XXX: should duplicate packet data in DV case */
	//�����źţ�������ǰ�������������ˣ�֪ͨ�ȴ��еĶ��߳̿���ȡ������
	SDL_CondSignal(cond);
	return 0;
}

