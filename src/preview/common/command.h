#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <typeinfo>
#include <stdexcept>
#include <functional>
#include <unordered_map>

#include "common/utils.h"
#include "common/previewLog.h"

namespace OwO
{   

using Values_t = std::vector<std::string>;
using HandleCallback_t = std::function<bool(const Values_t &)>;

namespace Detail{
    extern std::unordered_map<size_t, std::string> g_mapTypeName;

    /* 类型转换 */
    template<class Type>
    static bool convert(const Values_t & in, Type & out);

    // 去除首尾空格
    static std::string & trim(std::string & str);

    /* 拆分字符串 */
    static Values_t split(const std::string & str, char delim = ' ');
} // namespace Detail

/* 类型转换 */
template<class Type>
bool Detail::convert(const Values_t & args, Type & out){
    throw std::runtime_error("For type ", OwO::Format(typeid(Type).name(),", `Detail::convert` is undefined so fails to convert args."));
    return false;
}

template<>
bool Detail::convert<int>(const Values_t & args, int & out){
    try
    {
        out = std::stoi(args[0]); 
    }
    catch(const std::exception& e)
    {
        CONSOLE_ERROR("%s", e.what());
        return false;
    }
    return true;
}

template<>
bool Detail::convert<float>(const Values_t & args, float & out){
    try
    {
        out = std::stof(args[0]); 
    }
    catch(const std::exception& e)
    {
        CONSOLE_ERROR("%s", e.what());
        return false;
    }
    return true;
}

template<>
bool Detail::convert<bool>(const Values_t & args, bool & out){
    std::string  arg = args[0];

    if(arg == "true" || arg == "1"){
        out = true;
    }else if(arg == "false" || arg == "0"){
        out = false; 
    }else{
        return false;
    }
    return true;
}

template<>
bool Detail::convert<std::string>(const Values_t & args, std::string & out){
    out = args[0];
    return true;
}
 
template<>
bool Detail::convert<Values_t>(const Values_t & args, Values_t & out){
    out = args;
    return true;
}


struct COMMAND_S{
    std::string name; // 指令
    std::string description; // 描述
    std::string argType; // 参数类型
    HandleCallback_t handle; // 指令执行器
};


class Operator{
public:
    using Ptr = std::shared_ptr<Operator>;

public:
    FUNC_SET_GET(COMMAND_S, m_cmd, Command);

    Operator(const std::string & name, const std::string & desc, HandleCallback_t && handle);
    Operator(const std::string & name, const std::string & desc, const std::string & argType , HandleCallback_t && handle);
    bool run(const Values_t & args);

    Operator & setHandle(HandleCallback_t && handle); 

private:
    COMMAND_S m_cmd; 
};


class CommandManager{

public:
    /* 添加指令 */
    template<class Type>
    Operator::Ptr add(const std::string & name, const std::string & desc, const std::function<void (const Type& )> & func);
    Operator::Ptr add(const std::string & name, const std::string & desc, const std::function<void ()> & func);

    /* 解析输入 */
    void parse(std::string in);

    /* 帮助手册 */
    std::string usage();

private:
    /* 解析行 */
    void parseLine(const std::string & line, std::string & cmd, Values_t & args);

    /* 运行 */
    void run(const std::string & name, const Values_t & args);

private:
    std::unordered_map<std::string, Operator::Ptr> m_commands;
};


template<class Type>
Operator::Ptr CommandManager::add(const std::string & name, const std::string & desc, const std::function<void (const Type & )> & func){
    if(m_commands.count(name) > 0){
        throw std::runtime_error(OwO::Format("double define command ", name));
    }

    auto handle = [func](const Values_t & vals){
        Type out;

        if(vals.size() <= 0) return false;

        bool bFlag = Detail::convert(vals, out);
        if(bFlag){
            func(out);
        }else{
            INTERFACE_DEBUG("Check your command args, more infomation see `help`.");
        }
        return bFlag;
    };

    auto opt = std::make_shared<Operator>(name, desc, Detail::g_mapTypeName[typeid(Type).hash_code()],std::move(handle));
    m_commands.insert(std::make_pair(name, opt));
    return opt;
}


} // namespace OwO




#endif /* COMMAND_H */
