
#include "common/utils.h"

#include <io.h>
#include <direct.h>


#define PATH_DELIMITER '/'

namespace OwO
{
    bool MakeDirectory(const std::string &directory)
    {
        using namespace std;
        std::string folder_builder;
        std::string sub;
        sub.reserve(directory.size());
        for (auto it = directory.begin(); it != directory.end(); ++it) {
            const char c = *it;
            sub.push_back(c);
            if (c == PATH_DELIMITER || it == directory.end() - 1) {
                folder_builder.append(sub);
                if (0 != ::_access(folder_builder.c_str(), 0)) {
                    if (0 != ::_mkdir(folder_builder.c_str())) {
                        return false;
                    }
                }
                sub.clear();
            }
        }
        return true;
    }

    extern std::string QStringToUtf8(const QString & str){
        return str.toStdString().data();
    }

    extern std::string QStringToLocal(const QString & str){
        return str.toLocal8Bit().data();
    }

    extern QString Utf8ToQString(const std::string & str){
        return QString::fromStdString(str.c_str());
    }

    extern QString LocalToQString(const std::string & str){
        return QString::fromLocal8Bit(str.c_str());
    }

    extern QString ToQString(const std::string & str){
#ifdef __linux__
        return QString::fromStdString(str.c_str());
#else
        return QString::fromLocal8Bit(str.c_str());
#endif
    }

    extern std::string ToStdString(const QString & str){
#ifdef __linux__
        return QStringToUtf8(str);
#else
        return QStringToLocal(str);
#endif
    }

} // namespace OwO
