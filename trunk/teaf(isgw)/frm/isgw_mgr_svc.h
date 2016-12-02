/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/******************************************************************************
*  @file      isgw_mgr_svc.h
*  @author xwfang
*  @history 
*  
******************************************************************************/
#ifndef _ISGW_MGR_SVC_H_
#define _ISGW_MGR_SVC_H_
#include "isgw_comm.h"
#include "cmd_amount_contrl.h"
#include "plat_conn_mgr_asy.h"
#include "../comm/pp_prot.h"

#define MSG_QUE_SIZE 20*1024*1024

typedef int (*OPER_INIT)();
//typedef int (*OPER_PROC)(char* req, char* ack, int& ack_len);
typedef int (*OPER_PROC)(QModeMsg& req, char* ack, int& ack_len);

//��ģ�鸺��Ѵ�������յ���Ϣ����ҵ���߼��ṩ�Ľӿڽ��д��������͸������շ��ӿ�
class ISGWMgrSvc : public ACESvc<PPMsg, PPMsg>
{
public:
    static ISGWMgrSvc* instance();
    virtual ~ISGWMgrSvc(){}
    virtual int init();
    friend class IsgwOperBase;
    
    // get message queue size
    size_t message_count() { return queue_[0].message_count(); }
    
protected:
    ISGWMgrSvc()
    {
#ifdef ISGW_USE_DLL
        memset(dllname_, 0x0, sizeof(dllname_));
        oper_proc_ = NULL;
        
        dll_hdl_ = ACE_SHLIB_INVALID_HANDLE;
#endif
    }
    
    virtual PPMsg* process(PPMsg*& ppmsg);
    virtual int send(PPMsg* ack);
    
    int check(QModeMsg& qmode_req);
    int forward(QModeMsg& qmode_req, int rflag, unsigned int uin=0);

#ifdef ISGW_USE_DLL
private:
    virtual int init_dll(const char* dllname);
    ACE_DLL dll_;
    char dllname_[128]; //
    OPER_PROC oper_proc_;
    
    ACE_SHLIB_HANDLE dll_hdl_;
    virtual int init_dll_os(const char* dllname);
#endif
    
private:
    static ISGWMgrSvc *instance_;
    static int discard_flag_; // ��ʱ��Ϣ������־ 
    static int discard_time_; // ��ʱʱ���ж� 

    static int control_flag_; //Ĭ�ϲ����������� 
    static CmdAmntCntrl *freq_obj_; //Ƶ�ʿ��ƵĶ���

    //·�ɹ��ܵ����ÿ���
    //rflag_=0,�ر�·�ɹ���
    //rflag_=1,��·�ɹ��ܣ�����ֻ��·��ת��
    //rflag_=2,��·�ɹ��ܣ�����Ҳ����Ϣ����
    static int rflag_; //·�ɹ��ܵĿ���
    static int rtnum_; //���õ�·�ɵ����� 
    //key:appname 
    static map<string, PlatConnMgrAsy*> route_conn_mgr_map_;
    static ACE_Thread_Mutex conn_mgr_lock_;

	static int local_port_; // ���ؼ����˿�
};

#endif //_ISGW_MGR_SVC_H_
