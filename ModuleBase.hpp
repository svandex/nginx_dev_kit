#include "HeaderPrecompilation.hpp"

class ModuleBase{
    public:
	ModuleBase(){
	    _default_document.SetObject();
	    _default_writer.Reset(_return_string);
	}
	ModuleBase(const ModuleBase&)=delete;
    protected:
	//增加键值对
	void AddStringKeyValueToRet(const char*,const char*);

	//异常信息返回给客户端
	void AddExceptionMessageToRet(std::exception &);

	//确定返回字符串
	void EndReturnValueConstruction(rapidjson::Document&& _return_document){
	    _return_document.Accept(_default_writer);
	}
    protected:
	rapidjson::Document _default_document;
	rapidjson::StringBuffer _return_string;
	rapidjson::Writer<rapidjson::StringBuffer> _default_writer;

};
