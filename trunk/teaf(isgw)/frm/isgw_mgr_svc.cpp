#include "qmode_msg.h"
#include "isgw_mgr_svc.h"
#include "isgw_ack.h" //���Ϳͻ�����Ӧ��Ϣ
#include "stat.h"
#include "isgw_oper_base.h"
#include "cmd_amount_contrl.h"

#ifdef ISGW_USE_LUA
#include "lua_oper.h"
#endif

extern IsgwOperBase* factory_method();

int ISGWMgrSvc::discard_flag_ = 1; //Ĭ�Ͻ��ж��� 
int ISGWMgrSvc::discard_time_ = 2; //Ĭ�ϳ���2�붪��

int ISGWMgrSvc::control_flag_ = 0;
CmdAmntCntrl *ISGWMgrSvc::freq_obj_=NULL;

ISGWMgrSvc* ISGWMgrSvc::instance_ = NULL;
int ISGWMgrSvc::rflag_=0;
int ISGWMgrSvc::rtnum_=0;

map<string, PlatConnMgrAsy*> ISGWMgrSvc::route_conn_mgr_map_;
ACE_Thread_Mutex ISGWMgrSvc::conn_mgr_lock_;
int ISGWMgrSvc::local_port_ = 0; 

ISGWMgrSvc* ISGWMgrSvc::instance()
{
    if (instance_ == NULL)
    {
        instance_ = new ISGWMgrSvc();
    }
    return instance_;
}

int ISGWMgrSvc::init()
{
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc start to init\n"));
    //���ø�ҵ����Ĺ���������������һ������Ķ���
    //�������Ķ�������ʧ�ܣ��������������
    IsgwOperBase *object = factory_method();
    if(NULL == object)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc init IsgwOperBase failed\n"));
        return -1;
    }

    int ret =IsgwOperBase::instance(object)->init();
    if(ret !=0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc init IsgwOperBase failed\n"));
        return -1;
    }

// ͨ�� dll ��ʽ ���� dll ��ִ�г�ʼ������ 
// Ŀǰ��̬���ʵ�������⣬������
// ���ܾ�̬���룬��Ȼ��core
#if defined ISGW_USE_DLL
    ret = init_dll_os(NULL);
    if (ret != 0)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc init dll failed\n"));
        return -1;
    }
#elif defined ISGW_USE_LUA
    //ʵ����һ�£���������������
    LuaOper::instance();
#endif

    //��Ϣת��������
    //rflag_=0,�ر�·�ɹ���
    //rflag_=1,��·�ɹ��ܣ�����ֻ��·��ת��
    //rflag_=2,��·�ɹ��ܣ�����Ҳ����Ϣ����
    SysConf::instance()->get_conf_int("router", "route_flag", &rflag_);
    int idx=0;
    char tmp_str[32];
    while(1&rflag_)
    {
        char ip[16] = {0};
        int port = 0;
        snprintf(tmp_str, sizeof(tmp_str), "ip_%d", idx);
        if(0!=SysConf::instance()->get_conf_str("router", tmp_str, ip, sizeof(ip)))
            break;
            
        snprintf(tmp_str, sizeof(tmp_str), "port_%d", idx);
        if(0!=SysConf::instance()->get_conf_int("router", tmp_str, &port))
            break;        
        
        PlatConnMgrAsy *mgr =new PlatConnMgrAsy(ip, port);
        char appname[32] = {0};
        snprintf(appname, sizeof(appname), "router_%d", idx);        
        route_conn_mgr_map_[appname] = mgr;
        
        idx++;
    }
    rtnum_ = idx; // ��¼���õ� router ���� 

    // ���ؼ����˿�
    SysConf::instance()->get_conf_int("system", "port", &local_port_);
    
    //�������Ƶ�����
    SysConf::instance()->get_conf_int("cmd_amnt_cntrl", "control_flag", &control_flag_);
    freq_obj_ = new CmdAmntCntrl();
    
    // ������Ϣ��ʱ����
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "discard_flag", &discard_flag_);
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "discard_time", &discard_time_);

    int quesize = MSG_QUE_SIZE;	
    //������Ϣ���д�С
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "quesize", &quesize); 
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc set quesize=%d(byte)\n", quesize));
    open(quesize, quesize, NULL);     
    
    //�����߳�����������
    int threads = DEFAULT_THREADS; // ACESvc �� ���� 
    SysConf::instance()->get_conf_int("isgw_mgr_svc", "threads", &threads);        
    if (threads < SVC_QUE_NUM)
    {
        threads = SVC_QUE_NUM;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc set number of threads=%d\n", threads));
    activate(THR_NEW_LWP | THR_JOINABLE, threads);

    ACE_DEBUG((LM_INFO,"[%D] ISGWMgrSvc init succ,inner lock=0x%x,conn_mgr_lock=0x%x\n"
            , &(queue_[0].lock()), &(conn_mgr_lock_.lock()) ));
    return 0;
}

#ifdef ISGW_USE_DLL
// ace �� dll ������ʽ 
int ISGWMgrSvc::init_dll(const char* dllname)
{
    if (dllname != NULL && strlen(dllname) != 0)
    {
        strncpy(dllname_, dllname, sizeof(dllname_));
    }
    
    if (strlen(dllname_) == 0)
    {
        if (SysConf::instance()
            ->get_conf_str("isgw_mgr_svc", "dllname", dllname_, sizeof(dllname_)) != 0)
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get dllname failed\n"));
            return -1;
        }
    }

    // ǿ��ж�� dll 
    dll_.close ();
    int ret = dll_.open(dllname_);
    if (ret != 0) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc open dll %s failed"
            ",%s\n"
            , dllname_
            , dll_.error()
            ));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc open dll %s succ\n", dllname_));    
    
    OPER_INIT oper_init = (OPER_INIT)dll_.symbol("oper_init");
    if (oper_init == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_init failed\n"));
        return -1;
    }
    oper_init();
    oper_proc_ = (OPER_PROC)dll_.symbol("oper_proc");
    if (oper_proc_ == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_proc failed\n"));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc get all symbol succ\n"));
    
    return 0;
}

// os �Դ��� dll ������ʽ 
int ISGWMgrSvc::init_dll_os(const char* dllname)
{
    if (dllname != NULL && strlen(dllname) != 0)
    {
        strncpy(dllname_, dllname, sizeof(dllname_));
    }
    
    if (strlen(dllname_) == 0)
    {
        if (SysConf::instance()
            ->get_conf_str("isgw_mgr_svc", "dllname", dllname_, sizeof(dllname_)) != 0)
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get dllname failed\n"));
            return -1;
        }
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc init_dll_os %s start\n", dllname_));
    
    // ǿ��ж�� dll 
    if (dll_hdl_ != ACE_SHLIB_INVALID_HANDLE) 
    {
        ACE_OS::dlclose(dll_hdl_);
    }
    
    dll_hdl_ = ACE_OS::dlopen(dllname_);
    if (dll_hdl_ == ACE_SHLIB_INVALID_HANDLE) 
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc open dll %s failed"
            ",%s\n"
            , dllname_
            , ACE_OS::dlerror()
            ));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc open dll %s succ\n", dllname_));
    
    OPER_INIT oper_init = (OPER_INIT)ACE_OS::dlsym(dll_hdl_, "oper_init");
    if (oper_init == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_init failed\n"));
        return -1;
    }
    oper_init();
    oper_proc_ = (OPER_PROC)ACE_OS::dlsym(dll_hdl_, "oper_proc");
    if (oper_proc_ == NULL)
    {
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc get symbol oper_proc failed\n"));
        return -1;
    }
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc get all symbol succ\n"));
    
    return 0;
}
#endif

PPMsg* ISGWMgrSvc::process(PPMsg*& ppmsg)
{  
    unsigned int threadid = syscall(__NR_gettid); //ACE_OS::thr_self();
    unsigned int pid = ACE_OS::getpid();
    ACE_DEBUG((LM_TRACE, "[%D] ISGWMgrSvc (%u:%u) process start\n", threadid, pid));
    gettimeofday(&ppmsg->p_start, NULL);

    //��������Ҫ���볤��
    //������Э�鲻֪�������֣�Ĭ��Ϊ0���޷������ͳ��
#ifdef MSG_LEN_SIZE
    QModeMsg qmode_req(ppmsg->msg_len, ppmsg->msg, ppmsg->sock_fd, ppmsg->sock_seq
        , ppmsg->seq_no, ppmsg->protocol, (unsigned int)ppmsg->tv_time.tv_sec, ppmsg->sock_ip, ppmsg->sock_port);
#else
    QModeMsg qmode_req(ppmsg->msg, ppmsg->sock_fd, ppmsg->sock_seq
        , ppmsg->seq_no, ppmsg->protocol, (unsigned int)ppmsg->tv_time.tv_sec, ppmsg->sock_ip, ppmsg->sock_port);

    ppmsg->cmd = qmode_req.get_cmd();
    
    //�ϱ�������ͳ�� ���˴�ֻͳ��������������������ָ��
    ReprtInfo info(ppmsg->cmd, 1, 0, 0, 0);
    Stat::instance()->add_stat(&info);
#endif
    qmode_req.set_tvtime(&(ppmsg->tv_time));
    ppmsg->msg_len = 0;

    //����������Ϣ��Դ
    //ISGW_Object_Que<PriProReq>::instance()->enqueue(pp_req, pp_req->index);
    //pp_req = NULL;
    
    int ret = 0; 
    ///ack_len ����������ʲôЭ�飬�Ƿ������Ӧ��Ϣ���ͻ��˵ȵȣ�����ȡֵ����:
    ///��ֵΪ��(MAX_INNER_MSG_LEN����)����ҵ��ָ���ĳ��ȴ������ݣ���ҵ��֤Э��������ԣ�
    ///��ֵΪ����ʾ�����ͻ��˷�����Ӧ��Ϣ
    ///0��MAX_INNER_MSG_LEN ֵ�ɿ�ܱ�����Э��������ԣ�����ӽ�����MSG_SEPARATOR
    int ack_len = MAX_INNER_MSG_LEN; 
    
    // �ڲ�����Ĵ������ ���ȴ��� �����Ϸ��� 
    if (qmode_req.get_cmd() <100 && qmode_req.get_cmd() >0)
    {
        char ack_intnl[MAX_INNER_MSG_LEN] = {0};
        ret = IsgwOperBase::instance()->internal_process(qmode_req, ack_intnl, ack_len);
        snprintf(ppmsg->msg, MAX_INNER_MSG_LEN, "%s=%d&%s=%d&%s%s"
            , FIELD_NAME_CMD, qmode_req.get_cmd(), FIELD_NAME_RESULT, ret
            , ack_intnl, MSG_SEPARATOR);

        if (ack_len < 0) //����ͻ��˷�����Ӧ��Ϣ
        {
            //������Ӧ��Ϣ��Դ
            ISGW_Object_Que<PPMsg>::instance()->enqueue(ppmsg, ppmsg->index);
            ppmsg = NULL;
        }
        return ppmsg;
    }
    
    // ���Ϸ��� 
    ret = check(qmode_req);
    if (ret != 0)
    {
        ppmsg->ret = ret;
        // ����ԭ��Ϣ ���� result Ϊ ret ��ֵ
        snprintf(ppmsg->msg, MAX_INNER_MSG_LEN, "%s=%d&%s%s"
            , FIELD_NAME_RESULT, ret, qmode_req.get_body()
            , MSG_SEPARATOR);
        ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc process check msg failed,ret=%d"
            ",sock_fd=%u,prot=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,time=%u,now=%u"
            ",msg=%s\n"
            , ret
            , ppmsg->sock_fd
            , ppmsg->protocol
            , ppmsg->sock_ip
            , ppmsg->sock_port
            , ppmsg->sock_seq
            , ppmsg->seq_no
            , ppmsg->tv_time.tv_sec
            , ISGWAck::instance()->get_time()
            , ppmsg->msg
            ));

        ppmsg->ret = ret;
        return ppmsg;
    }

    ret = 0;
    char ack_buf[MAX_INNER_MSG_LEN+1] = {0}; // ����Ե�ַ&�ָ�Ľ����Ϣ
    
    unsigned time_diff = EASY_UTIL::get_span(&(ppmsg->tv_time), &ppmsg->p_start);
    //���ݿͻ�������ָ�������ز���
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWMgrSvc start to process cmd=%d,time_diff=%u"
        ",sock_fd=%u,prot=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u,MAX_INNER_MSG_LEN=%d\n"
        , qmode_req.get_cmd()
        , time_diff
        , ppmsg->sock_fd
        , ppmsg->protocol
        , ppmsg->sock_ip
        , ppmsg->sock_port
        , ppmsg->sock_seq
        , ppmsg->seq_no
        , MAX_INNER_MSG_LEN
        ));
    //�Ƿ�·�ɵı�־������ʹ��Э��ָ����
    int rflag = qmode_req.get_rflag();
    if(rflag == 0)
    {
        rflag = rflag_;
    }
#if defined ISGW_USE_DLL
    switch(qmode_req.get_cmd())
    {
        case CMD_FLASH_DLL:
            ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc process start init dll\n"));
            ret = init_dll_os((*(qmode_req.get_map()))["dllname"].c_str());
            if (ret != 0)
            {
                snprintf(ack_buf, ack_len-1, "%s", "info=svc init dll failed");
                ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc process init dll %s failed\n"
                    , (*(qmode_req.get_map()))["dllname"].c_str()
                    ));
            }
        break;
        default:
            if (oper_proc_ != NULL)
            {
                ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc process oper_proc,cmd=%d,uin=%u\n"
					, qmode_req.get_cmd(), qmode_req.get_uin()));
                ret = oper_proc_(qmode_req,  ack_buf, ack_len);
            }
            else
            {
                ret = -2;
                snprintf(ack_buf, ack_len-1, "%s", "info=svc oper_proc is null");
                ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc oper_proc failed,is null,try reinit dll\n"));
                init_dll_os(NULL);
            }
        break;
    }
#elif defined ISGW_USE_LUA
    ACE_DEBUG((LM_INFO, "[%D] ISGWMgrSvc run lua\n"));
    LuaOper::instance()->process(qmode_req.get_body(), ack_buf, ack_len);

#else
    
    int proc_rflag = rflag;
    if (0 != proc_rflag)
    {
        string &dst_svr = (*(qmode_req.get_map()))["_appname"];
        if(!dst_svr.empty()) // ָ����ת����ַ
        {
            int dst_port = 0;
            // Ŀ�ĵ�ַδ���� ���� �뱾�ص�ַƥ��
            if(SysConf::instance()->get_conf_int(dst_svr.c_str(), "port", &dst_port) != 0
                || dst_port == local_port_)
            {
                proc_rflag = 0; // ����ת����ֱ�Ӵ���
            }
        }
    }

    // �����ļ�(ini)δ����·��ת����ָ��δָ��ת��
    if (0 == proc_rflag)
    {
        ret = IsgwOperBase::instance()->process(qmode_req,  ack_buf, ack_len);
    }
    else if (1 == proc_rflag)
    {
        ret = forward(qmode_req, proc_rflag, qmode_req.get_uin());
        if(0 == ret) 
        {
            // ���(ֻ)ת����ģ���ڴ˴����������ؿͻ�����Ϣ
            ack_len = -1;
        }
        else
        {
            // ��������ת��ʧ��
            snprintf(ack_buf, MAX_INNER_MSG_LEN-1, "forward msg failed");
        }
    }
    else
    {
        // �ȴ���Ҳת��������ת��ʧ�ܲ�����
        ret = IsgwOperBase::instance()->process(qmode_req,  ack_buf, ack_len);
        forward(qmode_req, proc_rflag, qmode_req.get_uin());
    }
#endif
    struct timeval p_end;
    gettimeofday(&p_end, NULL);
    ppmsg->procs_span = EASY_UTIL::get_span(&ppmsg->p_start, &p_end);
    ppmsg->ret = ret;
    ppmsg->cmd = qmode_req.get_cmd();
    
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWMgrSvc finish process "
        "cmd=%d,ret=%d,ack=%s,ack_len=%d,procs_span=%u,"
        "sock_fd=%u,prot=%u,ip=%u,port=%u,sock_seq=%u,seq_no=%u\n"
        , qmode_req.get_cmd()
        , ret
        , ack_buf
        , ack_len
        , ppmsg->procs_span
        , ppmsg->sock_fd
        , ppmsg->protocol
        , ppmsg->sock_ip
        , ppmsg->sock_port
        , ppmsg->sock_seq
        , ppmsg->seq_no
        ));
    
    //���� ack_len �Ľ��������Ӧ��Ϣ
    if (ack_len < 0) //����ͻ��˷�����Ӧ��Ϣ
    {
        //������Ӧ��Ϣ��Դ
        //if (ppmsg != NULL)
        //{
        ISGW_Object_Que<PPMsg>::instance()->enqueue(ppmsg, ppmsg->index);
        ppmsg = NULL;
        //}
    }
    else if (ack_len == 0 || ack_len == MAX_INNER_MSG_LEN) //���տ�ܶ����Э����Ӧ
    {
        int msg_len = 0;
        //͸����ص����ݣ�֧��ǰ���첽���ã����Ʊ��ֺ� idip һ�� 
        unsigned int sock_fd = strtoul((*(qmode_req.get_map()))["_sockfd"].c_str(), NULL, 10);
        unsigned int sock_seq = strtoul((*(qmode_req.get_map()))["_sock_seq"].c_str(), NULL, 10);
        unsigned int msg_seq = strtoul((*(qmode_req.get_map()))["_msg_seq"].c_str(), NULL, 10);
        unsigned int prot = strtoul((*(qmode_req.get_map()))["_prot"].c_str(), NULL, 10);
        unsigned int time = strtoul((*(qmode_req.get_map()))["_time"].c_str(), NULL, 10);
        //�˴�͸��rflag�����߿ͻ���(Ҳ��һ���������)�Ƿ�ֱ��͸������Ϣ
        //int rflag = atoi((*(qmode_req.get_map()))["_rflag"].c_str());
        if (rflag != 0 || sock_fd != 0 || sock_seq != 0 || msg_seq != 0)
        {
            msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len
                , "_sockfd=%d&_sock_seq=%d&_msg_seq=%d&_prot=%d&_time=%d&_rflag=%d&"
                , sock_fd, sock_seq, msg_seq, prot, time, rflag);
        }
        
        if (strlen(ack_buf) == 0)
        {
            msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len-INNER_RES_MSG_LEN
                , "%s=%d&%s=%d"
                , FIELD_NAME_CMD, qmode_req.get_cmd(), FIELD_NAME_RESULT, ret
                ); 
        }
        else
        {
            msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len-INNER_RES_MSG_LEN
                , "%s=%d&%s=%d&%s"
                , FIELD_NAME_CMD, qmode_req.get_cmd(), FIELD_NAME_RESULT, ret
                , ack_buf); 
            if (msg_len > MAX_INNER_MSG_LEN-sizeof(MSG_SEPARATOR))
            {
                msg_len = MAX_INNER_MSG_LEN-sizeof(MSG_SEPARATOR);
            }
        }
        
        //���Ϲ�����͸���ֶ�(ҵ�����)��Э�������
        msg_len += snprintf(ppmsg->msg+msg_len, MAX_INNER_MSG_LEN-msg_len
            , "&_seqstr=%s%s", (*(qmode_req.get_map()))["_seqstr"].c_str(), MSG_SEPARATOR);
    }
    else //��ȫ�����û������Э�������Ϣ �������κ����� 
    {
        if (ack_len > MAX_INNER_MSG_LEN)
        {
            ACE_DEBUG((LM_ERROR, "[%D] warning ack msg len is limited,ack_len=%d\n", ack_len));
            ack_len = MAX_INNER_MSG_LEN;
        }
        
        ppmsg->msg_len = ack_len;
        memcpy(ppmsg->msg, ack_buf, ppmsg->msg_len);
    }
    
    // ��������ʱʹ�� 
    if(1 == control_flag_)
    {
        freq_obj_->amount_inc(qmode_req.get_cmd(), ret);
    }
    
    ACE_DEBUG((LM_TRACE, "[%D] ISGWMgrSvc (%u:%u) process end\n", threadid, pid));
    
    return ppmsg;
}

int ISGWMgrSvc::send(PPMsg* ack)
{
    ACE_DEBUG((LM_TRACE,"[%D] ISGWMgrSvc::send start\n"));
    
    //���������Ϣ������ �������Ϣ�͵����ӶԶ� 
    ISGWAck::instance()->putq(ack);
    
    ACE_DEBUG((LM_TRACE,"[%D] ISGWMgrSvc::send end\n"));
    return 0;
}

int ISGWMgrSvc::check(QModeMsg& qmode_req)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    
    //�ж��Ƿ���Ҫ������������
    if (1 == control_flag_)
    {
        int status = freq_obj_->get_status(qmode_req.get_cmd(), (unsigned)tv_now.tv_sec);
        if (status != 0)
        {
            // ͳ��ʧ����� 
            Stat::instance()->incre_stat(STAT_CODE_SVC_REJECT);
            return ERROR_REJECT;
        }
    }
    
    // �жϴ������Ϣ�Ƿ��Ѿ���ʱ 
    unsigned int span = EASY_UTIL::get_span(qmode_req.get_tvtime(), &tv_now);
    if (1==discard_flag_ && span > discard_time_*10000)
    {
        // ͳ��ʧ����� 
        ACE_DEBUG((LM_ERROR,"[%D] ISGWMgrSvc check failed,span=%u\n", span));
        Stat::instance()->incre_stat(STAT_CODE_SVC_TIMEOUT);
        return ERROR_TIMEOUT_FRM;
    }
    
    return 0;
    
}

// ��Ϣָ��������Ϣָ����·�ɣ�����Ͱ����õķ�ʽ·��
int ISGWMgrSvc::forward(QModeMsg& qmode_req, int rflag, unsigned int uin)
{    
    int ret = 0;
    char buf[MAX_INNER_MSG_LEN] = {0};
    // ת��ʱ��Ҫ���Ͽͻ�����Ϣ���Ա���յ�������Ϣʱ���͵��ͻ���
    int msg_len = 0;
    msg_len += snprintf(buf, sizeof(buf)
        , "_sockfd=%d&_sock_seq=%d&_msg_seq=%d&_prot=%d&_time=%d&_rflag=%d&%s%s"
        , qmode_req.get_handle(), qmode_req.get_sock_seq(), qmode_req.get_msg_seq()
        , qmode_req.get_prot(), qmode_req.get_time(), rflag 
        , qmode_req.get_body(), MSG_SEPARATOR);
    ACE_DEBUG((LM_NOTICE, "[%D] ISGWMgrSvc start to forward msg,buf=%s\n", buf));
    char appname[32] = {0};
    strncpy(appname, (*(qmode_req.get_map()))["_appname"].c_str(), sizeof(appname));
    if (strlen(appname) != 0) // ��Э������� 
    {
        if (route_conn_mgr_map_[appname] == NULL)
        {
            // ��Ҫ�����̰߳�ȫ
            ACE_Guard<ACE_Thread_Mutex> guard(conn_mgr_lock_);
            if (route_conn_mgr_map_[appname] == NULL)
            {
                PlatConnMgrAsy *mgr = new PlatConnMgrAsy();
                mgr->init(appname);            
                route_conn_mgr_map_[appname] = mgr;
            }
        }
        // �첽���͵�ʱ�� == 0 �ǳɹ��� 
        if(0 > route_conn_mgr_map_[appname]->send((void*)buf, msg_len, uin))
        {
            ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc forward failed,buf=%s\n", buf));
            // ͳ��ʧ����� 
            Stat::instance()->incre_stat(STAT_CODE_SVC_FORWARD);
            ret = -1;
        }
    }
    else // �����õ� ��������ʱ���Ѿ���ʼ��
    {
        int fail_count = 0;
        for(int idx=0; idx<rtnum_; idx++)
        {
            snprintf(appname, sizeof(appname), "router_%d", idx);
            // �첽���͵�ʱ�� == 0 �ǳɹ��� 
            if(0 > route_conn_mgr_map_[appname]->send((void*)buf, msg_len, uin))
            {
                ACE_DEBUG((LM_ERROR, "[%D] ISGWMgrSvc forward failed,buf=%s\n", buf));
                // ͳ��ʧ����� 
                Stat::instance()->incre_stat(STAT_CODE_SVC_FORWARD);
                fail_count++;
            }
        }

        // δ����ת��ip��ת����ʧ��
        if(rtnum_ <= 0 || fail_count == rtnum_)
        {
            ret = -1;
        }
    }
    return ret;
}


