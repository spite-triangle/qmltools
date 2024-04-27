#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <string>
#include <sstream>
#include <stdio.h>

#include <QString>

#define FUNC_SET(type, name, fcn) \
    void set##fcn(type const & other){\
        name = other; \
    }
    // 

#define FUNC_GET(type, name, fcn) \
    type const & get##fcn() const {\
        return name;\
    }
    // 
    
#define FUNC_SET_GET(type, name, fcn) \
    FUNC_SET(type,name,fcn)\
    FUNC_GET(type,name, fcn)


#define BLOCK \
    for(bool __i=false ; __i != true; __i = true)

namespace OwO
{
    template<class ... Args>
    extern std::string Format(Args ... args){
        std::ostringstream os;
        int _[] = {( os << args ,0)...};
        return os.str();
    }

    template<class ... Args>
    extern std::string Formats(const char * format, Args ... args){
        if(sizeof...(args) == 0){ 
            return std::string(format);
        }

        char buff[1024];
        sprintf(buff, format, args...);
        return std::string(buff);
    }

    extern std::string QStringToUtf8(const QString & str);
    extern std::string QStringToLocal(const QString & str);
    extern QString Utf8ToQString(const std::string & str);
    extern QString LocalToQString(const std::string & str);
    extern QString ToQString(const std::string & str);
    extern std::string ToStdString(const QString & str);
} // namespace OwO



#endif /* UTILS_H */
