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

    static JsonObjectPtr load(const QByteArray & bytes){
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(bytes, &error);

        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            return std::make_shared<QJsonObject>(doc.object());
        }

        return JsonObjectPtr();
    }

    QByteArray dump(){
        if(m_obj == nullptr) return QByteArray();

        QJsonDocument doc(*m_obj);
        return doc.toJson(QJsonDocument::Compact);
    }

    QVariant valueException(const QString & key) const {
        if(m_obj == nullptr){
            throw JsonValueException("Json object is null.");
        }

        if(m_obj->contains(key) == false){
            throw JsonValueException(OwO::Format("Not found key ", OwO::QStringToUtf8(key), "."));
        }
        return (*m_obj)[key].toVariant();
    }

private:
    JsonObjectPtr m_obj;
};

#endif // JSONUTILS_HPP
