#ifndef DECODER_H_
#define DECODER_H_
#include "packet_queue.h"
#include "frame_queue.h"

class Decoder
{
public:
	Decoder() = default;
	Decoder(AVCodecContext *ctx, PacketQueue *pq, SDL_cond *empty_q_cond,
		int64_t st_pts = AV_NOPTS_VALUE, int serial = -1) :avctx(ctx), pkt_queue(pq),
		empty_queue_cond(empty_q_cond), start_pts(st_pts), pkt_serial(serial) {}
	~Decoder();
	int decoder_start(int(*fn)(void *), void *arg);
	void decoder_abort(FrameQueue *fq);
	void decoder_init(AVCodecContext *ctx, PacketQueue *pq, SDL_cond *empty_q_cond,
		int64_t st_pts = AV_NOPTS_VALUE, int serial = -1);
	void decoder_destroy();
	int decoder_decode_frame(AVFrame *frame, AVSubtitle *sub);
	int is_finished();
	int get_pkt_serial();
	AVCodecContext* get_avctx();
	void set_start_pts(int64_t str_pts);
	void set_start_pts_tb(AVRational str_pts_tb);

private:
	AVPacket pkt;               //�����ݴ淢��ʧ�ܵ�pkt
	PacketQueue *pkt_queue;
	AVCodecContext *avctx;
	int pkt_serial;             //��ǰ���ڽ����pkt�����к�
	int finished;               // �Ƿ��Ѿ�����
	int packet_pending;         // �Ƿ��а��ڵȴ��ط�
	SDL_cond *empty_queue_cond; // ����Ϊ�յ���������
	int64_t start_pts;          // ��ʼ��ʱ���
	AVRational start_pts_tb;    // ��ʼ��ʱ�����ʱ��
	int64_t next_pts;           // ��һ֡ʱ���
	AVRational next_pts_tb;     // ��һ֡ʱ�����ʱ��
	SDL_Thread *decoder_tid;    // �����߳�
};



#endif // !DECODER_H_

