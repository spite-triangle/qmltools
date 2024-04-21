#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <QString>

namespace OwO
{
    template<class ... Args>
    extern std::string Format(Args ... args){
        std::ostringstream os;
        int _[] = {( os << args ,0)...};
        return os.str();
    }

    extern std::string QStringToUtf8(const QString & str);
    extern std::string QStringToGbk(const QString & str);
    extern QString Utf8ToQString(const std::string & str);
    extern QString GbkToQString(const std::string & str);
} // namespace OwO



#endif /* UTILS_H */
