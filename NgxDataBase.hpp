#ifndef __NGXDATABASE_HPP__
#define __NGXDATABASE_HPP__

#include "HeaderPrecompilation.hpp"
#include <exception>

enum class DataBaseType{
    MYSQL,
    SQLITE
};

template <typename T>
class NgxDataBase{
    public:
	NgxDataBase(const NgxDataBase&)=delete;

    private:
	std::exception e;
};
#endif
