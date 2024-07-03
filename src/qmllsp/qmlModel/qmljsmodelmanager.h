#ifndef QMLJSMANAGER_H
#define QMLJSMANAGER_H

#include <mutex>
#include <QString>

#include "qmljs/qmljsmodelmanagerinterface.h"

#include "utils/filepath.h"

namespace QmlJS
{

class ModelManager : public ModelManagerInterface{
    Q_OBJECT
public:
    ModelManager(QObject * parent = nullptr);

protected:
    virtual QHash<QString, QmlJS::Dialect> languageForSuffix() const override;
    virtual WorkingCopy workingCopyInternal() const override;
private:
    QHash<QString,Dialect> initLanguageForSuffix() const;
};
    
} // namespace QmlJS

#endif // QMLJSMANAGER_H
