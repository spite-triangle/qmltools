#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <string>
#include <sstream>
#include <stdio.h>
#include <thread>
#include <chrono>

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

#define RAII_DEFER(code) \
    std::shared_ptr<void> __raii(nullptr, [&](void*){\
        code;\
    })
    // 

#define BLOCK \
    for(bool __i=false ; __i != true; __i = true)

#define BLOCK_TRY(n,msec) \
    int __j = 0, __e=n; for(__j=0; __j < __e; (std::this_thread::sleep_for(std::chrono::microseconds(msec)),++__j)) 

#define TRY_BREAK break
#define TRY_CONTINUE continue

#define TRY_FCK if(__j > __e) 
#define TRY_OK if(__j < __e)

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

    extern bool MakeDirectory(const std::string & directory);

    extern std::string QStringToUtf8(const QString & str);
    extern std::string QStringToLocal(const QString & str);
    extern QString Utf8ToQString(const std::string & str);
    extern QString LocalToQString(const std::string & str);
    extern QString ToQString(const std::string & str);
    extern std::string ToStdString(const QString & str);
} // namespace OwO



#endif /* UTILS_H */
