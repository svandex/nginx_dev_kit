#ifndef __NGXUSER_HPP__
#define __NGXUSER_HPP__
#include "HeaderPrecompilation.hpp"

namespace TestValidation
{
/*
数据相关操作
*/
class FetchData
{
public:
    FetchData() = delete;
    FetchData(const FetchData &) = delete;

/*
默认关键字：

1. status，值为真假，表示执行是否成功
2. message，值为字符串，表示执行状态信息

*/
    static rapidjson::Document SQL(const char *, const char *,bool&);
    static rapidjson::Document SQL_Legacy(const char *, const char *, bool &);
};

/*
用户基本功能
*/
class BaseUser
{
public:
    //删除构造和拷贝构造函数
    BaseUser() = delete;
    BaseUser(const BaseUser &) = delete;
    //将JSON字符串转换成JSON对象
    BaseUser(const char *);

    //注册和登录时需要判断用户是否存在
    bool Existence();
    //判断是否会话过期
    virtual bool SessionExpired() { return true; };
    //返回json对象
    rapidjson::Document Ret() { return std::move(_response); };

    //调试使用
    void DocumentCopy(){
        _response = std::move(_request);
    }

protected:
    bool validated = false;
    rapidjson::Document _request;
    rapidjson::Document _response;
};

class RegisterUser : public BaseUser
{
public:
    RegisterUser() = delete;
    RegisterUser(const RegisterUser &) = delete;
    RegisterUser(const char *json) : BaseUser(json)
    {
        if (_request.HasMember("name") &&     //姓名
            _request.HasMember("password") && //密码
            _request.HasMember("id") &&       //工号
            _request.HasMember("role") &&     //功能组
            _request.HasMember("contact"))    //联系方式
        {
            validated = true;
        }
        else
        {
            validated = false;
        }
    }

    RegisterUser &Register();
};

class LoginUser : public BaseUser
{
public:
    LoginUser(const LoginUser &) = delete;
    LoginUser(const char *json) : BaseUser(json)
    {
        {
            if (_request.HasMember("id") &&
                _request.HasMember("password"))
            {
                validated = true;
                //获得数据库中上次登录时间
                lastSessionTime = GetLastSessionTime("id");
            }
            else
            {
                validated = false;
            }
        }
    }

    //只要登录就会更新sessionid和lastlogintime
    bool UpdateSessionId(const char*);
    bool UpdateLastLoginTime(const char *);
    LoginUser &Login();

protected:
    LoginUser(const char *json, bool) : BaseUser(json){};
    time_t GetLastSessionTime(const char*);
    bool SessionExpired() override;

    time_t lastSessionTime;
};

class DataUser : public LoginUser
{
public:
    DataUser() = delete;
    DataUser(const DataUser &) = delete;
    DataUser(const char *json) : LoginUser(json, true)
    {
        if (_request.HasMember("sessionid") &&
            _request.HasMember("database") &&
            _request.HasMember("statement"))
        {
            validated = true;
            //获得数据库中上次登录时间
            lastSessionTime = GetLastSessionTime("sessionid");
        }
        else
        {
            validated = false;
        }
    }

    DataUser &Data();
};

} // namespace TestValidation
#endif
