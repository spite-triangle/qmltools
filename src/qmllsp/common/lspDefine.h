#ifndef LSPDEFINE_H
#define LSPDEFINE_H

#include <memory>

#include <QString>

#include "common/json.hpp"

using Json = nlohmann::json;
using JsonPtr = std::shared_ptr<nlohmann::json>;

#endif // LSPDEFINE_H
