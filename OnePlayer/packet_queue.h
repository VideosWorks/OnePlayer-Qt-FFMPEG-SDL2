#ifndef PACKET_QUEUE_H_
#define PACKET_QUEUE_H_
#include <iostream>
#include <queue>
#include <utility>

#include "global.h"

using std::cout;
using std::endl;
using std::queue;
using std::pair;
using std::make_pair;

//PacketQueue��һ���ڵ�
typedef struct MyAVPacketList
{
	AVPacket pkt;
	struct MyAVPacketList *next;    //��һ���ڵ�
	int serial;                     //���кţ����ڱ�ǵ�ǰ�ڵ�����кţ������Ƿ��������ݣ���seek��ʱ�򷢻�����
} MyAVPacketList;

class PacketQueue
{
public:
	PacketQueue() = default;
	/*PacketQueue(queue<pair<AVPacket, int>> q, int nb_pkts = 0, int byte_size = 0,
		int64_t dur = 0, bool abort_req = true, int seri = 0, bool suc = true);*/
	PacketQueue(int nb_pkts = 0, int byte_size = 0,
		int64_t dur = 0, bool abort_req = true, int seri = 0, bool suc = true);
	PacketQueue(PacketQueue&&) = default;
	PacketQueue&operator=(PacketQueue&&) = default;
	~PacketQueue();
	void packet_queue_init();
	void packet_queue_start();
	void packet_queue_abort();
	void packet_queue_flush();
	void packet_queue_destroy();
	int packet_queue_put_nullpacket(int stream_index);
	int packet_queue_put(AVPacket *pkt);
	int packet_queue_get(AVPacket *pkt, bool block, int *serial);
	bool is_construct();
	bool is_abort();
	int get_serial();
	int get_nb_packets();
	int get_size();
	int64_t get_duration();


private:
	//queue<pair<AVPacket, int>> pq;	//pairģ��MyAVPacketList������packet��serial
	MyAVPacketList *first_pkt, *last_pkt;   //���ף���β
	int nb_packets;                 //�ڵ���
	int size;                       //�������нڵ��ֽ����������ڼ���cache��С
	int64_t duration;               //�������нڵ�ĺϼ�ʱ��
	bool abort_request;             //�Ƿ�Ҫ��ֹ���в��������ڰ�ȫ�����˳�����
	int serial;                     //���кţ���MyAVPacketList��serial������ͬ�������Ե�������һ��PacketQueueֻ��һ��serial����������Ķ��packet�ֱ����Լ���serial������������������serial������packet������
	SDL_mutex *mutex;               //����ά��PacketQueue�Ķ��̰߳�ȫ
	SDL_cond *cond;                 //���ڶ���д�߳�ͨ��
	int success;					//�����Ƿ�ɹ��ı�־

	int packet_queue_put_private(AVPacket *pkt);


};



#endif // !PACKET_QUEUE_H_
