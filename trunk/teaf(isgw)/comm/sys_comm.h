/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef _SYS_COMM_H_
#define _SYS_COMM_H_

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iconv.h>
#include <sys/time.h>

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <iomanip>
#include <algorithm>
using namespace std;

#ifdef WIN32
#pragma warning(disable:4503)
#define snprintf _snprintf
#endif // WIN32

extern char oi_symmetry_decrypt2(const char* pInBuf, int nInBufLen,
                                 const char* pKey, char* pOutBuf, int *pOutBufLen);
extern void oi_symmetry_encrypt2(const char* pInBuf, int nInBufLen,
                                 const char* pKey, char* pOutBuf, int *pOutBufLen);

namespace EASY_UTIL
{

typedef map<string, string> NAME_VALUE;
typedef std::pair<string, int> ADDRESS;
typedef vector<ADDRESS> HOSTS;

#define INFO_LIST_SEPARATOR_MAJOR    "|"                // ������Ϣ��һ���ָ���
#define INFO_LIST_SEPARATOR_MINOR    ","                // ������Ϣ�Ķ����ָ���
const int SECONDS_ONE_DAY = (60*60*24);

int format_match(const char *key_str, const char* value);
int string_to_map(const string &input, NAME_VALUE &form_data, int flag=0, char sep1=',', char sep2='|');
int string_to_map(const string &input, map<unsigned,unsigned> &form_data);
int string_to_map(const string& input, map<unsigned int, string>& data,
                        const string& record_delim="|", const string& field_delim=",");
int map_to_string(string &output, NAME_VALUE &form_data, int flag=0);
int map_to_string(string &output, map<unsigned,unsigned> &form_data);
int format_time(time_t nowtime, string &time_string, const string& format= "%Y-%m-%d %H:%M:%S");
int StringToHex(char *buffer, int &len);
int HexToString(char *buffer, int &len);
std::string CharToHex(char c);
char HexToChar(const std::string & sHex);
std::string Encode(const std::string & sSrc);   
std::string Decode(const std::string & sSrc);
// �°�ı���뺯�� ����ͳһʹ������������ 
std::string Encode_ex(const std::string & sSrc);   
// �°�ı���뺯�� ����ͳһʹ������������ 
std::string Decode_ex(const std::string & sSrc);
std::string Base64Encode(const std::string str);
std::string Base64Decode(const std::string str);
char *Utf8ToGb2312(char *dest,int dest_len,const char*src);
char *Utf8ToGb18030(char *dest,int dest_len,const char*src);
char *Gb2312ToUtf8(char *dest,int dest_len,const char*src);
char *Gb18030ToUtf8(char *dest,int dest_len,const char*src);
string Utf8ToUrlcode(string src);
string UrlcodeToUtf8(string src);
int code_convert(const char * from_charset, const char * to_charset,char * inbuf,int inlen,char * outbuf,int outlen);

//ȡģʽƥ�����ֵ����m%d��ȡ%d��ֵ
int get_format_value(const char *key_str, const char* value, std::string &patternvalue);

//��"%Y-%m-%d %H:%M:%S" ת��Ϊ unix time ԭ������ set_time_from_str
unsigned int get_time_from_str(const char* time_str);
//����days��ǰ0��0�ֵ� unix time ������
unsigned int get_time_days_ago(int days);
//��ʽ YYYYMMDD
unsigned int get_date_from_time(time_t  iTime);
//��ȡ nowtime �е��·�ֵ ԭ������ get_time_month
unsigned int get_month_from_time(time_t nowtime);
int replace_tag(const std::string &input
                                                , std::string &output 
                                                , const std::string &startTag 
                                                , const std::string &endTag   
                                                , const std::string &replaceTag
                                                );
//��ȡnowtime�е����ֵ
unsigned int get_year_from_time(time_t nowtime);

unsigned int get_hot_value(int time, int checkNum, int upNum, int replyNum);
//�� src �л�ȡ char ���ֵĴ��� ����ָ���� start λ�ÿ�ʼ
int get_charc_num(const string &src, const char c, int start=0);
//�� src �л�ȡ char ���ֵ��±�(��1��ʼ��)�б� ����ָ���� start λ�ÿ�ʼ���ڽ��� bitmap �ַ��� 
int get_charc_idxlist(const string &src, const char c, string &idxlist
    , const string& delim=" ", int start=0);
void parse_host_addr(const string src, HOSTS &hosts, const string delim);
unsigned int get_year_from_time(time_t nowtime);
void replace_char(char *str_depose, char old_char, char new_char);
string intos(unsigned num);
//����������ת�����ַ������
template <typename T>
string to_string(T input)
{
    std::ostringstream oss;
    oss << input;
    return oss.str();
}

template <typename T>
string list_to_string(T start, T end, const char*delim )
{
    std::ostringstream oss;
    for(T it=start; it!=end; it++)
    {
        oss << *it;
        oss << delim;
    }
    return oss.str();
}
void Replace(string &value, string src, string dest);
void w_substr(string &src, int len, char*dst);
//�ṩ�̰߳�ȫ��ip��ַתΪ�����ʽ�ĺ���
void inet_ntoa_r(unsigned u_ip, char*cp, int size);
//���������ϻ�õ�roleidx���н��룬areaΪ�����ţ�idx Ϊ�ý�ɫ����ҵĽ�ɫ�б��е�λ��
//����ֵ���ǻ�ȡ����roleidx
unsigned get_roleidx_from_role(const unsigned area, const unsigned idx);

//����ҽ�ɫ�����roleidx��idxΪ�ý�ɫ����ҵĽ�ɫ�б��е�λ��
//����ֵ���ǻ�ȡ���Ĵ�����
unsigned get_area_from_roleidx(const unsigned roleidx, unsigned& idx);

// ����hash����
unsigned int SDBMHash(const std::string& str);
unsigned int DJBHash(const std::string& str);
unsigned int hash_uid(const std::string& str);

// ��ȡָ��ʱ���unixtime
uint32_t get_timed_ut(int h, int m, int s, time_t cur_time);

// ȥ���ַ�����ָ���ַ�
std::string remove_charset(const std::string &src, char* charset);

// ��������ʮ������ת��
std::string bin2hex(const char* data, int32_t len);
int32_t hex2bin(const std::string& src, char* data, int32_t& len);

// ��ȡ����ַ�����Ĭ���������5~9���ַ���
std::string get_random_str(int32_t num = 0);

// ��vector���ӳ��ַ���
template <typename T>
string join(const T& list, const string& delim = " ")
{
    std::ostringstream oss;
    for(typename T::const_iterator cit = list.begin(); cit != list.end(); 
++cit)
    {
        if(cit == list.begin())
        {
            oss<<*cit;
        }
        else
        {
            oss<<delim<<*cit;
        }
    }
    return oss.str();
}

template <typename T>
string join(const T& list, const uint32_t max_length, const string& delim = " ")
{
    std::ostringstream oss;
    uint32_t cur_len = 0;
    
    for(typename T::const_iterator cit = list.begin(); cit != list.end(); ++cit)
    {
        std::ostringstream tmp_oss;
        if(cit == list.begin())
        {
            tmp_oss << *cit;
            cur_len += tmp_oss.str().size();
            if (cur_len > max_length) break;

            oss<<*cit;
        }
        else
        {            
            tmp_oss << delim << *cit;
            cur_len += tmp_oss.str().size();
            if (cur_len > max_length) break;
            
            oss<<delim<<*cit;
        }
    }
    return oss.str();
}

// map2str
template<typename K, typename V>
std::string map2str(const std::map<K, V> &src, const std::string &odlmt, 
const std::string &idlmt)
{
    std::ostringstream oss("");
    typename std::map<K, V>::const_iterator it = src.begin();
    if(it != src.end())
    {
        oss << it->first << idlmt << it->second;
        for(++it;it != src.end();++it)
        {
            oss << odlmt << it->first << idlmt << it->second;
        }
    }
    
    return oss.str();
};

// vec2str
template<typename T>
std::string vec2str(const vector<T> & src, const string & dlmt)
{
    ostringstream oss("");
    if(!src.empty())
    {
        oss << src[0];
        
        for(unsigned int i = 1;i < src.size();i++)
        {
            oss << dlmt << src[i];
        }
    }
    
    return oss.str();
};

typedef std::map<std::string, std::string> IPAddrMap;

void str2hex(const char* str, unsigned char* hex, int len);
void hex2str(unsigned char* data, int len, char* str);

void str2upper(const char* src, char* dest);
void str2lower(const char* src, char* dest);

typedef map<string, string> CGI_PARAM_MAP;

/// ���ֽ���������� str ����޸�(�ƻ�) ��ʹ����Աע�� 
int parse(char* str, CGI_PARAM_MAP& dest, const char minor='='
    , const char* major="&\r\n");

int parse(const string &src, CGI_PARAM_MAP &dest
    , const char minor='=', const char major='&');

int parse(const string &src, map<int, int> &dest
    , const char minor='=', const char major='&');

/// �ָ��ַ���src������vector dest ���� 
void split(const string& src, const string& delim, vector<string>& dest);
void split(const string& src, const string& delim, vector<unsigned int>& dest);

// �ָ�����set��(��ȥ��) ���һ�ȥ���հ׵�����
void split_ign(const string& src, const string& delim, set<string>& dest);
void split_ign(const string& src, const string& delim, set<unsigned int>& dest);
void split_ign(const string& src, const string& delim, set<int>& dest);
// �˰汾����Էָ���֮��հ׵����� һ�㲢�е����ݵ�ʱ����Ժ��� ����uin�б� ���� uin
void split_ign(const string& src, const string& delim, vector<string>& dest);
void split_ign(const string& src, const string& delim, vector<unsigned int>& dest);
/// ���ش� 1970-01-01 �����ڵ����� 
int  days(unsigned int unix_time=0);

char* get_time_str(char *time_str);
string get_local_ip(const string& if_name);
IPAddrMap get_local_ip_map();

}

#endif // _SYS_COMM_H_ 

