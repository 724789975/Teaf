#include "plat_conn_mgr_asy.h"
#include "isgw_ack.h"
#include "stat.h"
#include "asy_task.h"

ACE_Thread_Mutex PlatConnMgrAsy::conn_mgr_lock_;

//key: ip:fd   value: status 
//��ip Ϊkey ��ʱ�� ͬһ��ip�ϾͲ���֧�ֶ�������� Ҳ���ǲ���֧��ת��������˿� 
typedef map<string, int> CONN_STATUS_MAP;
static CONN_STATUS_MAP conn_status_; //���������Ϣ 

int set_conn_status(string ip, int status)
{
    if(status == 0) 
    {
        conn_status_.erase(ip);
        return 0;
    }
    
    //ֱ���޸�����״̬���˴���Ҫ����
    conn_status_[ip] = status;
    return 0;
}

int get_conn_status(string ip)
{
    return conn_status_[ip];
}

PlatConnMgrAsy::PlatConnMgrAsy()
{
    memset(section_, 0x0, sizeof(section_));
    memset(ip_, 0x0, sizeof(ip_));    
    ip_num_ = IP_NUM_DEF; // Ĭ��ÿ��2�������� ip 
    port_ = 0;    
    time_out_.set(SOCKET_TIME_OUT/1000, (SOCKET_TIME_OUT%1000)*1000);
    memset(&conn_info_, 0x0, sizeof(conn_info_));    
}

PlatConnMgrAsy::PlatConnMgrAsy(const char*host_ip, int port)
{
    port_ = port;
    time_out_.set(SOCKET_TIME_OUT/1000, (SOCKET_TIME_OUT%1000)*1000);
    memset(&conn_info_, 0x0, sizeof(conn_info_));
    
    ip_num_ = IP_NUM_MAX;    
    for(int i = 0; i < ip_num_; ++i)
    {
        snprintf(ip_[i], sizeof(ip_[i]), "%s", host_ip);
        init_conn(i);
    }
}

PlatConnMgrAsy::~PlatConnMgrAsy()
{
    for (int i=0; i<IP_NUM_MAX; i++)
    {
        fini_conn(i);
    }
}

int PlatConnMgrAsy::init(const char *section)
{
    ACE_Guard<ACE_Thread_Mutex> guard(conn_mgr_lock_); 
    if (section != NULL)
    {
        strncpy(section_, section, sizeof(section_));
    }
    
    if ((SysConf::instance()->get_conf_int(section_, "ip_num", &ip_num_) != 0)
        || (ip_num_ > IP_NUM_MAX))
    {
        ip_num_ = IP_NUM_MAX;
    }
    
    int time_out = SOCKET_TIME_OUT;
    if (SysConf::instance()->get_conf_int(section_, "time_out", &time_out) == 0)
    {
        time_out_.set(time_out/1000, (time_out%1000)*1000);
    }
    
    //��ȡ ip �б� 
    char ip_str[16];
    for(int i=0; i<ip_num_; i++)
    {
        memset(ip_str, 0x0, sizeof(ip_str));
        snprintf(ip_str, sizeof(ip_str), "ip_%d", i);
        
        if (SysConf::instance()
            ->get_conf_str(section_, ip_str, ip_[i], sizeof(ip_[i])) != 0)
        {
            ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy get config failed"
                ",section=%s,ip_%d\n", section_, i));
            ip_num_ = i; //ʵ�ʳɹ���ȡ���� ip ���� 
            break;
        }
    }
    
    if (ip_num_ == 0) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init failed"
            ",section=%s,ip_num=%d\n", section_, ip_num_));
        ip_num_ = IP_NUM_DEF; //���ΪĬ��ֵ 
        return -1;
    }
    
    if (SysConf::instance()->get_conf_int(section_, "port", &port_) != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy get port config failed"
            ",section=%s\n", section_)); 
        return -1;
    }
    
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrAsy start to init conn,ip_num=%d,section=%s,lock=0x%x\n"
            , ip_num_, section_, &(conn_mgr_lock_.lock()) ));
    for(int i=0; i<ip_num_; i++)
    {
        if (init_conn(i) !=0)
        {
            //ĳ�����Ӳ��� ���˳����� 
            //return -1;
        }
    }

    //��ʼ��һ�� stat ��֤��ʹ�ÿ�ܵ�ģ��ʹ��Ҳ����
    Stat::instance()->init("");
    return 0;
}

// ip port ��ָ��ʱ ��ʹ���ڲ��� ��ָ��ʱ���滻�ڲ��� 
int PlatConnMgrAsy::init_conn(int ip_idx, const char *ip, int port)
{
    if (ip_idx<0 || ip_idx >=IP_NUM_MAX)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init conn failed,para is invalid"
            ",ip_idx=%d\n", ip_idx));
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -1;
    }
    
    // ��֤���ݳ�ʼ����һ���� 
    ACE_Guard<ACE_Thread_Mutex> guard(conn_lock_[ip_idx]);

    //������Ҫ���ж�һ��,����Ѿ��������������򷵻�
    if (0 == is_estab(ip_idx))
    {
        ACE_DEBUG((LM_DEBUG,
            "[%D] PlatConnMgrAsy init_conn,intf is already connected"
            ",ip_idx=%d\n"
            , ip_idx
            ));
        return 0;
    }
    
    if (ip != NULL)
    {
        strncpy(ip_[ip_idx], ip, sizeof(ip_[ip_idx]));
    }
    if (port != 0)
    {
        port_ = port;
    }
	
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrAsy start to init conn"
        ",ip=%s,port=%d,ip_idx=%d,lock=0x%x\n"
        , ip_[ip_idx], port_, ip_idx, &(conn_lock_[ip_idx].lock()) ));

    // û�����õ� ip �Ͷ˿� 
    if (strlen(ip_[ip_idx]) == 0 || port_ == 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init conn failed"
            ",ip=%s,port=%d,ip_idx=%d\n"
            , ip_[ip_idx], port_, ip_idx));
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -2; //���ò����� 
    }
    
    ACE_INET_Addr svr_addr(port_, ip_[ip_idx]);
    ISGW_CONNECTOR connector;
    conn_info_[ip_idx].intf = new ISGWCIntf();
    ACE_Synch_Options opt(2, time_out_);
    if (connector.connect(conn_info_[ip_idx].intf, svr_addr, opt) != 0) //, my_option
    {
        ACE_DEBUG((LM_ERROR, "[%D] PlatConnMgrAsy init conn failed"
            ",ip=%s,port=%d,ip_idx=%d\n"
            , ip_[ip_idx], port_, ip_idx
            ));
        //fini_conn(ip_idx);
        Stat::instance()->incre_stat(STAT_CODE_CONN_FAIL);
        return -1;
    }

    //����������Ϣ 
    conn_info_[ip_idx].sock_fd = conn_info_[ip_idx].intf->get_handle();
    conn_info_[ip_idx].sock_seq = conn_info_[ip_idx].intf->get_seq();    
    ACE_DEBUG((LM_INFO, "[%D] PlatConnMgrAsy init conn succ"
        ",ip=%s,port=%d,ip_idx=%d,sock_fd=%d,sock_seq=%d,cnaddr=%@\n"
        , ip_[ip_idx]
        , port_
        , ip_idx
        , conn_info_[ip_idx].sock_fd
        , conn_info_[ip_idx].sock_seq
        , &conn_info_
        ));
    ostringstream os;
    os<<ip_[ip_idx]<<":"<<conn_info_[ip_idx].sock_fd;
    set_conn_status(os.str(), 1);
    return 0;
}

int PlatConnMgrAsy::send(const void * buf, int len, const unsigned int uin)
{
    int ip_idx = get_ip_idx(uin);
    if (is_estab(ip_idx) != 0)
    {
        ACE_DEBUG((LM_ERROR,
            "[%D] PlatConnMgrAsy send failed"
            ",intf is disconn"
            ",ip_idx=%d\n"
            , ip_idx
            ));
        if (init_conn(ip_idx) !=0)
        {
            return -1;
        }
    }

    PPMsg *req = NULL; // ����ʵ�������̨��������Ϣ req
    int index = ISGW_Object_Que<PPMsg>::instance()->dequeue(req);
    if ( req == NULL )
    {
        ACE_DEBUG((LM_ERROR,
            "[%D] PlatConnMgrAsy dequeue msg failed"
            ",maybe system has no memory\n"
            ));
        return -1;
    }
    memset(req, 0x0, sizeof(PPMsg));
    req->index = index;

    req->sock_fd = conn_info_[ip_idx].sock_fd;
    req->protocol = PROT_TCP_IP;
    req->sock_seq = conn_info_[ip_idx].sock_seq;
    gettimeofday(&(req->tv_time), NULL);
    req->time = ISGWAck::instance()->get_time();
	req->msg_len = len;
	memcpy(req->msg, buf, req->msg_len);
    ACE_DEBUG((LM_NOTICE, "[%D] PlatConnMgrAsy send msg to ISGWAck"
        ",sock_fd=%u,protocol=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u"
        ",len=%d,msg=%s\n"
        , req->sock_fd
        , req->protocol
        , req->sock_ip
        , req->sock_port
        , req->sock_seq
        , req->seq_no
        , req->time
        , req->msg_len
        , req->msg
        ));
    
    //���������Ϣ������ �������Ϣ�͵����ӶԶ� (����ָ����˵�����)
    ISGWAck::instance()->putq(req);
    return 0;
}

int PlatConnMgrAsy::send(const void * buf, int len, ASYRMsg & rmsg, const unsigned int uin)
{
	ASYTask::instance()->insert(rmsg);
	return send(buf, len, uin);
}

int PlatConnMgrAsy::get_ip_idx(const unsigned int uin)
{
    if (uin == 0)
    {
        return rand()%ip_num_;
    }
    
    return uin%ip_num_;
}

int PlatConnMgrAsy::is_estab(int ip_idx)
{
    //ISGWCIntf �������������״̬������ֻ��Ҫ�жϼ���
    ostringstream os;
    os<<ip_[ip_idx]<<":"<<conn_info_[ip_idx].sock_fd;
    if (get_conn_status(os.str()) != 1)
    {
        ACE_DEBUG((LM_ERROR,"[%D] PlatConnMgrAsy check estab failed"
            ",ip=%s,port=%d\n", ip_[ip_idx], port_));
        return -1;
    }
    
    return 0;
}

int PlatConnMgrAsy::fini_conn(int ip_idx)
{
    ostringstream os;
    os<<ip_[ip_idx]<<":"<<conn_info_[ip_idx].sock_fd;
    set_conn_status(os.str(), 0);
    conn_info_[ip_idx].sock_fd = 0;
    conn_info_[ip_idx].sock_seq = 0;
    return 0;
}

