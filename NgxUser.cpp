#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <regex>

#include "NgxUser.hpp"
#include "NgxSvandex.hpp"

#include "boost/random/random_device.hpp"
#include "boost/random/uniform_int_distribution.hpp"

constexpr int SVANDEX_SESSION_EXPIRED=1800;//30分钟没有操作则失效

DbInterface TestValidation::BaseOp::BaseDao{};

DbInterface TestValidation::UploadOp::UploadDao{};

std::string TestValidation::UploadOp::sessionid{};

//返回状态和信息
void TestValidation::TObject::setStatusAndMessage(rapidjson::Document& _d,bool _b,const char* _Mess){
	if(_d.HasMember("status")){
		_d["status"].SetBool(_b);
	}else{
		_d.AddMember("status",_b,_d.GetAllocator());
	}
	if(_d.HasMember("message")&&_d["message"].IsArray()){
		_d["message"].GetArray().PushBack(rapidjson::Value(_Mess,_d.GetAllocator()).Move(),_d.GetAllocator());
	}else{
		rapidjson::Value Arr(rapidjson::kArrayType);
		Arr.PushBack(rapidjson::Value(_Mess,_d.GetAllocator()).Move(),_d.GetAllocator());
		_d.AddMember("message",Arr,_d.GetAllocator());
	}
}

TestValidation::BaseOp::BaseOp(const char *json)
{
    _request.SetObject();
    _response.SetObject();

    if (_request.Parse(json).HasParseError())
    {
        throw std::logic_error("BaseOp Constrution Error!");
    }

    validated = true;
}

std::string TestValidation::BaseOp::GetID(){
	if(!validated){
		return std::string{};
	}

	if(_request.HasMember("id")){
		return _request["id"].GetString();
	}
	
	if(_request.HasMember("sessionid")){
		std::stringstream stm;
		stm << "select id from userinfo where `sessionid`='" << _request["sessionid"].GetString() << "'";
		auto result = BaseDao(SQLITE_MAIN_NAME, stm.str().c_str(), OpStatus);
		if(OpStatus){
			return result["sqlite"][0]["id"].GetString();	
		}
	}

	//default return
	return std::string{};
}

std::string TestValidation::BaseOp::GetEMail(){
	auto id=GetID();
	std::stringstream stm;
	stm << "select email from userinfo where `id`='" << id << "'";
	auto result = BaseDao(SQLITE_MAIN_NAME, stm.str().c_str(), OpStatus);
	if(OpStatus){
		return result["sqlite"][0]["email"].GetString();	
	}

	return std::string{};
}

bool TestValidation::BaseOp::Existence()
{
    if (!validated)
    {
        return false;
    }

    if (_request.HasMember("id"))
    {
        std::stringstream stm;
        stm << "select * from `userinfo` where `id`='" << _request["id"].GetString() << "'";
        auto result = BaseDao(SQLITE_MAIN_NAME, stm.str().c_str(), OpStatus);
        if (!OpStatus || result["sqlite"].GetArray().Size() == 0)
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

bool TestValidation::BaseOp::SessionExpired()
{
    if (!validated)
    {
        return false;
    }
    //比较登录时产生的sessionid或者客户端发送的sessionid和数据库中的
    if (std::difftime(std::time(nullptr), lastSessionTime) > SVANDEX_SESSION_EXPIRED)
    {
		_response.AddMember("expired",true,_response.GetAllocator());
        return true;
    }else{
		_response.AddMember("expired",false,_response.GetAllocator());
        return false;
	}
}

TestValidation::LoginOp &TestValidation::LoginOp::Login()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    //是否有相应的字段
    if (!validated)
    {
		setStatusAndMessage(_response,false,"Username or password error!");
        return *this;
    }

    //判断用户是否存在
    if (!Existence())
    {
		setStatusAndMessage(_response,false,"No user exists!");
        return *this;
    }

    //判断账号密码是否正确
    std::stringstream stm;
    stm << "select password from userinfo where id=\'" << _request["id"].GetString() << "\'";
    auto result = BaseDao(SQLITE_MAIN_NAME, stm.str().c_str(), OpStatus);
	if(!OpStatus&&result.HasMember("errmsg")){
		setStatusAndMessage(_response,false,result["errmsg"].GetString());
		return *this;
	}

    //更新登录历史记录
    if (result["sqlite"].GetArray().Size() == 1 && std::strcmp(result["sqlite"][0]["password"].GetString(), _request["password"].GetString()) == 0)
	{
		bool Status = UpdateSessionId("id");
		bool StatusTime = UpdateLastLoginTime("id");

		if (!Status&&!StatusTime)
		{
			setStatusAndMessage(_response,false,"sql engine execution error!");
		}
		else
		{
			setStatusAndMessage(_response,true,"login succeed!");

			std::stringstream ret_sessionid_stm;
			ret_sessionid_stm<<"select * from userinfo where id=\'"<<_request["id"].GetString()<<"\'";
			auto result = BaseDao(SQLITE_MAIN_NAME, ret_sessionid_stm.str().c_str(), OpStatus);

			_response.AddMember("sessionid",rapidjson::Value(result["sqlite"][0]["sessionid"].GetString(),_dallocator),_dallocator);
			_response.AddMember("id",rapidjson::Value(result["sqlite"][0]["id"].GetString(),_dallocator),_dallocator);
		}
	}else{
		setStatusAndMessage(_response,false,"password error!");
		return *this;
	}

	return *this;
}

time_t TestValidation::BaseOp::GetLastSessionTime(const char *field,const char* tableName)
{
    std::stringstream stm;
	stm << "select * from "<<tableName <<" where " << field << " ='" << _request[field].GetString() << "'";
    auto result = BaseDao(SQLITE_MAIN_NAME, stm.str().c_str(), OpStatus);
    if (!OpStatus || result["sqlite"].GetArray().Size() == 0)
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

bool TestValidation::BaseOp::UpdateSessionId(const char* field){
    auto uuid = Svandex::tools::GetUUID();
    std::stringstream updatestream;
    updatestream << "update `userinfo` set `sessionid`=\'" << uuid.c_str() << "\' where " << field << "= \'" << _request[field].GetString() << "\'";

    BaseDao(SQLITE_MAIN_NAME, updatestream.str().c_str(), OpStatus);
    return OpStatus;
}

bool TestValidation::BaseOp::UpdateLastLoginTime(const char* field){
    std::stringstream updatestream;
    updatestream << "update userinfo set `lastupdatetime`=\'" << Svandex::tools::GetCurrentTimeFT().c_str() << "\' where " << field << "= \'" << _request[field].GetString() << "\'";
    BaseDao(SQLITE_MAIN_NAME, updatestream.str().c_str(), OpStatus);
    return OpStatus;
}

TestValidation::RegisterOp &TestValidation::RegisterOp::Register()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    if (Existence())
    {
		setStatusAndMessage(_response,false,"User Has Already Exist");
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
			<< "\',\'"
            << _request["email"].GetString()
			/*
			<<"\',\'"
			<< _request["group"].GetString()
			*/
            << "\')";

        auto result = BaseDao(SQLITE_MAIN_NAME, stm.str().c_str(), OpStatus);
		if (!OpStatus)
		{
			if(result.HasMember("errmsg")){
				setStatusAndMessage(_response,false,result["errmsg"].GetString());
			}
			setStatusAndMessage(_response,false,"sql engine execution error!");
			return *this;
		}
		if (result["sqlite"].GetArray().Size() == 0&&OpStatus)
		{
			setStatusAndMessage(_response,true,"register succeed.");
			_response.AddMember("sessionid","nosessionid",_dallocator);
		}
		else
		{
			setStatusAndMessage(_response,true,"register failed.");
		}
	}
	return *this;
}

TestValidation::DataOp &TestValidation::DataOp::Data()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    if (!validated)
    {
		setStatusAndMessage(_response,false,"Json format or SELECT case sensitivity error!");
        return *this;
    }

	if (_request["sessionid"].GetString()!=std::string("nosessionid")?SessionExpired():false)
    {
		setStatusAndMessage(_response,false,"Session expired!");
        return *this;
    }

    auto result = BaseDao(_request["database"].GetString(), _request["statement"].GetString(), OpStatus);

    if (!OpStatus&&result.HasMember("errmsg"))
    {
		setStatusAndMessage(_response,false,result["errmsg"].GetString());
    }
    else
    {
        _response.AddMember("data", rapidjson::Value(result["sqlite"], _dallocator), _dallocator);
		setStatusAndMessage(_response,true,"Data acqusition succeed!");
    }

    return *this;
}

TestValidation::MailFileOp& TestValidation::MailFileOp::Construct(){
	//格式是否正确
	if (!validated)
	{
		setSAMFormatError();
		return *this;
	}
	//是否登录
	if (SessionExpired())
	{
		setSAMSessionError();
		return *this;
	}

	auto _id=GetID();
	auto _mail=GetEMail();

	/*
	//检查对应的工号文件夹是否存在
	if(access((upload_root_dir+"/"+_id).c_str(),F_OK)!=0){
		mkdir((upload_root_dir+"/"+_id).c_str(),S_IRWXU|S_IRWXG);
	} 
	*/


	//谁登录该系统发送的邮件都会被抄送
	if(_mail!="itest@saicmotor.com"){
		mb.getCopyRecipients().appendAddress(vmime::make_shared<vmime::mailbox>(_mail.c_str()));
	}

	//添加收件人
	auto& recv_array=_request["recv_users"];
	std::string recvArray;
	for(auto& it:recv_array.GetArray()){
		mb.getRecipients().appendAddress(vmime::make_shared<vmime::mailbox>(it.GetString()));
		recver.appendMailbox(vmime::make_shared<vmime::mailbox>(it.GetString()));
		recver.appendMailbox(vmime::make_shared<vmime::mailbox>(_mail.c_str()));
	}

	//1.先读取正文部分
	//根据路径读取正文内容
	if(_request["email_content"].HasMember("path")){
		//从文件中读取邮件内容
		std::ifstream* ifs=new std::ifstream(upload_root_dir+"/"+_request["email_content"]["path"].GetString(),std::ios::in);
		if(!ifs->is_open()){
			setSAMFileError();
			return *this;	
		}else{
			vmime::shared_ptr<vmime::utility::inputStream> email_body_stream=vmime::make_shared<vmime::utility::inputStreamPointerAdapter>(ifs);
			vmime::shared_ptr<vmime::contentHandler> email_body=vmime::make_shared<vmime::streamContentHandler>(email_body_stream,0);
			mb.getTextPart()->setCharset(vmime::charsets::UTF_8);
			mb.getTextPart()->setText(email_body);
		}
	}
	
	//根据字符串读取正文内容
	if(_request["email_content"].HasMember("body")){
		if(!_request["email_content"]["body"].IsString()){
			setSAMFormatError();
		}
		//发来的字符串作为正文部分
		mb.getTextPart()->setCharset(vmime::charsets::UTF_8);
		vmime::shared_ptr<vmime::stringContentHandler> email_body=vmime::make_shared<vmime::stringContentHandler>(_request["email_content"]["body"].GetString());
		mb.getTextPart()->setText(email_body);
	}


	//2.读取附件
	if(_request.HasMember("attachments")){
		//检查附件路径是否都正确
		auto& attachments=_request["attachments"]; 
		size_t attachcount=0;
		//附件数量
		attachcount=attachments.GetArray().Size();
		std::string filepath;
		for(auto& it:attachments.GetArray()){
			filepath=upload_root_dir+"/"+it["md5"].GetString();
			if(access(filepath.c_str(),F_OK)!=0){
				//邮件路径不存在
				setStatusAndMessage(_response,false,"One of email body or attachment doesn't exist!");
				return *this;	
			}
		}

		if(attachcount>0){
			//有附件
			for(size_t index=0;index<attachcount;index++){
				//附件元信息，MD5，媒介类型，文件名
				auto filepath=upload_root_dir+"/"+std::string(attachments[index]["md5"].GetString());
				const char* mtype=attachments[index]["mtype"].GetString();
				const char* mname=attachments[index]["mname"].GetString();

				vmime::shared_ptr<vmime::fileAttachment> att=vmime::make_shared<vmime::fileAttachment>(
						filepath.c_str(),
						vmime::mediaType(mtype),
						vmime::text(mname)
						);

				att->getFileInfo().setFilename(mname);

				mb.appendAttachment(att);
			}
		}
	}

	//succeed
	setStatusAndMessage(_response,true,"Mail Construction Succeed");
	return *this;
}

TestValidation::VerificatonCodeOp& TestValidation::VerificatonCodeOp::Construct(){
	if (!validated)
	{
		setSAMFormatError();
		return *this;
	}
	// 设置邮件正文内容，验证码写入数据库
	std::string chars("abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"1234567890");

	boost::random::random_device rng;
	boost::random::uniform_int_distribution<> index_dist(0,chars.size()-1);

	std::stringstream verificationCode;
	for(int i=0;i<6;++i){
		verificationCode<<chars[index_dist(rng)];	
	}

	//设置收件人
	auto _mail=BaseDao(SQLITE_SECURITY_QUIZ_NAME,(std::string("select `公司邮箱` from `用户信息` where `工号` ='")+_request["id"].GetString()+"'").c_str(),OpStatus);
	mb.getRecipients().appendAddress(vmime::make_shared<vmime::mailbox>(_mail["sqlite"][0]["公司邮箱"].GetString()));

	//设置正文
	mb.getTextPart()->setCharset(vmime::charsets::UTF_8);
	std::string email_body_string="您本次的验证码为 "+verificationCode.str()+" ,将于五分钟后失效。";
	vmime::shared_ptr<vmime::contentHandler> email_body=vmime::make_shared<vmime::stringContentHandler>(email_body_string);
	mb.getTextPart()->setText(email_body);

	std::stringstream stm;
	//检查数据库中是否已有记录
	stm<<"select * from verificationcode where id='"<<_request["id"].GetString()<<"'";	
	auto isExisted=BaseDao(SQLITE_MAIN_NAME,stm.str().c_str(),OpStatus);
	if(isExisted["sqlite"].GetArray().Size()>0){
		//更新数据库中记录
		stm.str("");
		stm<<"update verificationcode set vcode='"
			<<verificationCode.str().c_str()<<"',lastupdatetime='"
			<<Svandex::tools::GetCurrentTimeFT().c_str()<<"' where id='"
			<<_request["id"].GetString()<<"'";
		BaseDao(SQLITE_MAIN_NAME,stm.str().c_str(),OpStatus);
	}else{
		//将验证码写入数据库
		stm.str("");
		stm<<"insert into verificationcode values('"
			<<_request["id"].GetString()<<"','"
			<<verificationCode.str().c_str()<<"','"
			<<Svandex::tools::GetCurrentTimeFT().c_str()<<"')";
		BaseDao(SQLITE_MAIN_NAME,stm.str().c_str(),OpStatus);
	}

	if(OpStatus){
		recver.appendMailbox(vmime::make_shared<vmime::mailbox>(_mail["sqlite"][0]["公司邮箱"].GetString()));
	}

	setStatusAndMessage(_response,true,"Send Verification Code Succeed!");
	return *this;
}

void TestValidation::MailOp::SendMail() try{
	//发送邮件
	vmime::utility::url url("smtp://outlook.saicmotor.com");
	vmime::shared_ptr<vmime::net::session> sess=vmime::net::session::create();
	vmime::shared_ptr<vmime::net::transport> tr=sess->getTransport(url);
	tr->connect();
	tr->send(mb.construct(),sender,recver);
	tr->disconnect();
	setStatusAndMessage(_response,true,"Email send succeed!");
}catch(vmime::exception&e){
	_response.AddMember("vmime_exception",rapidjson::Value(e.what(),_dallocator), _dallocator);
}catch(std::exception&e){
	_response.AddMember("vmime_std_exception",rapidjson::Value(e.what(),_dallocator), _dallocator);
}


TestValidation::VerificatonCodeOp& TestValidation::VerificatonCodeOp::Verify(){
	if(!_request.HasMember("vcode")||!_request.HasMember("password")){
		setStatusAndMessage(_response,false,"Code error!!");
		return *this;
	}

	//判断验证码是否过期
	lastSessionTime=GetLastSessionTime("id","verificationcode");
	constexpr int VCODE_EXPIRE_TIME=5*60;
	if(std::difftime(std::time(nullptr), lastSessionTime) >VCODE_EXPIRE_TIME){
		setSAMSessionError();
		return *this;
	}

	std::stringstream stm;
	//判断验证码是否正确
	stm<<"select vcode from verificationcode where id='"<<_request["id"].GetString()<<"'";
	auto vcode=BaseDao(SQLITE_MAIN_NAME,stm.str().c_str(),OpStatus);
	if(!OpStatus||std::strcmp(vcode["sqlite"][0]["vcode"].GetString(),_request["vcode"].GetString())!=0){
		setStatusAndMessage(_response,false,"Code error!!");
		return *this;
	}

	//重新写人密码
	stm.str("");
	stm<<"update userinfo set password = '"<<_request["password"].GetString()
		<<"' where id='"<<_request["id"].GetString()<<"'";
	BaseDao(SQLITE_MAIN_NAME,stm.str().c_str(),OpStatus);
	if(OpStatus){
		setStatusAndMessage(_response,true,"Password change successfully.");
	}

	return *this;
}

//解析multipart-form数据
TestValidation::UploadOp::UploadOp(const char* multiform){
	doc.SetObject();
	std::string mf=multiform;
	std::regex re("name=\\\"(.*)\\\"\\r\\n\\r\\n(.*)\\r\\n");
	std::smatch m;

	//循环查找
	auto pos=mf.cbegin();
	auto end=mf.cend();

	size_t numOfKey=0;

	rapidjson::Document _tmp;
	_tmp.SetObject();
	//将一个文件对应的属性组成一个JSON对象，然后整体组成一个数组
	for(;std::regex_search(pos,end,m,re);pos=m.suffix().first){
		numOfKey+=1;
		_tmp.AddMember(rapidjson::Value(m.str(1).c_str(),_tmp.GetAllocator()),rapidjson::Value(m.str(2).c_str(),_tmp.GetAllocator()),_tmp.GetAllocator());	
	}

	if(sessionid.empty()){
		validated=false;
		setStatusAndMessage(doc,false,"No sessionid key found!");
		return;
	}else{
		//根据sessionid来找到id
		std::stringstream stm;
		stm.str("");
		stm<<"{\"sessionid\":\""<<sessionid<<"\"}";
		BaseOp bp(stm.str().c_str());
		time_t lasttime=bp.GetLastSessionTime("sessionid");
		if (std::difftime(std::time(nullptr), lasttime) > SVANDEX_SESSION_EXPIRED)
		{
			validated=false;
			setStatusAndMessage(doc,false,"Session Expired!");
			doc.AddMember("expired",true,doc.GetAllocator());
			return;
		}
		doc.AddMember("expired",false,doc.GetAllocator());
		//sessionid是否过期
		stm.str("");
		stm<<"select id from userinfo where sessionid = '"<<sessionid<<"'";
		auto result=UploadDao(SQLITE_MAIN_NAME,stm.str().c_str(),UploadStatus);
		if(UploadStatus&&result["sqlite"].GetArray().Size()>0){
			doc.AddMember("id",rapidjson::Value(result["sqlite"][0]["id"].GetString(),doc.GetAllocator()),doc.GetAllocator());	
		}else{
			validated=false;
			setStatusAndMessage(doc,false,"No User found!");
			return;	
		}
	}

	doc.AddMember("upload.number",(int)(numOfKey/5),doc.GetAllocator());

	if(doc["upload.number"].GetInt64()>10){
		doc.AddMember("alert","Up to only 10 files will be uploaded.",doc.GetAllocator());	
	}

	rapidjson::Value arr(rapidjson::kArrayType);
	for(size_t index=1;index<=(size_t)doc["upload.number"].GetInt64();index++){
		//临时Object存储对象
		rapidjson::Value oneObj(rapidjson::kObjectType);

		std::string keyStr="file"+std::to_string(index)+".";	
		if(_tmp.HasMember((keyStr+"name").c_str()) &&
			_tmp.HasMember((keyStr+"content_type").c_str()) &&
			_tmp.HasMember((keyStr+"path").c_str()) &&
			_tmp.HasMember((keyStr+"md5").c_str()) &&
			_tmp.HasMember((keyStr+"size").c_str())
		){
			oneObj.AddMember("name",rapidjson::Value(_tmp[(keyStr+"name").c_str()].GetString(),doc.GetAllocator()),doc.GetAllocator());	
			oneObj.AddMember("content_type",rapidjson::Value(_tmp[(keyStr+"content_type").c_str()].GetString(),doc.GetAllocator()),doc.GetAllocator());	
			oneObj.AddMember("path",rapidjson::Value(_tmp[(keyStr+"path").c_str()].GetString(),doc.GetAllocator()),doc.GetAllocator());	
			oneObj.AddMember("md5",rapidjson::Value(_tmp[(keyStr+"md5").c_str()].GetString(),doc.GetAllocator()),doc.GetAllocator());	
			oneObj.AddMember("size",rapidjson::Value(_tmp[(keyStr+"size").c_str()].GetString(),doc.GetAllocator()),doc.GetAllocator());	

			//加入到数组中
			arr.PushBack(oneObj.Move(),doc.GetAllocator());
		}else{
			validated=false;
			setStatusAndMessage(doc,false,"form-data key should start with file!Such as file1, file2");
			return;	
		}
	}
	doc.AddMember("metadata",arr.Move(),doc.GetAllocator());

	//获取字符串大小
	rapidjson::StringBuffer bf;
	rapidjson::Writer<rapidjson::StringBuffer> w(bf);
	doc.Accept(w);

	//最少的字符串也是2，比如空JSON对象就是"{}"
	if(std::strlen(bf.GetString())>2){
		validated=true;
	}else{
		validated=false;
	}
}

TestValidation::UploadOp& TestValidation::UploadOp::ToDataBase(){
	if(!validated){
		setStatusAndMessage(doc,false,"Client data format error.");
		return *this;
	}

	if(!UploadDao){
		setStatusAndMessage(doc,false,"Database Interface error.");
		return *this;
	}

	//写入到upload_files_metadata的表中

	for(auto& ele:doc["metadata"].GetArray()){
		std::stringstream stm;

		//检查md5是否已经存在
		stm<<"select * from upload_files_metadata where md5='"<<
			ele["md5"].GetString()<<"'";
		auto result=UploadDao(SQLITE_MAIN_NAME,stm.str().c_str(),UploadStatus);
		if(result["sqlite"].GetArray().Size()>0){
			setStatusAndMessage(doc,true,(std::string("md5 of file ")+ele["name"].GetString()+" already exist in database.").c_str());
		}else{
			stm.str("");
			stm<<"insert into upload_files_metadata values('"
				<<ele["name"].GetString()<<"','"
				<<ele["content_type"].GetString()<<"','"
				<<ele["path"].GetString()<<"','"
				<<ele["md5"].GetString()<<"',"
				<<std::stoi(ele["size"].GetString())<<",'"
				<<doc["id"].GetString()<<"')";
			UploadDao(SQLITE_MAIN_NAME,stm.str().c_str(),UploadStatus);
			if(!UploadStatus){
				setStatusAndMessage(doc,false,"Database execution error.");
				return *this;
			}else{
				setStatusAndMessage(doc,true,"Upload Succeed.");
			}
		}
	}

	return *this;
}
