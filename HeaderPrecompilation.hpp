#ifndef HEADERPRECOMPILATION_HPP
#define HEADERPRECOMPILATION_HPP


#include "rapidjson/stringbuffer.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/istreamwrapper.h"

#include "sqlite3.h"
#include "SQLiteCpp/SQLiteCpp.h"

#include "NgxAll.hpp"

#include <sstream>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unordered_map>

constexpr const char* SQLITE_MAIN_NAME="management";
constexpr const char* SQLITE_SECURITY_QUIZ_NAME="securityQuiz";

#endif
