#ifndef QMLCOMPLETION_H
#define QMLCOMPLETION_H

#include <functional>

#include <QList>

#include "QmlJS/QmlJSinterpreter.h"

#include "common/utils.h"
#include "common/lspDefine.h"
#include "qmlModel/QmlJSsemanticinfo.h"

namespace QmlJS
{
    class ObjectValue;
    class CompletionContextFinder;
} // namespace QmlJS


class QmlCompletion{
public:
    enum COMPLETION_TYPE_E{
        TYPE_DEFAULT = 0,
        TYPE_TEXT = 1,
        TYPE_METHOD = 2,
        TYPE_FUNCTION = 3,
        TYPE_CONSTRUCTOR = 4,
        TYPE_FIELD = 5,
        TYPE_VARIABLE = 6,
        TYPE_CLASS = 7,
        TYPE_INTERFACE = 8,
        TYPE_MODULE = 9,
        TYPE_PROPERTY = 10,
        TYPE_UNIT = 11,
        TYPE_VALUE = 12,
        TYPE_ENUM = 13,
        TYPE_KEYWORD = 14,
        TYPE_SNIPPET = 15,
        TYPE_COLOR = 16,
        TYPE_FILE = 17,
        TYPE_REFERENCE = 18,
        TYPE_FOLDER = 19,
        TYPE_ENUMMEMBER = 20,
        TYPE_CONSTANT = 21,
        TYPE_STRUCT = 22,
        TYPE_EVENT = 23,
        TYPE_OPERATOR = 24,
        TYPE_TYPEPARAMETER = 25
    }; 
    
    struct COMPLETION_ITEM_S{
        QString text;
        QString detail;
        COMPLETION_TYPE_E type;
    };

    using CompletionItems_t = QList<COMPLETION_ITEM_S>;

public:

    FUNC_SET(QmlJS::SemanticInfo, m_semanticInfo, SemanticInfo);
    FUNC_SET(QmlJS::QTextDocumentPtr, m_currDoc, CurrentDocument);

    void setPosition(const POSITION_S & pos);
    void setCheckPoint(std::function<bool()> && fcn);

    Json complete();

private:
    bool checkPoint();
    
    Json convertToJson(const CompletionItems_t & completions);

    /* 以 pos 为起点，在 document 中向前查找非数字字母的字符 */
    int64_t previousPosWithoutIdentifier(int64_t pos);

    const QmlJS::ObjectValue * findQmlScopeType(const QmlJS::CompletionContextFinder & finder, int64_t pos);
    CompletionItems_t completeStringLiteral(const QmlJS::CompletionContextFinder & finder, const QmlJS::ObjectValue *qmlScopeType);
    CompletionItems_t completeImport(const QmlJS::CompletionContextFinder & finder);
    CompletionItems_t completeOperator(const QmlJS::CompletionContextFinder & finder, const QmlJS::ObjectValue *qmlScopeType, int64_t pos, QChar completionOperator);
    CompletionItems_t completeGlobal(const QmlJS::CompletionContextFinder & finder, const QmlJS::ObjectValue *qmlScopeType, bool isQmlFile);
    CompletionItems_t completeFileName(const QString &relativeBasePath,  const QString &fileName, const QStringList &patterns  = QStringList());
    CompletionItems_t completeUrl(const QString &relativeBasePath, const QString &urlString);

private:
    int64_t m_docPos; // 在文档中的绝对位置
    std::function<bool()> m_fcnCheckPoint;

    QmlJS::QTextDocumentPtr m_currDoc;
    QmlJS::SemanticInfo m_semanticInfo;
};


#endif /* QMLCOMPLETION_H */
