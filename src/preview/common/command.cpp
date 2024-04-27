
#include "command.h"

#include <sstream>
#include <stdexcept>
#include <iomanip>

#include "common/previewLog.hpp"

namespace OwO
{    

std::unordered_map<size_t, std::string> Detail::g_mapTypeName = {
            {typeid(std::string).hash_code(), "string"},
            {typeid(int).hash_code(), "int"},
            {typeid(float).hash_code(), "float"}
        };


std::string & Detail::trim(std::string &str)
{
    if (str.empty()) return str;
    str.erase(0,str.find_first_not_of(" "));

    if(str.empty()) return str;
    str.erase(str.find_last_not_of(" ") + 1);

    return str;
}
Values_t Detail::split(const std::string &str, char delim)
{
    Values_t values;

    std::string value;
    std::istringstream sstream(str);
    while (std::getline(sstream, value, delim)) {
        trim(value);
        if(value.empty()) continue;
        values.emplace_back(std::move(value));
    }

    return values;
}

Operator & Operator::setHandle(HandleCallback_t && handle) {
    m_cmd.handle = std::move(handle);
    return *this;
}

Operator::Operator(const std::string &name, const std::string &desc, HandleCallback_t &&handle)
{
    m_cmd.name = name;
    m_cmd.description = desc;
    m_cmd.handle = std::move(handle);
}

Operator::Operator(const std::string &name, const std::string &desc, const std::string &argType, HandleCallback_t &&handle)
{
    m_cmd.name = name;
    m_cmd.description = desc;
    m_cmd.argType = argType;
    m_cmd.handle = std::move(handle);
}

bool Operator::run(const Values_t & args)
{
    LOG_DEBUG("run command %s", m_cmd.name.c_str());

    if((bool)m_cmd.handle == true){
        return m_cmd.handle(args); 
    }

    return true;
}


void CommandManager::parse(std::string in)
{
    std::string & line = Detail::trim(in);

    Values_t args;
    std::string cmd;
    parseLine(line, cmd, args);


    run(cmd, args);
}

std::string CommandManager::usage()
{
    std::ostringstream os;

    os << "commands:"<< std::endl; 

    /* 命令 */
    for (auto & opt : m_commands)
    {
        auto cmd = opt.second->getCommand();
        os  << std::setw(20) << std::left;
        if(cmd.argType.empty()){
            os << "    " + cmd.name;
        }else{
            os << OwO::Format("    ",cmd.name," [",cmd.argType,"]");
        }
        os << cmd.description << "\n";
    }
    auto out = os.str();
    out.erase(out.end() - 1);
    return out;
}

void CommandManager::parseLine(const std::string &line, std::string &cmd, Values_t &args)
{
    Values_t values = Detail::split(line);
    if(values.size() > 0){
        cmd = values[0];
    }

    auto handle = [](std::ostringstream & buff, Values_t & values ,size_t & i, char ch){
        while((++i, i < values.size())) {
            auto & next =  values[i];

            buff << " " << next;
            if(next.back() == ch) break;
        }
    };

    // 将 "" 包裹的合并
    std::ostringstream buff;
    for (size_t i = 1; i < values.size(); i++)
    {
        auto & curr = values[i];

        buff << curr;

        if(curr[0] == '"' && curr.back() != '"'){
            handle(buff, values, i, '"');

        }
        else if(curr[0] == '\'' && curr.back() != '\''){
            handle(buff, values, i, '\'');

        }

        auto arg = buff.str();
        if(arg.size() > 2 && arg[0] == '\'' && arg.back() == '\'' ){
            arg.erase(arg.begin());
            arg.erase(arg.end() - 1);
        }
        if(arg.size() > 2 && arg[0] == '"' && arg.back() == '"' ){
            arg.erase(arg.begin());
            arg.erase(arg.end() - 1);
        }

        args.emplace_back(std::move(arg));
        buff.clear();
    }
}

void CommandManager::run(const std::string &name, const Values_t & args)
try
{
    if(m_commands.count(name) < 0){
        INTERFACE_DEBUG("%s",OwO::Format(name," is invalid command.").c_str());
        return;
    }

    auto opt = m_commands.at(name);
    opt->run(args); 
}catch(const std::exception & e)
{
    INTERFACE_DEBUG("%s",OwO::Format("An exception occurred: ", e.what(), " during runing " ,name).c_str());    
}


Operator::Ptr  CommandManager::add(const std::string & name, const std::string & desc, const std::function<void ()> & func) {

    if(m_commands.count(name) > 0){
        throw std::runtime_error(OwO::Format("double define command ", name));
    }

    auto handle = [func](const Values_t & vals){
        func();
        return true;
    };

    Operator::Ptr opt = std::make_shared<Operator>(name, desc, std::move(handle));
    m_commands.insert(std::make_pair(name, std::move(opt)));

    return opt;
}

}// namespace OwO
