#ifndef __NGXDATABASE_HPP__
#define __NGXDATABASE_HPP__

#include "HeaderPrecompilation.hpp"
#include <exception>

enum class DataBaseType{
	MYSQL,
	SQLITE
};

enum class style{
	legacy,
	modern
};

class NgxDataBase{
	public:
		NgxDataBase()=delete;
		NgxDataBase(const char* _dir){
			root_dir=_dir;
		}

		//函数对象传递，执行数据库操作
		template<::style=style::modern>
			rapidjson::Document operator() (const char*,const char*,bool&); 

	private:
		std::string root_dir;
};

template<>
rapidjson::Document NgxDataBase::operator()<style::legacy>(const char* dname,const char* stm,bool& status)try{
	rapidjson::Document result;
	result.SetObject();

	rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();

	std::stringstream dataBasePath;
	dataBasePath << root_dir.c_str()<<"/"<< dname << ".db";

	SQLite::Database db(dataBasePath.str().c_str(), SQLite::OPEN_READWRITE);
	SQLite::Statement query(db, stm);

	//步数做为key进行查询，表示第几行
	size_t _step = 0;
	rapidjson::Value resultArray(rapidjson::kArrayType);
	resultArray.SetArray();
	while (query.executeStep())
	{
		rapidjson::Value one_element(rapidjson::kArrayType);
		for (int index = 0; index < query.getColumnCount(); index++)
		{
			switch (query.getColumn(index).getType())
			{
				case SQLITE_TEXT:
					{
						one_element.PushBack(rapidjson::Value(query.getColumn(index).getString().c_str(), _dallocator).Move(), _dallocator);
						break;
					}
				case SQLITE_INTEGER:
					{
						one_element.PushBack(rapidjson::Value(std::to_string(query.getColumn(index).getInt64()).c_str(), _dallocator).Move(), _dallocator);
						break;
					}
				case SQLITE_FLOAT:
					{
						one_element.PushBack(rapidjson::Value(std::to_string(query.getColumn(index).getDouble()).c_str(), _dallocator).Move(), _dallocator);
						break;
					}
				default:
					{
						one_element.PushBack(rapidjson::Value(""), _dallocator);
					}
					break;
			}
		}
		result.AddMember(rapidjson::Value(std::to_string(_step).c_str(), _dallocator), one_element.Move(), _dallocator);
		_step++;

	}
	status=true;
	return result;
}
catch (std::exception &e)
{
	rapidjson::Document result;
	result.SetObject();
	rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();
	result.AddMember("errmsg", rapidjson::Value(e.what(), _dallocator), _dallocator);
	status = false;
	return result;
}

template<>
rapidjson::Document NgxDataBase::operator()<style::modern>(const char* dname,const char* stm,bool& status)try{
	rapidjson::Document result;
	result.SetObject();
	rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();

	std::stringstream dataBasePath;
	dataBasePath << root_dir.c_str() <<"/"<< dname << ".db";

	SQLite::Database db(dataBasePath.str().c_str(), SQLite::OPEN_READWRITE);
	SQLite::Statement query(db, stm);

	//最终结果是一个数组
	rapidjson::Value resultArray(rapidjson::kArrayType);
	resultArray.SetArray();
	while (query.executeStep())
	{
		//一次查询获得的所有数据放入一个one_element中
		rapidjson::Value one_element(rapidjson::kObjectType);
		for (int index = 0; index < query.getColumnCount(); index++)
		{
			switch (query.getColumn(index).getType())
			{
				case SQLITE_TEXT :
					one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(query.getColumn(index).getText(), _dallocator), _dallocator);
					break;
				case SQLITE_INTEGER:
					one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(query.getColumn(index).getInt()).Move(), _dallocator);
					break;
				case SQLITE_FLOAT:
					one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(query.getColumn(index).getDouble()).Move(), _dallocator);
					break;
				default:
					one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(""), _dallocator);
					break;
			}
		}

		//将每次查询结果one_elemnt作为一个元素放入数组中
		resultArray.PushBack(one_element.Move(), _dallocator);
	}

	result.AddMember("sqlite", resultArray.Move(), _dallocator);
	status=true;
	return result;

}
catch (std::exception &e)
{
	rapidjson::Document result;
	result.SetObject();
	rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();
	result.AddMember("errmsg", rapidjson::Value(e.what(), _dallocator), _dallocator);
	status = false;
	return result;
}


#endif
