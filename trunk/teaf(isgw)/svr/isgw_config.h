/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/*******************************************************************************
 *  @file    isgw_config.h
 *
 *  ���ļ�Ϊ��svr��ܶ���������ļ��������ڴ��ļ��ж���һ�º��ֵ��
 *  �ı�ϵͳ��Ĭ�����á�
 *
 *  @author xwfang
 ******************************************************************************/

#ifndef _ISGW_CONFIG_H_
#define _ISGW_CONFIG_H_

#define MAX_INNER_MSG_LEN 4096
//#define MSG_SEPARATOR "\r\n"

// �����Ƿ�ʹ�ö�����ģʽ
//#define MSG_LEN_SIZE 4  // ISGWIntf ʹ�� ��Ŀǰֻ֧�� 2 ��4 ����ֵ
//#define NO_EXTRA_SIZE

// �����Ƿ�ʹ�ö�����ģʽ
//#define MSG_LEN_SIZE_C 4  // ISGWCIntf ʹ�� ����ֵĿǰӦ��ֻ֧�� 4 һ��ȡֵ 
//#define NO_EXTRA_SIZE_C

//#define FIELD_NAME_CMD "cmd"

//��ʱ�澯��ʱ��
//#define ALARM_TIMEOUT 1
#define OBJECT_QUEUE_SIZE 5000
//����֧�ֿ����������������˾ͱ�ʾҪ����
//#define MAX_IDLE_TIME_SEC 600 //��λ s 

#endif //_ISGW_CONFIG_H_
