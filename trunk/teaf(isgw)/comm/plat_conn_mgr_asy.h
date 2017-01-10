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
* �첽���ӹ����� plat_conn_mgr_asy.h 
* send ʱ ֻ�����ҵ���ص����� sockfd �� sockseq ����Ϣ�ŵ� ISGWAck ���м���
* recv ʱ ȡ����Ķ����е���Ϣ ���ظ��ϲ��� 
* Ӧ��ģ����Ҫ�Լ�ƥ����յ�����Ϣ �������ĸ� ǰ��Ӧ�õ� 
* ʹ��ʱ �� ��������� ��̬�� ��ʹ�� ȷ���ܱ������������״̬ 
******************************************************************************/
#ifndef _PLAT_CONN_MGR_ASY_H_
#define _PLAT_CONN_MGR_ASY_H_
#include "easyace_all.h"
#include "isgw_cintf.h"
#include "asy_prot.h"

#define IP_NUM_MAX 20  //��˷����� ip ������ 
#define IP_NUM_DEF 2  //��˷����� ip ȱʡ���� 
#define MAX_SECTION_NAME_LEN 32 
#define SOCKET_TIME_OUT 100  //��λ ms 

struct stConnInfo
{
    ISGWCIntf* intf;
    uint32 sock_fd; // ������Ч��ʱ�� ���԰Ѵ�ֵ����Ϊ 0 
    uint32 sock_seq;
};

class PlatConnMgrAsy
{
public:
    PlatConnMgrAsy();
    PlatConnMgrAsy(const char*host_ip, int port);
    ~PlatConnMgrAsy();
    // ͨ�����ó�ʼ������ 
    int init(const char *section = NULL); 
    // �ж� intf �Ƿ����� ,���� ISGWAck �Ķ��м��� �ɽӿڲ��Լ���������Ϣ  
    int send(const void *buf, int len, const unsigned int uin=0);
    int send(const void *buf, int len, ASYRMsg &rmsg, const unsigned int uin=0);
    
private:
    ///ͨ��ָ��ip��port ��ʼ����index�����ӣ���ָ�������ڲ��� 
    int init_conn(int ip_idx, const char *ip=NULL, int port=0);
    inline int get_ip_idx(const unsigned int uin);
    // �ж������Ƿ����� 
    inline int is_estab(int ip_idx);
    int fini_conn(int ip_idx);
    
private:
    // stream �����������Ϣ 
    stConnInfo conn_info_[IP_NUM_MAX];
    ACE_Thread_Mutex conn_lock_[IP_NUM_MAX]; 

    char section_[MAX_SECTION_NAME_LEN];
    ACE_Time_Value time_out_;
    int ip_num_; // ���õķ������� ip ���� 
 
    char ip_[IP_NUM_MAX][16]; 
    int port_;
    static ACE_Thread_Mutex conn_mgr_lock_; // conn mgr ������������������Ӳ���ʱ�ô���
};

#endif //_PLAT_CONN_MGR_ASY_H_
