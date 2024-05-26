#ifndef JSONUTILS_HPP
#define JSONUTILS_HPP

#include <QVariant>
#include <QJsonObject>
#include <QJsonValueRef>
#include <QJsonDocument>
#include <QJsonParseError>

#include "common/utils.h"
#include "common/lspDefine.h"
#include "common/lspException.hpp"


class JsonUtils{
public:

public:
    JsonUtils(JsonObjectPtr obj) 
        : m_obj(obj)
    {}

    bool load(const QByteArray & bytes){
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(bytes, &error);

        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            m_obj = std::make_shared<QJsonObject>(doc.object());
            return true;
        }

        return false;
    }

    QByteArray dump(){
        if(m_obj == nullptr) return QByteArray();

        QJsonDocument doc(*m_obj);
        return doc.toJson(QJsonDocument::Compact);
    }

    QJsonValueRef valueException(const QString & key) const {
        if(m_obj == nullptr){
            throw JsonValueException("Json object is null.");
        }

        if(m_obj->contains(key) == false){
            throw JsonValueException(OwO::Format("Not found key ", OwO::QStringToUtf8(key), "."));
        }
        return (*m_obj)[key];
    }

private:
    JsonObjectPtr m_obj;
};

#endif // JSONUTILS_HPP
