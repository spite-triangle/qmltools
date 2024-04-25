
#include "common/utils.h"
#include "utils.h"


namespace OwO
{

    extern std::string QStringToUtf8(const QString & str){

    return str.toStdString().data();
    }

    extern std::string QStringToGbk(const QString & str){
        return str.toLocal8Bit().data();
    }

    extern QString Utf8ToQString(const std::string & str){
        return QString::fromLatin1(str.c_str());
    }

    extern QString GbkToQString(const std::string & str){
        return QString::fromLocal8Bit(str.c_str());
    }

    extern QString ToQString(const std::string & str){
#ifdef __linux__
        return QString::fromLatin1(str.c_str());
#else
        return QString::fromLocal8Bit(str.c_str());
#endif
    }

    extern std::string ToStdString(const QString & str){
#ifdef __linux__
        return QStringToUtf8(str);
#else
        return QStringToGbk(str);
#endif
    }

} // namespace OwO
