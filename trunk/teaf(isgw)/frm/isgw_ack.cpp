#include "isgw_ack.h"
#include "ace_sock_hdr_base.h"
#include "stat.h"
#include "isgw_uintf.h"
#include "../comm/pp_prot.h"

ISGWAck* ISGWAck::instance_ = NULL;
struct timeval ISGWAck::time_ = {0,0};
ISGWAck* ISGWAck::instance()
{
    if (instance_ == NULL)
    {
        instance_ = new ISGWAck();
    }
    return instance_;
}

int ISGWAck::init(int tv)
{

#ifdef ISGW_ACK_USE_TIMER
    if (tv < DEFAULT_TIME_INTERVAL) //��С����С��1 ms 
    {
        tv = DEFAULT_TIME_INTERVAL;
    }
    ACE_Time_Value delay(0,0);
    ACE_Time_Value interval(0, tv);
    ACE_Reactor::instance()->schedule_timer(this, 0, delay, interval);
    ACE_DEBUG((LM_INFO, "[%D] ISGWAck init timer succ,tv=%d\n", tv));
#endif
    ACE_DEBUG((LM_INFO, "[%D] ISGWAck init succ,my lock=0x%x\n", &(queue_lock_.lock())));
    return 0;
}

void ISGWAck::putq(PPMsg* ack_msg)
{
    if (ack_msg == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWAck putq failed,ack_msg is null\n"));
        return ;
    }
    
    {
        ACE_Guard<ACE_Thread_Mutex> guard(queue_lock_);
        msg_queue_.push_back(ack_msg);
    }
    
#ifdef ISGW_ACK_USE_TIMER 
    
#else
    notify_stgy_.notify(); //6.0 ���ϰ汾����û��bug��
#endif

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    unsigned time_inq = EASY_UTIL::get_span(&ack_msg->tv_time, &ack_msg->p_start); //�������Ϣ�������ʱ��
    unsigned time_diff = EASY_UTIL::get_span(&ack_msg->tv_time, &tv_now); 
    // �������涨��ʱ�� �����ͳ�� ������ 
    if(time_diff>ALARM_TIMEOUT*10000)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWAck put queue failed,too late,sock_fd=%u,prot=%u,ip=%u"
            ",port=%u,sock_seq=%u,seq_no=%u,time_inq=%u,time_diff=%u,que_size=%d,cmd=%d,msg=%s\n"
            ,ack_msg->sock_fd
            ,ack_msg->protocol
            ,ack_msg->sock_ip
            ,ack_msg->sock_port
            ,ack_msg->sock_seq
            ,ack_msg->seq_no
            ,time_inq
            ,time_diff
            ,msg_queue_.size()
            ,ack_msg->cmd
            ,ack_msg->msg
            ));
        Stat::instance()->incre_stat(STAT_CODE_PUT_ACK_TOOLATE);
    }
    
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWAck putq succ,sock_fd=%u,prot=%u,ip=%u"
        ",port=%u,sock_seq=%u,seq_no=%u,time_inq=%u,time_diff=%u,que_size=%d,cmd=%d\n"
        ,ack_msg->sock_fd
        ,ack_msg->protocol
        ,ack_msg->sock_ip
        ,ack_msg->sock_port
        ,ack_msg->sock_seq
        ,ack_msg->seq_no
        ,time_inq
        ,time_diff
        ,msg_queue_.size()
        ,ack_msg->cmd
    	));
}

unsigned int ISGWAck::get_time()
{
#ifdef ISGW_ACK_USE_TIMER

#else
    gettimeofday(&time_, NULL);
#endif
    return (uint32)time_.tv_sec;
}

unsigned int ISGWAck::get_utime()
{
#ifdef ISGW_ACK_USE_TIMER
    
#else
    gettimeofday(&time_, NULL);
#endif
    return (uint32)time_.tv_usec;
}

// return lefted ack number in queue
int ISGWAck::process()
{	
    //ACE_DEBUG((LM_TRACE, "[%D] ISGWAck start to process msg"
    //	",quesize=%d\n",msg_queue_.size()));
	
    PPMsg* msg = NULL;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(queue_lock_);
        if (msg_queue_.empty())
        {
            // no message available(should not happen when handle_input invoked)
            return 0;
        }
    
        if ((msg = msg_queue_.front()) == NULL)
        {
            msg_queue_.pop_front();
            return msg_queue_.size();
        }
        msg_queue_.pop_front();
    }

    ACE_DEBUG((LM_TRACE, "[%D] ISGWAck get a msg succ,start to find sock handler.\n"));

    //�ϱ���ǰ�����ͳ����ֵ
    stat(msg);
    
    if(msg->protocol == PROT_UDP_IP) //UDP Э�� 
    {
        //ֻ��һ�� ISGWUIntf ʵ����ֱ�ӷ�����Ϣ����
        ACE_INET_Addr to_addr(msg->sock_port, msg->sock_ip);
        //���ûָ������ ��ȡ�ַ����ĳ���
        if (msg->msg_len == 0 || (msg->msg_len>MAX_INNER_MSG_LEN))
        {
            msg->msg_len = strlen(msg->msg);
        }
        
        //UDPЭ�鷢�Ͳ�Ҫ��������ô�ɿ�
        ISGWUIntf::instance()->send_udp(msg->msg, msg->msg_len, to_addr);
        //���� reclaim memory //������Ӧ��Ϣ��Դ 
        ISGW_Object_Que<PPMsg>::instance()->enqueue(msg, msg->index);
        msg = NULL;
        
    }
    else // TCP 
    {
        ACE_Event_Handler* eh = ACE_Reactor::instance()
            ->find_handler(msg->sock_fd);

        AceSockHdrBase* intf = dynamic_cast<AceSockHdrBase*>(eh); //
        
        if (intf == NULL || intf->get_seq() != msg->sock_seq)
        {
            ACE_DEBUG((LM_ERROR,
                "[%D] ISGWAck find msg owner failed,sock_fd=%u,prot=%u"
                ",ip=%u,port=%u,sock_seq=%u,seq_no=%u,total_span=%u,procs_span=%u"
                ",cmd=%d,msg=%s\n"
                , msg->sock_fd
                , msg->protocol
                , msg->sock_ip
                , msg->sock_port
                , msg->sock_seq
                , msg->seq_no
                , msg->total_span
                , msg->procs_span
                , msg->cmd
                , msg->msg
                ));
            //���� reclaim memory //������Ӧ��Ϣ��Դ 

            ISGW_Object_Que<PPMsg>::instance()->enqueue(msg, msg->index);
            msg = NULL;

            Stat::instance()->incre_stat(STAT_CODE_ACK_NOOWNER);
            return msg_queue_.size();
        }
        
    	//���ûָ������ ��ȡ�ַ����ĳ���
        if (msg->msg_len == 0 || (msg->msg_len>MAX_INNER_MSG_LEN))
        {
            msg->msg_len = strlen(msg->msg);
        }

        ACE_DEBUG((LM_NOTICE,
                "[%D] ISGWAck send msg,sock_fd=%u,prot=%u"
                ",ip=%u,port=%u,sock_seq=%u,seq_no=%u,total_span=%u,procs_span=%u"
                ",send_len=%d\n"
                , msg->sock_fd
                , msg->protocol
                , msg->sock_ip
                , msg->sock_port
                , msg->sock_seq
                , msg->seq_no
                , msg->total_span
                , msg->procs_span
                , msg->msg_len
                ));        
        if(-1==intf->send_n(msg->msg, msg->msg_len))
        {
            //����ʧ���������ر�����
            intf->close();
        }
        //���� reclaim memory //������Ӧ��Ϣ��Դ 
        //if (msg != NULL)
        //{
        ISGW_Object_Que<PPMsg>::instance()->enqueue(msg, msg->index);
        msg = NULL;
        //}
        
    }
    
    return msg_queue_.size();
}

int ISGWAck::handle_input(ACE_HANDLE /* fd = ACE_INVALID_HANDLE */)
{
    while (process() != 0)
    {
        
    }
    return 0;
}

int ISGWAck::handle_timeout(const ACE_Time_Value & tv, const void * arg)
{
    gettimeofday(&time_, NULL); //ˢ�´��ʱ��
    while (process() != 0)
    {
        
    }
    return 0;
}

uint32_t ISGWAck::stat(PPMsg* ack_msg)
{
    struct timeval t_end;
    gettimeofday(&t_end, NULL);
    ack_msg->total_span = EASY_UTIL::get_span(&ack_msg->tv_time, &t_end);
    
#ifdef MSG_LEN_SIZE
    // ������Э����Ҫͳ������������ʱ��
    ReprtInfo info(ack_msg->cmd, 1, 0, ack_msg->total_span, ack_msg->procs_span);
    if(ack_msg->ret<0) info.failed_count = 1;
    Stat::instance()->add_stat(&info);
#else
    //�˴�ֻͳ�ƺ�ʱ���߼����ص�ʧ�����ݣ���ͳ��������(��ڴ��Ѿ�ͳ��)
    ReprtInfo info(ack_msg->cmd, 0, 0, ack_msg->total_span, ack_msg->procs_span);
    if(ack_msg->ret<0) info.failed_count = 1;
    Stat::instance()->add_stat(&info);
#endif

    ACE_DEBUG((LM_NOTICE,
        "[%D] ISGWAck statisitc finish,cmd=%d,time_diff=%d,sock_fd=%u,prot=%u"
        ",ip=%u,port=%u,sock_seq=%u,seq_no=%u,total_span=%u,procs_span=%u\n"
        , info.cmd
        , (ack_msg->total_span-ack_msg->procs_span)
        , ack_msg->sock_fd
        , ack_msg->protocol
        , ack_msg->sock_ip
        , ack_msg->sock_port
        , ack_msg->sock_seq
        , ack_msg->seq_no
        , ack_msg->total_span
        , ack_msg->procs_span
        ));
        
    return ack_msg->total_span;
}

