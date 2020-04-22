#ifndef __NGXSVANDEX_HPP__
#define __NGXSVANDEX_HPP__

#include <string>
#include <vector>
#include <ctime>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace Svandex{
    namespace tools{
	std::string GetCurrentTimeFT(){
	    auto t=std::time(nullptr);
	    char strrep[100];
	    std::tm *tms=localtime(&t);

	    std::strftime(strrep,sizeof(strrep),"%F %T",tms);
	    std::string str(strrep);
	    str.shrink_to_fit();
	    return str;
	}
	std::vector<std::string> GetEnvVariable(const char* pEnvName);
	std::string GetUUID(){
	    auto tag =boost::uuids::random_generator()();
	    return boost::uuids::to_string(tag);
	}
    }
}
#endif
