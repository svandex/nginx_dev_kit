#include "HeaderPrecompilation.hpp"
#include "NgxUser.hpp"


void url_login(std::string json,rapidjson::Document& doc){
	TestValidation::LoginOp _LoginOp(json.c_str());
	doc=_LoginOp.Login().Ret();
}

void url_register(std::string json,rapidjson::Document& doc){
	TestValidation::RegisterOp _RegisterOp(json.c_str());
	doc=_RegisterOp.Register().Ret();
}

void url_exist(std::string json,rapidjson::Document& doc){
	TestValidation::BaseOp _BaseOp(json.c_str());
	if(_BaseOp.Existence()){
		doc.AddMember("status",true,doc.GetAllocator());
	}else{
		doc.AddMember("status",false,doc.GetAllocator());
	}
}

void url_data(std::string json,rapidjson::Document& doc){
	TestValidation::DataOp _DataOp(json.c_str());
	doc=_DataOp.Data().Ret();
}

void url_mailfile(std::string json,rapidjson::Document& doc){
	TestValidation::MailFileOp _Op(json.c_str());
	_Op.Construct().SendMail();
	doc=_Op.Ret();
}

void url_vcode(std::string json,rapidjson::Document& doc){
	TestValidation::VerificatonCodeOp _Op(json.c_str());
	_Op.Construct().SendMail();
	doc=_Op.Ret();
}

void url_vpassword(std::string json,rapidjson::Document& doc){
	TestValidation::VerificatonCodeOp _Op(json.c_str());
	_Op.Verify();
	doc=_Op.Ret();
}
