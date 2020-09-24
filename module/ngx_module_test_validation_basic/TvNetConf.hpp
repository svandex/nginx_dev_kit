#ifndef TVNETCONF_HPP
#define TVNETCONF_HPP

#include "NgxAll.hpp"

class TvNetConf final{
	public:
		typedef TvNetConf this_type;
	public:
		TvNetConf()=default;
		~TvNetConf()=default;

		ngx_str_t database_root_dir;
		ngx_str_t upload_root_dir;

		static void* create(ngx_conf_t* cf){
			return NgxPool(cf).alloc<this_type>();
		}

		static this_type& cast(void* conf){
			return *reinterpret_cast<this_type*>(conf);
		}
};

NGX_MOD_INSTANCE(TvNetModule,ngx_http_test_validation_basic_module,TvNetConf)

#endif
