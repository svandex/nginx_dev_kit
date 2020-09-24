#ifndef TVNETHANDLER_HPP
#define TVNETHANDLER_HPP

#include "HeaderPrecompilation.hpp"
#include "NgxAll.hpp"
#include "TvNetConf.hpp"

//每个URL对应HandlerOfURL的函数对象
using HandlerOfURL=std::function<void(std::string,rapidjson::Document&)>;
using UMap=std::unordered_map<std::string,HandlerOfURL>;

class TvNetHandler final{
	public:
		typedef TvNetModule this_module;
		typedef TvNetHandler this_type;
	public:
		static ngx_int_t handler_normal(ngx_http_request_t *r);
		static void ngx_http_read_post_handler(ngx_http_request_t *r);
	private:
		static void init_handlers();
		static UMap umap;
};

#endif
