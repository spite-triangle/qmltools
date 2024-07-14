#include "qmlCompletion.h"

#include <QUrl>
#include <QTextCursor>

#include <utils/algorithm.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsbundle.h>
#include <qmljs/qmljsscanner.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljscompletioncontextfinder.h>

#include "common/lspLog.hpp"
#include "common/lspProject.h"
#include "qmlexpressionundercursor.h"
#include "qmlModel/qmlLanguageModel.h"

using namespace QmlJS;

namespace Private
{

bool IsIdentifierChar(const QChar &c, bool atStart, bool acceptDollar)
{
    switch (c.unicode()) {
    case '_':
        return true;
    case '$':
        if (acceptDollar)
            return true;
        return false;

    default:
        if (atStart)
            return c.isLetter();
        else
            return c.isLetterOrNumber();
    }
}

bool IsLiteral(QmlJS::AST::Node *ast)
{
    using namespace QmlJS;
    if (AST::cast<AST::StringLiteral *>(ast))
        return true;
    else if (AST::cast<AST::NumericLiteral *>(ast))
        return true;
    else
        return false;
}

const QmlJS::Value *GetPropertyValue(const QmlJS::ObjectValue *object,
                                           const QStringList &propertyNames,
                                           const QmlJS::ContextPtr &context)
{
    using namespace QmlJS;

    if (propertyNames.isEmpty() || !object)
        return nullptr;

    const Value *value = object;
    for (const QString &name : propertyNames) {
        if (const ObjectValue *objectValue = value->asObjectValue()) {
            value = objectValue->lookupMember(name, context);
            if (!value)
                return nullptr;
        } else {
            return nullptr;
        }
    }
    return value;
}


class PropertyProcessor
{
public:
    virtual void operator()(const Value *base, const QString &name, const Value *value) = 0;
};

class CompleteFunctionCall
{
public:
    CompleteFunctionCall(bool hasArguments = true) : hasArguments(hasArguments) {}
    bool hasArguments;
};

class CompletionAdder : public PropertyProcessor
{
protected:
    QmlCompletion::CompletionItems_t * completions;

public:
    CompletionAdder(QmlCompletion::CompletionItems_t * completions, QmlCompletion::COMPLETION_TYPE_E order) : completions(completions), order(order){}


    void operator()(const Value *base, const QString &name, const Value *value) override
    {
        Q_UNUSED(base)
        // QVariant data;
        QmlCompletion::COMPLETION_ITEM_S item;
        item.text = name;
        item.type = order;

        if (const FunctionValue *func = value->asFunctionValue()) {
            // constructors usually also have other interesting members,
            // don't consider them pure functions and complete the '()'
            // if (!func->lookupMember(QLatin1String("prototype"), nullptr, nullptr, false))
            //     data = QVariant::fromValue(CompleteFunctionCall(func->namedArgumentCount() || func->isVariadic()));
            item.type = QmlCompletion::COMPLETION_TYPE_E::TYPE_FUNCTION;
        }

        completions->push_back(item); 
    }

    QmlCompletion::COMPLETION_TYPE_E order;
};


class LhsCompletionAdder : public CompletionAdder
{
public:
    LhsCompletionAdder( QmlCompletion::CompletionItems_t * completions,
                        QmlCompletion::COMPLETION_TYPE_E order,
                       bool afterOn)
        : CompletionAdder(completions, order)
        , afterOn(afterOn)
    {}

    void operator ()(const Value *base, const QString &name, const Value *) override
    {
        const CppComponentValue *qmlBase = value_cast<CppComponentValue>(base);

        QString itemText = name;
        QString postfix;
        if (!itemText.isEmpty() && itemText.at(0).isLower())
            postfix = QLatin1String(": ");
        if (afterOn)
            postfix = QLatin1String(" {");

        // readonly pointer properties (anchors, ...) always get a .
        if (qmlBase && !qmlBase->isWritable(name) && qmlBase->isPointer(name))
            postfix = QLatin1Char('.');

        itemText.append(postfix);

        QmlCompletion::COMPLETION_ITEM_S item;
        item.text = itemText;
        item.type = order;

        completions->push_back(item);
    }

    bool afterOn;
};


class ProcessProperties: private MemberProcessor
{
    QSet<const ObjectValue *> _processed;
    bool _globalCompletion = false;
    bool _enumerateGeneratedSlots = false;
    bool _enumerateMethods = true;
    const ScopeChain *_scopeChain;
    const ObjectValue *_currentObject = nullptr;
    PropertyProcessor *_propertyProcessor = nullptr;

public:
    ProcessProperties(const ScopeChain *scopeChain)
        : _scopeChain(scopeChain)
    {
    }

    void setGlobalCompletion(bool globalCompletion)
    {
        _globalCompletion = globalCompletion;
    }

    void setEnumerateGeneratedSlots(bool enumerate)
    {
        _enumerateGeneratedSlots = enumerate;
    }

    void setEnumerateMethods(bool enumerate)
    {
        _enumerateMethods = enumerate;
    }

    void operator ()(const Value *value, PropertyProcessor *processor)
    {
        _processed.clear();
        _propertyProcessor = processor;

        processProperties(value);
    }

    void operator ()(PropertyProcessor *processor)
    {
        _processed.clear();
        _propertyProcessor = processor;

        const QList<const ObjectValue *> scopes = _scopeChain->all();
        for (const ObjectValue *scope : scopes)
            processProperties(scope);
    }

private:
    void process(const QString &name, const Value *value)
    {
        (*_propertyProcessor)(_currentObject, name, value);
    }

    bool processProperty(const QString &name, const Value *value, const PropertyInfo &) override
    {
        process(name, value);
        return true;
    }

    bool processEnumerator(const QString &name, const Value *value) override
    {
        if (! _globalCompletion)
            process(name, value);
        return true;
    }

    bool processSignal(const QString &name, const Value *value) override
    {
        if (_globalCompletion || _enumerateMethods)
            process(name, value);
        return true;
    }

    bool processSlot(const QString &name, const Value *value) override
    {
        if (_enumerateMethods)
            process(name, value);
        return true;
    }

    bool processGeneratedSlot(const QString &name, const Value *value) override
    {
        if (_enumerateGeneratedSlots || (_currentObject && _currentObject->className().endsWith(QLatin1String("Keys")))) {
            // ### FIXME: add support for attached properties.
            process(name, value);
        }
        return true;
    }

    void processProperties(const Value *value)
    {
        if (! value)
            return;
        if (const ObjectValue *object = value->asObjectValue())
            processProperties(object);
    }

    void processProperties(const ObjectValue *object)
    {
        if (! object || !Utils::insert(_processed, object))
            return;

        processProperties(object->prototype(_scopeChain->context()));

        _currentObject = object;
        object->processMembers(this);
        _currentObject = nullptr;
    }
};


} // namespace Private



bool QmlCompletion::checkPoint()
{
    if(m_fcnCheckPoint){
        return m_fcnCheckPoint();
    }
    return false;
}

Json QmlCompletion::convertToJson(const CompletionItems_t &completions)
{
    Json res = Json::array();
    for (auto & completion : completions)
    {
        Json item{
            {"label", OwO::QStringToUtf8(completion.text)},
            {"kind", completion.type}
        };
        res.push_back(item);
    }
    
    return res;
}

int64_t QmlCompletion::previousPosWithoutIdentifier(int64_t pos)
{
    QTextDocument* doc = m_semanticInfo.doc.data();

    while (Private::IsIdentifierChar(doc->characterAt(pos - 1), false, false))
        --pos;

    return pos;
}

const QmlJS::ObjectValue *QmlCompletion::findQmlScopeType(const QmlJS::CompletionContextFinder & contextFinder, int64_t pos)
{
    QTextDocument* doc = m_semanticInfo.doc.data();
    Document::Ptr document = m_semanticInfo.document;
    const ContextPtr &context = m_semanticInfo.context;
    const QList<AST::Node *> path = m_semanticInfo.rangePath(pos);

    const ObjectValue *qmlScopeType = nullptr;
    // contextFinder.qmlObjectTypeName : 获取所在域的 obj 名称

    // 解析 qmlScopeType
    if (contextFinder.isInQmlContext()) {
        // find the enclosing qml object
        int i;
        for (i = path.size() - 1; i >= 0; --i) {
            if(checkPoint() == true) return nullptr;

            AST::Node *node = path[i];
            if (AST::cast<AST::UiObjectDefinition *>(node) || AST::cast<AST::UiObjectBinding *>(node)) {
                qmlScopeType = document->bind()->findQmlObject(node);
                if (qmlScopeType)
                    break;
            }
        }

        // grouped property bindings change the scope type
        for (i++; i < path.size(); ++i) {
            if(checkPoint() == true) return nullptr;

            auto objDef = AST::cast<AST::UiObjectDefinition *>(path[i]);
            if (!objDef || !document->bind()->isGroupedPropertyBinding(objDef))
                break;
            const ObjectValue *newScopeType = qmlScopeType;
            for (AST::UiQualifiedId *it = objDef->qualifiedTypeNameId; it; it = it->next) {
                if(checkPoint() == true) return nullptr;

                if (!newScopeType || it->name.isEmpty()) {
                    newScopeType = nullptr;
                    break;
                }
                const Value *v = newScopeType->lookupMember(it->name.toString(), context);
                v = context->lookupReference(v);
                newScopeType = value_cast<ObjectValue>(v);
            }
            if (!newScopeType)
                break;
            qmlScopeType = newScopeType;
        }

        // fallback to getting the base type object
        if (!qmlScopeType)
            qmlScopeType = context->lookupType(document.data(), contextFinder.qmlObjectTypeName());
    }

    return qmlScopeType;
}

QmlCompletion::CompletionItems_t QmlCompletion::completeStringLiteral(const QmlJS::CompletionContextFinder & finder, const ObjectValue *qmlScopeType)
{
    QTextDocument* doc = m_semanticInfo.doc.data();
    Document::Ptr document = m_semanticInfo.document;
    const ContextPtr &context = m_semanticInfo.context;

    // get the text of the literal up to the cursor position
    //QTextCursor tc = textWidget->textCursor();
    QTextCursor tc(doc);
    tc.setPosition(m_docPos);
    QmlExpressionUnderCursor expressionUnderCursor;
    expressionUnderCursor(tc);
    QString literalText = expressionUnderCursor.text();

    // expression under cursor only looks at one line, so multi-line strings
    // are handled incorrectly and are recognizable by don't starting with ' or "
    if (!literalText.isEmpty()
            && literalText.at(0) != QLatin1Char('"')
            && literalText.at(0) != QLatin1Char('\'')) {
        return CompletionItems_t();
    }

    literalText = literalText.mid(1);

    if (finder.isInImport()) {
        QStringList patterns;
        patterns << QLatin1String("*.qml") << QLatin1String("*.js");

        // 获取文件名称 .qml .js
        return completeFileName(document->path().toString(), literalText, patterns);
    }

    // url 属性
    const Value *value = Private::GetPropertyValue(qmlScopeType, finder.bindingPropertyName(), context);
    if ( value != nullptr && value->asUrlValue()) {
        // url 相关的路径
       return completeUrl(document->path().toString(), literalText);
    }

    // ### enum completion?
    return CompletionItems_t();
}

 // currently path-in-stringliteral is the only completion available in imports
QmlCompletion::CompletionItems_t QmlCompletion::completeImport(const QmlJS::CompletionContextFinder & finder)
{
    QTextDocument* doc = m_semanticInfo.doc.data();
    Document::Ptr document = m_semanticInfo.document;
    const ContextPtr &context = m_semanticInfo.context;

    // 根据 project 定位 qmljs 项目信息
    ModelManagerInterface::ProjectInfo pInfo = ModelManagerInterface::instance()->projectInfo(ProjectExplorer::Project::Instance());

    QmlBundle platform = pInfo.extendedBundle.bundleForLanguage(document->language());
    if (!platform.supportedImports().isEmpty()) {
        QTextCursor tc(doc);
        tc.setPosition(m_docPos);
        QmlExpressionUnderCursor expressionUnderCursor;
        expressionUnderCursor(tc);

        QString libVersion = finder.libVersionImport();
        if (!libVersion.isNull()) {

            QStringList completions=platform.supportedImports().complete(libVersion, QString(), PersistentTrie::LookupFlags(PersistentTrie::CaseInsensitive|PersistentTrie::SkipChars|PersistentTrie::SkipSpaces));
            completions = PersistentTrie::matchStrengthSort(libVersion, completions);

            int toSkip = qMax(libVersion.lastIndexOf(QLatin1Char(' '))
                                , libVersion.lastIndexOf(QLatin1Char('.')));
            if (++toSkip > 0) {
                QStringList nCompletions;
                QString prefix(libVersion.left(toSkip));
                nCompletions.reserve(completions.size());
                for (const QString &completion : std::as_const(completions)){
                    if(checkPoint()) return CompletionItems_t();

                    if (completion.startsWith(prefix))
                        nCompletions.append(completion.right(completion.size()-toSkip));
                }
                completions = nCompletions;
            }

            CompletionItems_t items;
            for(auto & text : completions){
                COMPLETION_ITEM_S item;
                item.text = text;
                item.type = COMPLETION_TYPE_E::TYPE_KEYWORD;
                items.push_back(item);
            }
            return items;
        }
    }
    return CompletionItems_t();
}

QmlCompletion::CompletionItems_t QmlCompletion::completeOperator(const QmlJS::CompletionContextFinder &finder, const ObjectValue *qmlScopeType, int64_t pos, QChar completionOperator)
{
    QTextDocument* doc = m_semanticInfo.doc.data();
    Document::Ptr document = m_semanticInfo.document;
    const ContextPtr &context = m_semanticInfo.context;

    const QList<AST::Node *> path = m_semanticInfo.rangePath(m_docPos);
    const ScopeChain &scopeChain = m_semanticInfo.scopeChain(path);

    CompletionItems_t completions;

    // Look at the expression under cursor.
    //QTextCursor tc = textWidget->textCursor();
    QTextCursor tc(doc);
    tc.setPosition(pos - 1);

    QmlExpressionUnderCursor expressionUnderCursor;
    AST::ExpressionNode *expression = expressionUnderCursor(tc);

    if (expression && ! Private::IsLiteral(expression)) {
        // Evaluate the expression under cursor.
        ValueOwner *interp = context->valueOwner();
        const Value *value = interp->convertToObject(scopeChain.evaluate(expression));

        if (value && completionOperator == QLatin1Char('.')) { // member completion
            Private::ProcessProperties processProperties(&scopeChain);
            if (finder.isInLhsOfBinding() && qmlScopeType) {
                Private::LhsCompletionAdder completionAdder(&completions, COMPLETION_TYPE_E::TYPE_PROPERTY, finder.isAfterOnInLhsOfBinding());
                processProperties.setEnumerateGeneratedSlots(true);
                processProperties(value, &completionAdder);
            } else {
                Private::CompletionAdder completionAdder(&completions, COMPLETION_TYPE_E::TYPE_FIELD);
                processProperties(value, &completionAdder);
            }
        } else if (value && completionOperator == QLatin1Char('(') && pos == m_docPos) {
            // function completion
            if (const FunctionValue *f = value->asFunctionValue()) {
                // QString functionName = expressionUnderCursor.text();
                // int indexOfDot = functionName.lastIndexOf(QLatin1Char('.'));
                // if (indexOfDot != -1)
                //     functionName = functionName.mid(indexOfDot + 1);

                // f->optionalNamedArgumentCount()
                // f->isVariadic()

                QStringList namedArguments;
                for (int i = 0; i < f->namedArgumentCount(); ++i){
                    COMPLETION_ITEM_S item;
                    item.text = f->argumentName(i);
                    item.type = COMPLETION_TYPE_E::TYPE_VALUE;
                    completions.push_back(item);
                }
            }
        }
    }

    return completions;
}

QmlCompletion::CompletionItems_t QmlCompletion::completeGlobal(const QmlJS::CompletionContextFinder &finder, const ObjectValue *qmlScopeType, bool isQmlFile)
{
    bool doGlobalCompletion = true;
    bool doQmlKeywordCompletion = true;
    bool doJsKeywordCompletion = true;
    bool doQmlTypeCompletion = false;

    QTextDocument* doc = m_semanticInfo.doc.data();
    Document::Ptr document = m_semanticInfo.document;
    const ContextPtr &context = m_semanticInfo.context;

    const QList<AST::Node *> path = m_semanticInfo.rangePath(m_docPos);
    const ScopeChain &scopeChain = m_semanticInfo.scopeChain(path);

    CompletionItems_t completions;

    if(checkPoint()) return CompletionItems_t();
    if (finder.isInLhsOfBinding() && qmlScopeType) {
        doGlobalCompletion = false;
        doJsKeywordCompletion = false;
        doQmlTypeCompletion = true;

        Private::ProcessProperties processProperties(&scopeChain);
        processProperties.setGlobalCompletion(true);
        processProperties.setEnumerateGeneratedSlots(true);
        processProperties.setEnumerateMethods(false);

        // id: is special
        COMPLETION_ITEM_S item;
        item.text = "id: ";
        item.type = COMPLETION_TYPE_E::TYPE_PROPERTY;
        completions.push_back(item);

        {
            Private::LhsCompletionAdder completionAdder(&completions, COMPLETION_TYPE_E::TYPE_PROPERTY, finder.isAfterOnInLhsOfBinding());
            processProperties(qmlScopeType, &completionAdder);
        }

        if (ScopeBuilder::isPropertyChangesObject(context, qmlScopeType)
                && scopeChain.qmlScopeObjects().size() == 2) {
            Private::CompletionAdder completionAdder(&completions, COMPLETION_TYPE_E::TYPE_FIELD);
            processProperties(scopeChain.qmlScopeObjects().constFirst(), &completionAdder);
        }
    }

    if(checkPoint()) return CompletionItems_t();
    if (finder.isInRhsOfBinding() && qmlScopeType) {
        doQmlKeywordCompletion = false;

        // complete enum values for enum properties
        const Value *value = Private::GetPropertyValue(qmlScopeType, finder.bindingPropertyName(), context);
        if (const QmlEnumValue *enumValue =
                value_cast<QmlEnumValue>(value)) {
            const QString &name = context->imports(document.data())->nameForImportedObject(enumValue->owner(), context.data());
            const QStringList keys = enumValue->keys();
            for (const QString &key : keys) {
                // QString completion;
                // if (name.isEmpty())
                //     completion = QString::fromLatin1("\"%1\"").arg(key);
                // else
                //     completion = QString::fromLatin1("%1.%2").arg(name, key);
                
                COMPLETION_ITEM_S item;
                item.text = key;
                item.type = COMPLETION_TYPE_E::TYPE_ENUMMEMBER;
                completions.push_back(item);
            }
        }
    }

    if (!finder.isInImport() && !finder.isInQmlContext())
        doQmlTypeCompletion = true;

    if(checkPoint()) return CompletionItems_t();
    if (doQmlTypeCompletion) {
        if (const ObjectValue *qmlTypes = scopeChain.qmlTypes()) {
            Private::ProcessProperties processProperties(&scopeChain);
            Private::CompletionAdder completionAdder(&completions, COMPLETION_TYPE_E::TYPE_TYPEPARAMETER);
            processProperties(qmlTypes, &completionAdder);
        }
    }

    if(checkPoint()) return CompletionItems_t();
    if (doGlobalCompletion) {
        // It's a global completion.
        Private::ProcessProperties processProperties(&scopeChain);
        processProperties.setGlobalCompletion(true);
        Private::CompletionAdder completionAdder(&completions, COMPLETION_TYPE_E::TYPE_FIELD);
        processProperties(&completionAdder);
    }

    if(checkPoint()) return CompletionItems_t();
    if (doJsKeywordCompletion) {
        for (auto & keyword : Scanner::keywords())
        {
            COMPLETION_ITEM_S item;
            item.text = keyword;
            item.type = COMPLETION_TYPE_E::TYPE_KEYWORD;
            completions.push_back(item);
        }
    }

    // add qml extra words
    if(checkPoint()) return CompletionItems_t();
    if (doQmlKeywordCompletion && isQmlFile) {
        QStringList lstExtraWords = {"property", "signal", "import", "alias", "readonly"};  

        if (!doJsKeywordCompletion){
            lstExtraWords.append({"default", "function"});
        }

        for (auto & word : lstExtraWords)
        {
            COMPLETION_ITEM_S item;
            item.text = word;
            item.type = COMPLETION_TYPE_E::TYPE_KEYWORD;
            completions.push_back(item);
        }
    }

    return completions;
}

QmlCompletion::CompletionItems_t QmlCompletion::completeFileName(const QString &relativeBasePath, const QString &fileName, const QStringList &patterns)
{
    if(checkPoint()) return CompletionItems_t();

    CompletionItems_t res;
    const QFileInfo fileInfo(fileName);
    QString directoryPrefix;
    if (fileInfo.isRelative())
        directoryPrefix = relativeBasePath + QLatin1Char('/') + fileInfo.path();
    else
        directoryPrefix = fileInfo.path();
    if (!QFileInfo::exists(directoryPrefix))
        return res;

    QDirIterator dirIterator(directoryPrefix,
                             patterns,
                             QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    while (dirIterator.hasNext()) {
        if(checkPoint()) return CompletionItems_t();

        dirIterator.next();

        COMPLETION_ITEM_S item;
        item.text = dirIterator.fileName();
        item.type = COMPLETION_TYPE_E::TYPE_FILE;

        res.append(item);
    }

    return res;
}

QmlCompletion::CompletionItems_t QmlCompletion::completeUrl(const QString &relativeBasePath, const QString &urlString)
{
    const QUrl url(urlString);
    QString fileName;
    if (url.scheme().compare(QLatin1String("file"), Qt::CaseInsensitive) == 0) {
        fileName = url.toLocalFile();
        // should not trigger completion on 'file://'
        if (fileName.isEmpty())
            return CompletionItems_t();
    } else if (url.scheme().isEmpty()) {
        // don't trigger completion while typing a scheme
        if (urlString.endsWith(QLatin1String(":/")))
            return CompletionItems_t();
        fileName = urlString;
    } else {
        return CompletionItems_t();
    }

    return completeFileName(relativeBasePath, fileName);
}

void QmlCompletion::setPosition(const POSITION_S &pos)
{
    ASSERT_RETURN(m_semanticInfo.isValid() == true, "m_semanticInfo isn't valid.");
    auto content = m_semanticInfo.doc->toPlainText();
    m_docPos = QmlLanguageModel::convertPosition( content, pos);
}

void QmlCompletion::setCheckPoint(std::function<bool()> &&fcn)
{
    m_fcnCheckPoint = std::move(fcn); 
}

Json QmlCompletion::complete()
{
    ASSERT_RETURN(m_semanticInfo.isValid() == true, "m_semanticInfo isn't valid.", Json());

    // 标识符的起始位置
    int64_t startPos = previousPosWithoutIdentifier(m_docPos);
    bool bOnIdentifier = (m_docPos == startPos);

    QTextCursor startPositionCursor(m_semanticInfo.doc.data());
    startPositionCursor.setPosition(startPos);
    CompletionContextFinder contextFinder(startPositionCursor);

    // The completionOperator is the character under the cursor or directly before the
    // identifier under cursor. Use in conjunction with onIdentifier. Examples:
    // a + b<complete> -> ' '
    // a +<complete> -> '+'
    // a +b<complete> -> '+'
    QChar completionOperator = 
            (startPos > 0) ? m_semanticInfo.doc->characterAt(startPos - 1) : QChar();

    bool bIsQml = m_semanticInfo.document->language() == Dialect::Qml;

    const ObjectValue *qmlScopeType = nullptr;
    if(contextFinder.isInQmlContext()){
        qmlScopeType = findQmlScopeType(contextFinder, startPos);
        ASSERT_RETURN(qmlScopeType != nullptr, "qmlScopeType != nullptr", Json()); 
    }

    CompletionItems_t completions;
    if(contextFinder.isInStringLiteral()){
        completions = completeStringLiteral(contextFinder, qmlScopeType);
    }else if(contextFinder.isInImport()){
        completions = completeImport(contextFinder);
    }else if(completionOperator == QLatin1Char('.') || 
            (completionOperator == QLatin1Char('(') && 
            !bOnIdentifier)){
        completions = completeOperator(contextFinder, qmlScopeType, startPos, completionOperator);
    }else{
        completions = completeGlobal(contextFinder, qmlScopeType, bIsQml);
    }

    return convertToJson(completions);
}

