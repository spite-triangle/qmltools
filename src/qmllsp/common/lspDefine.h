#ifndef LSPDEFINE_H
#define LSPDEFINE_H

#include <memory>

#include <QUrl>
#include <QString>

#include "common/json.hpp"

using Json = nlohmann::json;
using JsonPtr = std::shared_ptr<nlohmann::json>;

struct POSITION_S{
    uint64_t line;
    uint64_t character;
};

struct RANGE_S{
    POSITION_S start;
    POSITION_S end;
};

struct TEXT_DOCUMENT_ITEM_S{
    std::string uri;
    std::string languageId;
    int64_t version;
    std::string text;
};

enum DIAGNOSTIC_SEVERITY_E : int{
    DS_DEFAULT = 0,
    DS_ERROR = 1,
    DS_WARNING = 2,
    DS_INFORMATION = 3,
    DS_HINT = 4,
};

#endif // LSPDEFINE_H
