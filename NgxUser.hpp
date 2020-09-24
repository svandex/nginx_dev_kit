#ifndef NGXUSER_HPP
#define NGXUSER_HPP
#include "HeaderPrecompilation.hpp"
#include "vmime/vmime.hpp"

using DbInterface=std::function<rapidjson::Document(const char*,const char*,bool&)>;

namespace TestValidation
{
	//提供基础功能
	class TObject{
		protected:
			void setStatusAndMessage(rapidjson::Document&,bool,const char*);	
			rapidjson::Document _response;

			//状态和信息
			void setSAMFormatError(){
				setStatusAndMessage(_response,false,"Client data format error!");
			}

			void setSAMSessionError(){
				setStatusAndMessage(_response,false,"Session Expired!");
			}

			void setSAMDBError(){
				setStatusAndMessage(_response,false,"Database execution error!");
			}

			void setSAMFileError(){
				setStatusAndMessage(_response,false,"File operation failed!");
			}
	};

	//请求基类
	class BaseOp:public TObject
	{
		public:
			//删除构造和拷贝构造函数
			BaseOp() = delete;
			BaseOp(const BaseOp &) = delete;
			//将JSON字符串转换成JSON对象
			BaseOp(const char *);

			//注册和登录时需要判断用户是否存在
			bool Existence();
			//返回json对象
			rapidjson::Document Ret() { return std::move(_response); };

			//静态函数对象functor作为接口，处理数据库读取相关
			static DbInterface BaseDao;

			//上次更新时间
			time_t GetLastSessionTime(const char*,const char*dbName="userinfo");

			//执行状态
			bool Status(){
				if(_response.HasMember("status")){
					return _response["status"].GetBool();
				}
				return false;
			}
		protected:
			//只要登录就会更新sessionid和lastlogintime
			bool UpdateSessionId(const char*);
			bool UpdateLastLoginTime(const char *);
			bool SessionExpired();

			//工号和邮箱，常用值
			std::string GetID();
			std::string GetEMail();

			time_t lastSessionTime;

			//格式是否正确
			bool validated = false;
			rapidjson::Document _request;

			//数据库执行状态
			bool OpStatus=false;
	};

	//处理注册请求
	class RegisterOp : public BaseOp
	{
		public:
			RegisterOp() = delete;
			RegisterOp(const RegisterOp &) = delete;
			RegisterOp(const char *json) : BaseOp(json)
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

			RegisterOp &Register();
	};

	//处理登录请求
	class LoginOp : public BaseOp
	{
		public:
			LoginOp()=delete;
			LoginOp(const LoginOp &) = delete;
			LoginOp(const char *json) : BaseOp(json)
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

			LoginOp &Login();
	};

	//处理数据请求
	class DataOp : public BaseOp
	{
		public:
			DataOp() = delete;
			DataOp(const DataOp &) = delete;
			DataOp(const char *json) : BaseOp(json)
		{
			if(	_request.HasMember("database") &&
				_request.HasMember("sessionid")&&
				_request.HasMember("statement"))
			{
				validated = true;
				if (_request["sessionid"].GetString()!=std::string("registeronly")){
					//获得数据库中上次登录时间
					lastSessionTime = GetLastSessionTime("sessionid");
				}
			}else{
				validated=false;
			}
		}

			//读取数据
			DataOp &Data();
	};

	//处理邮件请求
	class MailOp:public BaseOp{
		public:
			MailOp()=delete;
			MailOp(const MailOp&)=delete;
			MailOp(const char* json):BaseOp(json){
				sender.setName(vmime::text("iTEST工作室",vmime::charset("utf-8")));
				sender.setEmail("iTEST@saicmotor.com");
				mb.setExpeditor(sender);
				mb.setSubject(vmime::text("一封来自iTEST的邮件",vmime::charset("utf-8")));
			}
			//发送邮件
			void SendMail();
		protected:
			//数据成员
			vmime::messageBuilder mb;
			vmime::mailbox sender;
			vmime::mailboxList recver;
			rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();
	};

	//正文和附件都来自文件
	class MailFileOp: public MailOp{
		public:
			MailFileOp()=delete;
			MailFileOp(const MailFileOp&)=delete;
			MailFileOp(const char* json):MailOp(json){
				//需要重新更新lastsessiontime

				if(	_request.HasMember("recv_users") //邮件接收者，数组
						&&	_request.HasMember("email_content") //邮件正文
						&&	(_request["email_content"].HasMember("body")||_request["email_content"].HasMember("path"))
				  )
				{
					//获得数据库中上次登录时间
					if(_request.HasMember("sessionid")){
						lastSessionTime = GetLastSessionTime("sessionid");
					}

					validated=true;
					//判断数组
					if( _request["recv_users"].IsArray()==false)
					{
						//键值对中的值都应该是数组
						validated=false;
						_response.AddMember("recv_alert","recv_users or attachments, key-value should be an array.",_dallocator);
					}else{
						//确保每个邮件接收者都是以saicmotor.com结尾
						auto& recv_array=_request["recv_users"];
						for(auto& it:recv_array.GetArray()){
							if(std::string(it.GetString()).find("@saicmotor.com")==std::string::npos){
								validated=false;
								_response.AddMember("email_alert","email should be ended with @saicmotor.com.",_dallocator);
								break;
							}
						}
					}

					//判断附件的格式
					if(_request.HasMember("attachments")&&_request["attachments"].IsArray()==true){
						auto& attachments=_request["attachments"];
						for(auto& at:attachments.GetArray()){
							if(!at.HasMember("md5")||!at.HasMember("mname")||!at.HasMember("mtype")){
								validated=false;
								_response.AddMember("attachments","attachments metadata incorrect.",_dallocator);
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
			MailFileOp& Construct();

			//设置根目录
			void SetRootDir(const char* _dir){
				upload_root_dir=_dir;
			}
		private:
			std::string upload_root_dir;
	};

	//验证码发送
	class VerificatonCodeOp:public MailOp{
		public:
			VerificatonCodeOp()=delete;
			VerificatonCodeOp(const VerificatonCodeOp&)=delete;
			VerificatonCodeOp(const char* json):MailOp(json){
				if (_request.HasMember("id"))
				{
					validated = true;
				}
				else
				{
					validated = false;
				}
			}
			//验证码构造
			VerificatonCodeOp& Construct();

			//验证码验证
			VerificatonCodeOp& Verify();

	};

	//文件上传
	class UploadOp:public TObject{
		public:
			UploadOp()=delete;
			UploadOp(const UploadOp&)=delete;
			//MultiPart-form字段解析
			UploadOp(const char* multiform);
	/*
			//静态函数对象functor作为接口，处理数据库读取相关
			static std::function<rapidjson::Document(const char*,const char*,bool&)> BaseDao;
		private:
			bool UploadOpStatus=false;
			bool validated=false;
	*/
			//返回json对象
			rapidjson::Document Ret() { return std::move(doc); };
			UploadOp& ToDataBase();

			//数据库调用接口
			static DbInterface UploadDao;

			//存储sessionid
			static std::string sessionid;
			
		private:
			rapidjson::Document doc;
			bool validated=false;
			bool UploadStatus=false;
	};

} // namespace TestValidation

#endif
