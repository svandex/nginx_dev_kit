#ifndef __TVNETHANDLER_HPP__
#define __TVNETHANDLER_HPP__

#include "HeaderPrecompilation.hpp"
#include "TvNetConf.hpp"
#include "../../NgxUser.hpp"

class TvNetHandler final{
    public:
	typedef TvNetModule this_module;
	typedef TvNetHandler this_type;
    public:
	static ngx_int_t handler(ngx_http_request_t*r)
	    try{
		//相关对象
		NgxOperation nop;
		NgxRequest req(r);
		NgxResponse resp(r);
		NgxPool pool(r->pool);

		//JSON解析
		rapidjson::Document doc;
		doc.SetObject();

		//返回消息
		ngx_str_t ret=ngx_null_string;
		NgxString retmsg=NgxString(ret);

		//默认URL
		ngx_str_t str_login=ngx_string("/login");
		ngx_str_t str_register=ngx_string("/register");
		ngx_str_t str_exist=ngx_string("/exist");
		ngx_str_t str_sqlite=ngx_string("/sqlite");

		if(req.body().read(ngx_http_request_empty_handler)==NGX_DONE){
		    //默认只读一次，如果没读完直接返回
		    if(req.body()->request_body->rest>0){
			resp.status(NGX_HTTP_CLIENT_ERROR);
			ngx_str_t s=ngx_string("HTTP packet body too large.");
			NgxString msg=NgxString(s);
			resp.length(msg.size());
			resp.send(msg);
			return resp.eof();
		    }
		    if(req.body().bufs()!=NULL){
			ngx_str_t _json=nop.NgxChainToNgxStr(req.body().bufs(),req->pool);

			//返回的ngx_str_t仍然是在ngx_pool_t的内存池中，并不是以/0为结尾字符串
			std::string json;
			json.resize(_json.len);
			memcpy((void*)json.data(),(void*)_json.data,_json.len);
			if(doc.Parse(json.c_str()).HasParseError()){
			    ngx_str_t s=ngx_string("客户端发送的JSON格式解析错误");
			    retmsg=NgxString(s);
			}else{
			    //根据URL来实现不同功能
			    try{
				if(ngx_strncmp(r->uri.data,str_login.data,str_login.len)==0){
				    TestValidation::LoginUser _LoginUser(json.c_str());
				    doc=_LoginUser.Login().Ret();
				    nop.EndReturnValueConstruction(std::move(doc));
				}
				if(ngx_strncmp(r->uri.data,str_register.data,str_register.len)==0){
				    TestValidation::RegisterUser _RegisterUser(json.c_str());
				    doc=_RegisterUser.Register().Ret();
				    nop.EndReturnValueConstruction(std::move(doc));
				}
				if(ngx_strncmp(r->uri.data,str_exist.data,str_exist.len)==0){
				    TestValidation::BaseUser _BaseUser(json.c_str());
				    if(_BaseUser.Existence()){
					nop.AddStringKeyValueToRet("result","true");
				    }else{
					nop.AddStringKeyValueToRet("result","false");
				    }
				    nop.EndReturnValueConstruction(std::move(nop._default_document));
				}
				if(ngx_strncmp(r->uri.data,str_sqlite.data,str_sqlite.len)==0){
				    TestValidation::DataUser _DataUser(json.c_str());
				    doc=_DataUser.Data().Ret();
				    nop.EndReturnValueConstruction(std::move(doc));
				}
			    }catch(std::exception &e){
				nop.AddExceptionMessageToRet(e);
			    }
			    ngx_str_t ret_json=pool.dup(nop._return_string.GetString());
			    retmsg=NgxString(ret_json);
			}

			//发送返回内容
			resp.status(NGX_HTTP_OK);
			resp.length(retmsg.size());
			resp.send(retmsg);
			resp.finalize();
			return resp.eof();
		    }
		}

		resp.status(NGX_HTTP_OK);
		resp.send();
		resp.finalize();
		return resp.eof();
	    }catch(const NgxException& e){
		return e.code();
	    }
};

#endif
