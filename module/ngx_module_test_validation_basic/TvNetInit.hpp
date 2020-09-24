#ifndef TVNETINIT_HPP
#define TVNETINIT_HPP

#include "TvNetConf.hpp"
#include "TvNetHandler.hpp"

class TvNetInit final
{
    public:
	typedef TvNetConf conf_type;
	typedef TvNetHandler handler_type;
	typedef TvNetInit this_type;
    public:
	static ngx_command_t* cmds(){
		static ngx_command_t n[]={
			//数据库根目录
			{
				ngx_string("database_root_dir"),
				NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
				&this_type::tv_handler,
				NGX_HTTP_LOC_CONF_OFFSET,
				offsetof(conf_type,database_root_dir),
				nullptr
			},
			//上传文件所在的根目录
			{
				ngx_string("upload_root_dir"),
				NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
				&this_type::tv_handler,
				NGX_HTTP_LOC_CONF_OFFSET,
				offsetof(conf_type,upload_root_dir),
				nullptr
			},
			ngx_null_command

	    };
	    return n;
	}

	static ngx_http_module_t* ctx(){
	    static ngx_http_module_t c={
		NGX_MODULE_NULL(6),
		&conf_type::create,
		NGX_MODULE_NULL(1)
	    };

	    return &c;
	}

	static const ngx_module_t& module(){
	    static ngx_module_t m={
		NGX_MODULE_V1,

		ctx(),
		cmds(),

		NGX_HTTP_MODULE,
		NGX_MODULE_NULL(7),
		NGX_MODULE_V1_PADDING
	    };

	    return m;
	}

	private:
	static char* tv_handler(ngx_conf_t* cf, ngx_command_t* cmd,void* conf){
		auto rc=ngx_conf_set_str_slot(cf,cmd,conf);

		if(rc!=NGX_CONF_OK){
			return rc;
		}

		NgxHttpCoreModule::handler(cf,&handler_type::handler_normal);

		return NGX_CONF_OK;
	}

};


#endif
