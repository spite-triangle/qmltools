#ifndef DEFINES_H
#define DEFINES_H


#include <memory>
#include <string>
#include <stdio.h>

#define FUNC_SET(type, name, fcn) \
    void set##fcn(type const & other){\
        if(other == name) return; \
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

#ifdef __linux__
#define _FILE_NAME_   (strrchr(__FILE__, '/') + 1)
#else
#define _FILE_NAME_   (strrchr(__FILE__, '\\') + 1)
#endif

#ifdef DOC_TEST
#define LOG_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define LOG_DEBUG(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define LOG_WARN(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

#define CONSOLE_ERROR(text,...) printf( "ERROR [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define CONSOLE_WARN(text,...) printf( "DEBUG [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)
#define CONSOLE_DEBUG(text,...) printf( "WARN [%s:%d] " text "\n", _FILE_NAME_ ,__LINE__ ,##__VA_ARGS__)

#else
#define LOG_ERROR(text,...) 
#define LOG_DEBUG(text,...) 
#define LOG_WARN(text,...) 

#define CONSOLE_ERROR(text,...) printf( ">> " text "\n" ,##__VA_ARGS__)
#define CONSOLE_WARN(text,...) printf( ">> " text "\n",##__VA_ARGS__)
#define CONSOLE_DEBUG(text,...) printf( ">> " text "\n",##__VA_ARGS__)
#endif



#define ASSERT_RETURN(cond, msg, ...) \
    if((cond) != true){ \
        LOG_DEBUG(msg); \
        return __VA_ARGS__; \
    }
    // 

#define RAII_DEFER(code) \
    std::shared_ptr<void> __raii(nullptr, [&](void*){\
        code;\
    })
    // 



#endif /* DEFINES_H */
