#ifndef __NGXUSER_HPP__
#define __NGXUSER_HPP__
#include "HeaderPrecompilation.hpp"

namespace TestValidation
{
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

			//静态函数对象functor作为接口，处理数据库读取相关
			static std::function<rapidjson::Document(const char*,const char*,bool&)> operation;

		protected:
			bool validated = false;
			rapidjson::Document _request;
			rapidjson::Document _response;
	};

	//处理注册请求
	class RegisterUser : public BaseUser
	{
		public:
			RegisterUser() = delete;
			RegisterUser(const RegisterUser &) = delete;
			RegisterUser(const char *json) : BaseUser(json)
		{
			if (_request.HasMember("id") &&     //工号
					_request.HasMember("password")/* && //密码
					_request.HasMember("name") &&       //姓名
					_request.HasMember("role") &&     //功能组
					_request.HasMember("email") && //邮件
					_request.HasMember("group")*/
					)    //联系方式
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

	//处理登录请求
	class LoginUser : public BaseUser
	{
		public:
			LoginUser()=delete;
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

	//处理数据请求
	class DataUser : public LoginUser
	{
		public:
			DataUser() = delete;
			DataUser(const DataUser &) = delete;
			DataUser(const char *json) : LoginUser(json, true)
		{
			if(	_request.HasMember("database") &&
				_request.HasMember("sessionid")&&
				_request.HasMember("statement"))
			{
				validated = true;
				//获得数据库中上次登录时间
				lastSessionTime = GetLastSessionTime("sessionid");
				if (_request["sessionid"].GetString()==std::string("registeronly")){
					//注册阶段的sessionid不需要任何动作
				}
				else
				{
					//没有sessionid则数据库只有只读权限
					//数据库除了管理数据外，都是可读的
					if(std::string(_request["statement"].GetString()).find("SELECT")==std::string::npos){
						validated=false;	
					}
					//只能读取有限的试验信息
				}
			}else{
				validated=false;
			}
		}

			//读取数据
			DataUser &Data();
	};

	//处理邮件请求
	class MailUser: public LoginUser{
		public:
			MailUser()=delete;
			MailUser(const MailUser&)=delete;
			MailUser(const char* json):LoginUser(json){
				if(validated==false){
					//登录信息错误，则无法继续后续执行动作
					return;
				}
				if(	_request.HasMember("recv_users") &&//邮件接收者，数组
					_request.HasMember("email_content_path")//邮件文件地址
				  )
				{
					if( _request["recv_users"].IsArray()==false||
						_request["email_content_path"].IsArray()==false)
					{
						//键值对中的值都应该是数组
						validated=false;
						rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();
						_response.AddMember("recv_alert","recv_users key-value should be an array.",_dallocator);
					}else{
						//确保每个邮件接收者都是以saicmotor.com结尾
						validated=true;
						auto& recv_array=_request["recv_users"];
						for(auto& it:recv_array.GetArray()){
							if(std::string(it.GetString()).find("@saicmotor.com")==std::string::npos){
								validated=false;
								rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();
								_response.AddMember("recv_alert","email should be ended with @saicmotor.com.",_dallocator);
								break;
							}
						}
					}
				}else{
					//键值对格式错误
					validated=false;				
				}

			}

			//邮件发送
			MailUser& Send();
			//设置根目录
			void SetRootDir(const char* _dir){
				upload_root_dir=_dir;
			}
			//设置邮件发送方
			void SetHost(const char* _host){
				host_email=_host;
			}
			//设置邮件发送方密码
			void SetHostPwd(const char* _pwd){
				host_email_password=_pwd;
			}
		private:
			std::string upload_root_dir;
			std::string host_email;
			std::string host_email_password;
	};

} // namespace TestValidation
#endif
