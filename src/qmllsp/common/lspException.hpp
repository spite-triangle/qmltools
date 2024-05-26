#ifndef LSPEXCEPTION_H
#define LSPEXCEPTION_H

#include <string>
#include <stdexcept>


enum EXCEPTION_TYPE_E{
    EXCEPT_RUN_TIME,
    EXCEPT_INTERRUPT,
    EXCEPT_JSON_VALUE,
    EXCEPT_PARSE_REQUEST
};

class LspException : public std::runtime_error{
public:
    explicit LspException(const std::string& error, const EXCEPTION_TYPE_E & type = EXCEPTION_TYPE_E::EXCEPT_RUN_TIME)
        : std::runtime_error(error), m_type(type)
        {}

    EXCEPTION_TYPE_E type(){ return m_type; }

private:
    EXCEPTION_TYPE_E m_type;
};


/**
 * @brief 接收数据异常
 */
class ParseRequestException : public LspException{
public:
    explicit ParseRequestException(const std::string& error)
        : LspException(error, EXCEPTION_TYPE_E::EXCEPT_PARSE_REQUEST){}
};

/**
 * @brief json 无对应值
 */
class JsonValueException : public LspException{
public:
    explicit JsonValueException(const std::string& error)
        : LspException(error, EXCEPTION_TYPE_E::EXCEPT_JSON_VALUE){}
};


/**
 * @brief 任务中断异常
 */
class InterruptException : public LspException{
public:
    explicit InterruptException(const std::string& error)
        : LspException(error, EXCEPT_INTERRUPT){}
};

#endif // LSPEXCEPTION_H
