/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef _RDS_ACCESS_H_
#define _RDS_ACCESS_H_

#include "isgw_comm.h"
#include <string>
#include "hiredis/hiredis.h"

struct SSPair{
    SSPair() : score(0), rank(0), has_err(0) {}
    string member;
    int64_t score;
    long long rank;
    int32_t has_err;
};

class RdsSvr {

public:
    struct Handle {
        Handle() : rc(NULL), mutex(NULL), ip(), port(0), dbid(0)
        {
            timeout = (struct timeval){0};
        }
        redisContext *rc;
        ACE_Thread_Mutex *mutex;
        string ip;
        int port;
        struct timeval timeout;
        int dbid;
    };

public:
    //ֻ��һ��redisʱ�ſɵ��������
    static RdsSvr &instance();

    //ɾ��ָ��key
    int del_key(string dbid, const string &uin, const string &custom_key);
    //��ָ��key���ù���ʱ��
    int expire_key(const string& dbid, const string& uin, const string& custom_key, int seconds, int flag=0);
    //�õ�redis�������ϵ�unixʱ��
    int redis_time(unsigned &tv_sec, unsigned &tv_usec);
    // ��ȡkey����ʱ��
    int ttl(const std::string& key);
    //��ѯָ��key�µ�string���͵�����
    int get_string_val(string dbid
                       , const string &uin
                       , const string &custom_key
                       , string &value);
    //����ָ��key�µ�string���͵�����
    int set_string_val(string dbid
                        , const string &uin
                        , const string &custom_key
                        , const string &value
                        , const string expire="");
    //��ָ��key��string������������������
    //��key�µ����ݱ���Ϊ����
    int inc_string_val(string dbid
                        , const string &uin
                        , const string &custom_key
                        , string &value);

    // ����ָ��key����ԭ�ӷ���ԭ�ȵ�ֵ
    int get_set(const std::string& key
                        , const string& custom_key
                        , const std::string& nvalue
                        , std::string& ovalue);

/*-----------------sorted set---------------------------*/
    //��ѯָ��key��sorted-set�����б�
    int get_sset_list(string dbid
                        , const string &ssid
                        , vector<SSPair> &elems
                        , int start
                        , int num
                        , int flag);
    //��ѯָ��key��sorted-set���ݵ�ָ��member������
    int get_sset_rank(string dbid
                        , const string &ssid
                        , vector<SSPair> &elems
                        , int flag);
    //��ѯָ��key��sorted-set���ݵ�ָ��member�ķ���
    int get_sset_score(string dbid
                        , const string &ssid
                        , vector<SSPair> &elems);
    //����ָ��key��sorted-set���ݵ�ָ��member��scoreֵ
    //����Ѿ����ڸ�member�����串��
    int set_sset_list(const string& dbid
                        , const string &ssid
                        , const vector<SSPair> &elems
                        , int &num);
    //����������ָ��ÿ��member��score�仯ֵ
    int inc_sset_list(string dbid
                        , const string &ssid
                        , SSPair &elem);
    //ɾ��ָ��key��sorted-set���ݵ�ָ��member
    int del_sset_list(string dbid
                        , const string &ssid
                        , const vector<SSPair> &elems
                        , int &num);
    //��ѯָ��key��sorted-set���ݵ�member����
    int get_sset_num(string dbid, const string &ssid);
/*-----------------sorted set---------------------------*/

    //��ȡhash����ָ��field��valueֵ
    int get_hash_field(string dbid
                        , const string &hkey
                        , const vector<string> &fields
                        , map<string, string> &value);
    //��ȡhash����field/value�б�
    int get_hash_field_all(string dbid, const string &hkey, std::map<string, string> &elems);
    //����hash����ָ��field��valueֵ
    int set_hash_field(string dbid, const string &hkey, const std::map<string, string> &elems);

    //����hash����ָ��field��ֵ������
    int inc_hash_field(const string& dbid
                        , const string& hkey
                        , const string& field
                        , int& value);
    //ɾ��hash����ָ��field��valueֵ
    int del_hash_field(string dbid, const string &hkey, const string &field);
    // ���ع�ϣ����field��Ŀ��ʧ�ܷ���-1
    int get_hash_field_num(const string &hkey);
    int hexists(const std::string &hkey, const std::string& field);

    // �����б�Ԫ�ظ�����Ĭ��dbidΪ0
    int get_list_num(const std::string& list);

    // �����б�ָ������Ԫ��
    int get_list_range(const std::string& list
        , std::vector<std::string>& range_vec
        , int32_t start
        , int32_t stop);

    // ������Ϣ��ָ��Ƶ��
    int pub_msg(const std::string& channel, const std::string& msg);

    // ���б���������,Ĭ�ϴ��ұ߿�ʼ���
    int push_list_value(const std::string& list_name, std::string& value, uint32_t left = 0);

    // ��һ�൯���ݳ�ջ
    int pop_list_value(const std::string& list_name, std::string& value, uint32_t left = 0);

    //ִ��ָ����lua�ű�
    int eval_exec(string dbid
                    , const string &script
                    , const string keys
                    , const string param
                    , int &res);
    // ִ��ָ����lua�ű�(�������)
    int eval_multi_command(const string &script
        , const std::vector<std::string>& keys
        , const std::vector<std::string>& params
        , std::vector<std::string>& res
        , int32_t type = 0);
    
    RdsSvr();
    RdsSvr(string &redis);
    ~RdsSvr();
    int init(string redis_sec="redis");

private:
    template <typename T>
    std::string stringify(T input)
    {
        std::ostringstream oss;
        oss << input;
        return oss.str();
    }
    
    ACE_Thread_Mutex& get_handle(Handle * &h, int flag=0);
    void rst_handle(Handle * &h);
    int sel_database(Handle * &h, int dbid);
    int get_string_reply(Handle * &h
                        , redisReply * &reply
                        , string &key
                        , const string value=""
                        , const string expire=""
                        , const int flag=0);

    // master redis server list
    std::vector<Handle> m_redis_list_;

    // slave redis server list
    std::vector<Handle> s_redis_list_;
    struct timeval timeout_;
    int port_;
    char master_ip_[128];
    char slave_ip_[128];
    char passwd_[64];
    static int inited;
};

#endif 

