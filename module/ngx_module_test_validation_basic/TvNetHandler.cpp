#include "TvNetHandler.hpp"
#include "NgxOperation.hpp"
#include "NgxDataBase.hpp"
#include "NgxUser.hpp"
#include <regex>

UMap TvNetHandler::umap={};

void TvNetHandler::init_handlers(){
	//登陆
	umap.insert({"login",[](std::string json,rapidjson::Document& doc){
			TestValidation::LoginOp _LoginOp(json.c_str());
			doc=_LoginOp.Login().Ret();
			}
		});
	//注册
	umap.insert({"register",[](std::string json,rapidjson::Document& doc){
			TestValidation::RegisterOp _RegisterOp(json.c_str());
			doc=_RegisterOp.Register().Ret();
			}
		});
	//存在
	umap.insert({"exist",[](std::string json,rapidjson::Document& doc){
			TestValidation::BaseOp _BaseOp(json.c_str());
				if(_BaseOp.Existence()){
					doc.AddMember("status",true,doc.GetAllocator());
				}else{
					doc.AddMember("status",false,doc.GetAllocator());
				}
			}
		});
	//数据
	umap.insert({"sysdata",[](std::string json,rapidjson::Document& doc){
			TestValidation::DataOp _DataOp(json.c_str());
			doc=_DataOp.Data().Ret();
			}
		});

	//邮件
	umap.insert({"email",[](std::string json,rapidjson::Document& doc){
			TestValidation::MailFileOp _Op(json.c_str());
			_Op.SetRootDir(doc["upload_dir"].GetString());
			if(_Op.Construct().Status()){
				_Op.SendMail();
			}
			doc=_Op.Ret();
			}
		});

	//验证码
	umap.insert({"vcode",[](std::string json,rapidjson::Document& doc){
			TestValidation::VerificatonCodeOp _Op(json.c_str());
			if(_Op.Construct().Status()){
				_Op.SendMail();
			}
			doc=_Op.Ret();
			}
		});

	//修改密码
	umap.insert({"vpassword",[](std::string json,rapidjson::Document& doc){
			TestValidation::VerificatonCodeOp _Op(json.c_str());
			_Op.Verify();
			doc=_Op.Ret();
			}
		});

	//上传文件
	umap.insert({"upload",[](std::string multiform,rapidjson::Document& doc){
			TestValidation::UploadOp _Op(multiform.c_str());
			doc=_Op.ToDataBase().Ret();
			}
		});
}

ngx_int_t TvNetHandler::handler_normal(ngx_http_request_t*r){
	//相关对象
	NgxRequest req(r);
	NgxResponse resp(r);

	try{
		//注册回调函数
		init_handlers();

		//设置返回头部，返回一定是JSON字符串
		ngx_table_elt_t h;
		h.hash=1;
		ngx_str_set(&h.key,"Content-Type");
		ngx_str_set(&h.value,"application/json");

		//返回报文默认值
		resp.status(NGX_HTTP_OK);
		resp.headers().add(h);

		//单个缓冲区
		//r->request_body_in_single_buf=1;

		ngx_int_t rc;
		//req.body().read(ngx_http_read_post_handler);
		rc=ngx_http_read_client_request_body(req.body(),ngx_http_read_post_handler);
		NgxException::fail(rc>=NGX_HTTP_SPECIAL_RESPONSE,rc);

		//读取正文部分不完整，ngx_http_read_client_request_body返回NGX_AGAIN
		if(rc!=NGX_OK){
			return NGX_DONE; 
		}

		return NGX_OK;
	}catch(const NgxException& e){
		return e.code();
	}
}

void TvNetHandler::ngx_http_read_post_handler(ngx_http_request_t *r){
	//相关对象
	NgxOperation nop;
	NgxRequest req(r);
	NgxResponse resp(r);
	NgxPool pool(r->pool);
	try{
		//获取配置中的变量
		auto& cf=TvNetModule::instance().conf().loc(r);
		NgxString database_dir=cf.database_root_dir;
		NgxString upload_dir=cf.upload_root_dir;

		//初始化数据读取能力
		NgxDataBase db(database_dir.data());
		TestValidation::BaseOp::BaseDao= db;
		TestValidation::UploadOp::UploadDao=db;

		//JSON解析
		rapidjson::Document doc;
		doc.SetObject();

		//返回消息
		ngx_str_t ret=ngx_null_string;
		NgxString retmsg=NgxString(ret);

		std::string uri(reinterpret_cast<const char*>(r->uri.data),r->uri.len);

		if(req.body().bufs()!=NULL){
			ngx_str_t _json=nop.NgxChainToNgxStr(req.body().bufs(),req->pool);

			//返回的ngx_str_t仍然是在ngx_pool_t的内存池中，并不是以/0为结尾字符串
			std::string json;
			json.resize(_json.len);
			memcpy((void*)json.data(),(void*)_json.data,_json.len);

			//发送字符串是否可以解析为JSON
			if(doc.Parse(json.c_str()).HasParseError()&&uri.compare("/upload")!=0){
				ngx_str_t s=ngx_string("{\"status\":false,\"message\":\"input format is not in json format.\"}");
				retmsg=NgxString(s);
			}else{
				if(uri.compare("/upload")==0){
					std::string fulluri(reinterpret_cast<const char*>(r->uri.data));
					std::regex re("sessionid=([\\w|-]*)\\s");
					std::smatch m;
					if(std::regex_search(fulluri,m,re)){
						TestValidation::UploadOp::sessionid=m[1];	
					}
				}
				//将nginx中的配置信息传递到doc中
				doc.AddMember("upload_dir",rapidjson::Value(upload_dir.data(),doc.GetAllocator()),doc.GetAllocator());

				//根据URL来实现不同功能
				try{
					//URI去掉第一个字符
					umap[uri.substr(1)](json,doc);
					nop.EndReturnValueConstruction(std::move(doc));
				}catch(std::exception &e){
					//异常返回
					nop.AddExceptionMessageToRet(e);
					resp.status(NGX_HTTP_INTERNAL_SERVER_ERROR);
				}
				ngx_str_t ret_json=pool.dup(nop._return_string.GetString());
				retmsg=NgxString(ret_json);
			}
		}else{//没有读取到数据
			resp.status(NGX_HTTP_INTERNAL_SERVER_ERROR);
		}
		resp.length(retmsg.size());
		resp.send(retmsg);
	}catch(const NgxException& e){
		resp.status(NGX_HTTP_INTERNAL_SERVER_ERROR);
	}

	//r->main->count--;
	resp.eof();
	resp.finalize();
}
