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
*  @file      asy_task.h
*  @author xwfang
*  @history 
*   201405 xwfang ����˵��첽���ӵķ�����Ϣ������(�����̴߳���)
*  
******************************************************************************/
#ifndef _ASY_TASK_H_
#define _ASY_TASK_H_
#include "sys_comm.h"
#include "isgw_comm.h"
#include "asy_prot.h"

#define MAX_ASYR_RECORED 10240*2 //map �������ļ�¼�� 
#define DISCARD_TIME 5  //��ʱ����

class ASYTask : public ACE_Task_Base
{
public:
    static ASYTask* instance()
    {
        ACE_DEBUG((LM_TRACE,
		"[%D] in ASYTask::instance\n"
		));
        if (instance_ == NULL)
        {
            instance_ = new ASYTask();
        }
        ACE_DEBUG((LM_TRACE,
		"[%D] out ASYTask::instance\n"
		));		
        return instance_;
    }       
    virtual int init();
    virtual int fini();
    virtual int insert(ASYRMsg &rmsg);
    //����ȱʡ�Ļص�����
    virtual int set_proc(ASY_PROC asy_proc);
    
protected:
    virtual int svc (void);

protected:
    //ACE_Time_Value time_out_;
    
    static ASYTask *instance_;
    //����첽��Ϣ����ؽ��
    static ASYR_MAP asyr_map_; 
    static ACE_Thread_Mutex asyr_map_lock_;
    static ASY_PROC asy_proc_; //ȱʡ�Ļص�����
    
protected:
    static const int DEFAULT_THREADS = 5;
};

#endif //_ASY_TASK_H_
