#include "NgxUser.hpp"
#include "NgxSvandex.hpp"
#define SVANDEX_SESSION_EXPIRED 6000
#define DATABASE_PATH "/mnt/c/Users/svandex/Desktop/TVNET/db/"

rapidjson::Document TestValidation::FetchData::SQL(const char *dname, const char *stm, bool &status) try
{
    rapidjson::Document result;
    result.SetObject();
    rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();

    std::stringstream dataBasePath;
    dataBasePath << DATABASE_PATH << dname << ".db";

    SQLite::Database db(dataBasePath.str().c_str(), SQLite::OPEN_READWRITE);
    SQLite::Statement query(db, stm);

#ifdef TV_SQLITE_LEGACY_API
    size_t _step = 0;
#endif
    rapidjson::Value resultArray(rapidjson::kArrayType);
    resultArray.SetArray();
    while (query.executeStep())
    {
#ifdef TV_SQLITE_LEGACY_API
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
#else
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
        resultArray.PushBack(one_element.Move(), _dallocator);
#endif
    }

#ifdef TV_SQLITE_LEGACY_API
#else
    result.AddMember("sqlite", resultArray.Move(), _dallocator);
#endif

    status = true;
    return std::move(result);
}
catch (std::exception &e)
{
    rapidjson::Document result;
    result.SetObject();
    rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();
    result.AddMember("errmsg", rapidjson::Value(e.what(), _dallocator), _dallocator);
    status = false;
    return std::move(result);
}

rapidjson::Document TestValidation::FetchData::SQL_Legacy(const char *dname, const char *stm, bool &status) try{
    rapidjson::Document result;
    result.SetObject();
    rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();

    std::stringstream dataBasePath;
    dataBasePath << DATABASE_PATH << dname << ".db";

    SQLite::Database db(dataBasePath.str().c_str(), SQLite::OPEN_READWRITE);
    SQLite::Statement query(db, stm);

    size_t _step = 0;
    rapidjson::Value resultArray(rapidjson::kArrayType);
    resultArray.SetArray();
    while (query.executeStep())
    {
#define TV_SQLITE_LEGACY_API

#ifdef TV_SQLITE_LEGACY_API
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
#else
        rapidjson::Value one_element(rapidjson::kObjectType);
        for (int index = 0; index < query.getColumnCount(); index++)
        {
            switch (query.getColumn(index).getType())
            {
            case SQLITE_TEXT :
                one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(query.getColumn(index).getText(), _dallocator), _dallocator);
                break;
            case SQLITE_INTEGER:
                one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(query.getColumn(index).getInt64()).Move(), _dallocator);
                break;
            case SQLITE_FLOAT:
                one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(query.getColumn(index).getDouble()).Move(), _dallocator);
                break;
            default:
                one_element.AddMember(rapidjson::Value(query.getColumnName(index), _dallocator), rapidjson::Value(""), _dallocator);
                break;
            }
        }
        resultArray.PushBack(one_element.Move(), _dallocator);
#endif
    }

#ifdef TV_SQLITE_LEGACY_API
#else
    result.AddMember("sqlite", resultArray.Move(), _dallocator);
#endif

    status = true;
    return std::move(result);
}
catch (std::exception &e)
{
    rapidjson::Document result;
    result.SetObject();
    rapidjson::Document::AllocatorType &_dallocator = result.GetAllocator();
    result.AddMember("errmsg", rapidjson::Value(e.what(), _dallocator), _dallocator);
    status = false;
    return std::move(result);
}

TestValidation::BaseUser::BaseUser(const char *json)
{
    _request.SetObject();
    _response.SetObject();

    if (_request.Parse(json).HasParseError())
    {
        throw std::logic_error("BaseUser Constrution Error!");
    }

    validated = true;
}

bool TestValidation::BaseUser::Existence()
{
    if (!validated)
    {
        return false;
    }

    if (_request.HasMember("id"))
    {
        std::stringstream stm;
        stm << "select * from `userinfo` where `id`='" << _request["id"].GetString() << "'";
        bool status;
        auto result = TestValidation::FetchData::SQL("management", stm.str().c_str(), status);
        if (!status || result["sqlite"].GetArray().Size() == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}

bool TestValidation::LoginUser::SessionExpired()
{
    if (!validated)
    {
        return false;
    }
    //比较登录时产生的sessionid或者客户端发送的sessionid和数据库中的
    if (std::difftime(std::time(nullptr), lastSessionTime) > SVANDEX_SESSION_EXPIRED)
    {
        return true;
    }

    return false;
}

TestValidation::LoginUser &TestValidation::LoginUser::Login()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    //是否有相应的字段
    if (!validated)
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "账号或密码错误", _dallocator);
        return *this;
    }

    //判断用户是否存在
    if (!Existence())
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "用户不存在", _dallocator);
        return *this;
    }

    //判断账号密码是否正确
    std::stringstream stm;
    stm << "select password from userinfo where id=\'" << _request["id"].GetString() << "\'";
    bool status;
    auto result = TestValidation::FetchData::SQL("management", stm.str().c_str(), status);

    //更新登录历史记录
    if (result["sqlite"].GetArray().Size() == 1 && std::strcmp(result["sqlite"][0]["password"].GetString(), _request["password"].GetString()) == 0)
    {
        bool status = UpdateSessionId("id");
	bool statusTime = UpdateLastLoginTime("id");

	if (!status&&!statusTime)
        {
            _response.AddMember("status", false, _dallocator);
            _response.AddMember("message", "SQL执行失败", _dallocator);
        }
        else
        {
            _response.AddMember("status", true, _dallocator);
            _response.AddMember("message", "登录成功", _dallocator);

	    std::stringstream ret_sessionid_stm;
	    ret_sessionid_stm<<"select sessionid from userinfo where id=\'"<<_request["id"].GetString()<<"\'";
	    bool status;
	    auto result = TestValidation::FetchData::SQL("management", ret_sessionid_stm.str().c_str(), status);

	    _response.AddMember("sessionid",rapidjson::Value(result["sqlite"][0]["sessionid"].GetString(),_dallocator),_dallocator);
        }
    }else{
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "密码错误", _dallocator);
        return *this;
    }

    return *this;
}

time_t TestValidation::LoginUser::GetLastSessionTime(const char *field)
{
    bool status;
    std::stringstream stm;
    stm << "select * from userinfo where " << field << " =\'" << _request[field].GetString() << "\'";
    auto result = TestValidation::FetchData::SQL("management", stm.str().c_str(), status);
    if (!status || result["sqlite"].GetArray().Size() == 0)
    {
        return 0;
    }
    else
    {
        std::istringstream lst{result["sqlite"][0]["lastupdatetime"].GetString()};
        if (lst.str() == "expired")
        {
            return std::time(nullptr);
        }
        std::tm t = {};
        lst >> std::get_time(&t, "%Y-%m-%d %T");
        return std::mktime(&t);
    }
}

bool TestValidation::LoginUser::UpdateSessionId(const char* field){
    auto uuid = Svandex::tools::GetUUID();
    std::stringstream updatestream;
    updatestream << "update `userinfo` set `sessionid`=\'" << uuid.c_str() << "\' where " << field << "= \'" << _request[field].GetString() << "\'";

    bool status;
    TestValidation::FetchData::SQL("management", updatestream.str().c_str(), status);
    return status;
}

bool TestValidation::LoginUser::UpdateLastLoginTime(const char* field){
    std::stringstream updatestream;
    updatestream << "update userinfo set `lastupdatetime`=\'" << Svandex::tools::GetCurrentTimeFT().c_str() << "\' where " << field << "= \'" << _request[field].GetString() << "\'";

    bool status;
    TestValidation::FetchData::SQL("management", updatestream.str().c_str(), status);
    return status;
}

TestValidation::RegisterUser &TestValidation::RegisterUser::Register()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    if (Existence())
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "User Has Already Exist", _dallocator);
    }
    else
    {
        //用户不存在
        std::stringstream stm;
        stm << "insert into `userinfo` values(\'"
            << _request["role"].GetString() << "\',\'"
            << _request["id"].GetString() << "\',\'"
            << _request["password"].GetString() << "\',\'"
            << _request["contact"].GetString() << "\',"
            << "0,\'expired\',\'"
            << Svandex::tools::GetCurrentTimeFT().c_str() << "\',\'"
            << _request["name"].GetString()
            << "\')";

        bool status;
        auto result = TestValidation::FetchData::SQL("management", stm.str().c_str(), status);
        if (result["sqlite"].GetArray().Size() == 0)
        {
            _response.AddMember("status", true, _dallocator);
            _response.AddMember("message", "register succeed.", _dallocator);
        }
        else
        {
            _response.AddMember("status", false, _dallocator);
            _response.AddMember("message", "register fail.", _dallocator);
        }
    }
    return *this;
}

TestValidation::DataUser &TestValidation::DataUser::Data()
{
    rapidjson::Document::AllocatorType &_dallocator = _response.GetAllocator();

    if (!validated)
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "没有数据库名称或者SQL语句", _dallocator);
        return *this;
    }

    if (SessionExpired())
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "会话过期", _dallocator);
        return *this;
    }

    bool status;
    auto result = TestValidation::FetchData::SQL(_request["database"].GetString(), _request["statement"].GetString(), status);

    if (!status)
    {
        _response.AddMember("status", false, _dallocator);
        _response.AddMember("message", "SQL执行失败", _dallocator);
    }
    else
    {
        _response.AddMember("data", rapidjson::Value(result["sqlite"], _dallocator), _dallocator);
        _response.AddMember("status", true, _dallocator);
        _response.AddMember("message", "数据返回成功", _dallocator);
    }

    return *this;
}
