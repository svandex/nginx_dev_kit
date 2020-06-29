#include <vmime/vmime.hpp>
#include "NgxUser.hpp"
#include "NgxSvandex.hpp"
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define SVANDEX_SESSION_EXPIRED 6000

std::function<rapidjson::Document(const char*,const char*,bool&)> TestValidation::BaseUser::operation{};

TestValidation::BaseUser::BaseUser(const char *json)
{
    _request.SetObject();
    _response.SetObject();

    if (_request.Parse(json).HasParseError())
    {
        throw std::logic_error("BaseUser Constrution Error!");
    }

    validated = true;
}

bool TestValidation::BaseUser::Existence()
{
    if (!validated)
    {
        return false;
    }

    if (_request.HasMember("id"))
    {
        std::stringstream stm;
        stm << "select * from `userinfo` where `id`='" << _request["id"].GetString() << "'";
        bool status;
        auto result = operation("management", stm.str().c_str(), status);
        if (!status || result["sqlite"].GetArray().Size() == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}

bool TestValidation::LoginUser::SessionExpired()
{
    if (!validated)
    {
        return false;
    }
    //比较登录时产生的sessionid或者客户端发送的sessionid和数据库中的
    if (std::difftime(std::time(nullptr), lastSessionTime) > SVANDEX_SESSION_EXPIRED)
    {
        return true;
    }

    return false;
}

TestValidation::LoginUser &TestValidation::LoginUser::Login()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    //是否有相应的字段
    if (!validated)
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "username or password error!", _dallocator);
        return *this;
    }

    //判断用户是否存在
    if (!Existence())
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "no user exists!", _dallocator);
        return *this;
    }

    //判断账号密码是否正确
    std::stringstream stm;
    stm << "select password from userinfo where id=\'" << _request["id"].GetString() << "\'";
    bool status;
    auto result = operation("management", stm.str().c_str(), status);

    //更新登录历史记录
    if (result["sqlite"].GetArray().Size() == 1 && std::strcmp(result["sqlite"][0]["password"].GetString(), _request["password"].GetString()) == 0)
	{
		bool status = UpdateSessionId("id");
		bool statusTime = UpdateLastLoginTime("id");

		if (!status&&!statusTime)
		{
			_response.AddMember("status", false, _dallocator);
			_response.AddMember("message", "sql engine execution error!", _dallocator);
		}
		else
		{
			_response.AddMember("status", true, _dallocator);
			_response.AddMember("message", "login succeed!", _dallocator);

			std::stringstream ret_sessionid_stm;
			ret_sessionid_stm<<"select * from userinfo where id=\'"<<_request["id"].GetString()<<"\'";
			bool status;
			auto result = operation("management", ret_sessionid_stm.str().c_str(), status);

			_response.AddMember("sessionid",rapidjson::Value(result["sqlite"][0]["sessionid"].GetString(),_dallocator),_dallocator);
			_response.AddMember("id",rapidjson::Value(result["sqlite"][0]["id"].GetString(),_dallocator),_dallocator);
		}
	}else{
		_response.AddMember("status", false, _dallocator);
		_response.AddMember("message", "password error!", _dallocator);
		return *this;
	}

	return *this;
}

time_t TestValidation::LoginUser::GetLastSessionTime(const char *field)
{
    bool status;
    std::stringstream stm;
    stm << "select * from userinfo where " << field << " =\'" << _request[field].GetString() << "\'";
    auto result = operation("management", stm.str().c_str(), status);
    if (!status || result["sqlite"].GetArray().Size() == 0)
    {
        return 0;
    }
    else
    {
        std::istringstream lst{result["sqlite"][0]["lastupdatetime"].GetString()};
        if (lst.str() == "expired")
        {
            return std::time(nullptr);
        }
        std::tm t = {};
        lst >> std::get_time(&t, "%Y-%m-%d %T");
        return std::mktime(&t);
    }
}

bool TestValidation::LoginUser::UpdateSessionId(const char* field){
    auto uuid = Svandex::tools::GetUUID();
    std::stringstream updatestream;
    updatestream << "update `userinfo` set `sessionid`=\'" << uuid.c_str() << "\' where " << field << "= \'" << _request[field].GetString() << "\'";

    bool status;
    operation("management", updatestream.str().c_str(), status);
    return status;
}

bool TestValidation::LoginUser::UpdateLastLoginTime(const char* field){
    std::stringstream updatestream;
    updatestream << "update userinfo set `lastupdatetime`=\'" << Svandex::tools::GetCurrentTimeFT().c_str() << "\' where " << field << "= \'" << _request[field].GetString() << "\'";

    bool status;
    operation("management", updatestream.str().c_str(), status);
    return status;
}

TestValidation::RegisterUser &TestValidation::RegisterUser::Register()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    if (Existence())
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "User Has Already Exist", _dallocator);
    }
    else
    {
        //用户不存在
        std::stringstream stm;
        stm << "insert into `userinfo` values(\'"
            //<< _request["role"].GetString() << "\',\'"
            << _request["id"].GetString() << "\',\'"
            << _request["password"].GetString() << "\',\'"
            //<< _request["email"].GetString() << "\',"
            << "expired\',\'"
            << Svandex::tools::GetCurrentTimeFT().c_str() 
			/*
			<< "\',\'"
            << _request["name"].GetString()<<"\',\'"
			<< _request["group"].GetString()
			*/
            << "\')";

        bool status;
        auto result = operation("management", stm.str().c_str(), status);
	if (!status)
        {
            _response.AddMember("status", false, _dallocator);
            _response.AddMember("message", "sql engine execution error!", _dallocator);
	    return *this;
        }
	if (result["sqlite"].GetArray().Size() == 0&&status)
        {
            _response.AddMember("status", true, _dallocator);
            _response.AddMember("message", "register succeed.", _dallocator);
	    _response.AddMember("sessionid","registeronly",_dallocator);
        }
        else
        {
            _response.AddMember("status", false, _dallocator);
            _response.AddMember("message", "register fail.", _dallocator);
        }
    }
    return *this;
}

TestValidation::DataUser &TestValidation::DataUser::Data()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    if (!validated)
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "json format or SELECT case sensitivity error!", _dallocator);
        return *this;
    }

    if (_request["sessionid"].GetString()!=std::string("registeronly") &&SessionExpired())
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "session expired!", _dallocator);
        return *this;
    }

    bool status;
    auto result = operation(_request["database"].GetString(), _request["statement"].GetString(), status);

    if (!status)
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "sql engine excution error!", _dallocator);
    }
    else
    {
        _response.AddMember("data", rapidjson::Value(result["sqlite"], _dallocator), _dallocator);
        _response.AddMember("status", true, _dallocator);
        _response.AddMember("message", "data acqusition succeed!", _dallocator);
    }

    return *this;
}

TestValidation::MailUser& TestValidation::MailUser::Send(){
	rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();
	if (!validated)
	{
		_response.AddMember("status", false, _dallocator);
		_response.AddMember("message", "json format error!", _dallocator);
		return *this;
	}

	//检查对应的工号文件夹是否存在
	if(access((upload_root_dir+"/"+_request["id"].GetString()).c_str(),F_OK)!=0){
		mkdir((upload_root_dir+"/"+_request["id"].GetString()).c_str(),S_IRWXU|S_IRWXG);
	} 
	//检查邮件内容的路径是否都正确
	auto& content_path_array=_request["email_content_path"]; 
	std::string filepath;
	for(auto& it:content_path_array.GetArray()){
		filepath=upload_root_dir+"/"+_request["id"].GetString()+"/"+it.GetString();
		if(access(filepath.c_str(),F_OK)!=0){
			//邮件路径不存在
			_response.AddMember("email_content_path_alert", "one of email body or attachment doesn't exist!", _dallocator);
			return *this;	
		}
	}

	try{
		//构造邮件
		vmime::messageBuilder mb;
		//mb.setExpeditor(vmime::mailbox(host_email.c_str()));
		vmime::mailbox et;
		et.setName(vmime::text("iTEST工作室",vmime::charset("utf-8")));
		et.setEmail("itest@saicmotor.com");
		mb.setExpeditor(et);

		auto& recv_array=_request["recv_users"];
		//附件数量
		auto attchcount=recv_array.GetArray().Size()-1;
		std::string recvArray;
		for(auto& it:recv_array.GetArray()){
			mb.getRecipients().appendAddress(vmime::make_shared<vmime::mailbox>(it.GetString()));
		}

		mb.setSubject(vmime::text("一封来自iTEST的邮件",vmime::charset("utf-8")));

		//从文件中读取邮件内容
		//先读取正文部分
		std::ifstream* ifs=new std::ifstream(upload_root_dir+"/"+_request["id"].GetString()+"/"+content_path_array.GetArray()[0].GetString(),std::ios::in);
		if(!ifs->is_open()){
			_response.AddMember("email_content_path_alert", "one of email body or attachment cannot be opened!", _dallocator);
			return *this;	
		}else{
			vmime::shared_ptr<vmime::utility::inputStream> email_body_stream=vmime::make_shared<vmime::utility::inputStreamPointerAdapter>(ifs);
			vmime::shared_ptr<vmime::contentHandler> email_body=vmime::make_shared<vmime::streamContentHandler>(email_body_stream,0);
			mb.getTextPart()->setCharset(vmime::charsets::UTF_8);
			mb.getTextPart()->setText(email_body);
		}
		//读取附件
		if(attchcount>1){
			//有附件
		}

		//发送邮件
		vmime::utility::url url("smtp://outlook.saicmotor.com");
		vmime::shared_ptr<vmime::net::session> sess=vmime::net::session::create();
		vmime::shared_ptr<vmime::net::transport> tr=sess->getTransport(url);
		/*
		tr->setProperty("options.need-authentication",true);
		tr->setProperty("auth.username",host_email.c_str());
		tr->setProperty("auth.password",host_email_password.c_str());
		*/
		tr->connect();

		//vmime::mailbox from(host_email.c_str());
		vmime::mailbox from(et);
		vmime::mailboxList to;

		for(auto& it:recv_array.GetArray()){
			to.appendMailbox(vmime::make_shared<vmime::mailbox>(it.GetString()));
		}
		tr->send(mb.construct(),from,to);
		tr->disconnect();
		_response.AddMember("status",true, _dallocator);
		_response.AddMember("message","email send succeed!", _dallocator);

	}catch(vmime::exception&e){
		_response.AddMember("vmime_exception",rapidjson::Value(e.what(),_dallocator), _dallocator);
	}catch(std::exception&e){
		_response.AddMember("vmime_std_exception",rapidjson::Value(e.what(),_dallocator), _dallocator);
	}

	return *this;
}
