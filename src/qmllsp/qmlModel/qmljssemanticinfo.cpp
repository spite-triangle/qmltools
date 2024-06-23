// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljssemanticinfo.h"

#include <qmljs/qmljslink.h>
#include <qmljs/jsoncheck.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <utils/qtcsettings.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace {

Q_LOGGING_CATEGORY(qmllsLog, "qtc.qmlls.editor", QtWarningMsg);

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 100,
    UPDATE_OUTLINE_INTERVAL = 500
};

struct Declaration
{
    QString text;
    int startLine = 0;
    int startColumn = 0;
    int endLine = 0;
    int endColumn = 0;
};

static Utils::QtcSettings createUserSettings()
{
    return Utils::QtcSettings(QSettings::IniFormat,
                                  QSettings::UserScope,
                                  QLatin1String("QtProject"),
                                  QLatin1String("QtCreator"));
}


class FindIdDeclarations: protected Visitor
{
public:
    using Result = QHash<QString, QList<SourceLocation> >;

    Result operator()(Document::Ptr doc)
    {
        _ids.clear();
        _maybeIds.clear();
        if (doc && doc->qmlProgram())
            doc->qmlProgram()->accept(this);
        return _ids;
    }

protected:
    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (!id->name.isEmpty())
                text += id->name.toString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node) { AST::Node::accept(node, this); }

    using Visitor::visit;
    using Visitor::endVisit;

    bool visit(AST::UiScriptBinding *node) override
    {
        if (asString(node->qualifiedId) == QLatin1String("id")) {
            if (auto stmt = AST::cast<const AST::ExpressionStatement*>(node->statement)) {
                if (auto idExpr = AST::cast<const AST::IdentifierExpression *>(stmt->expression)) {
                    if (!idExpr->name.isEmpty()) {
                        const QString &id = idExpr->name.toString();
                        QList<SourceLocation> *locs = &_ids[id];
                        locs->append(idExpr->firstSourceLocation());
                        locs->append(_maybeIds.value(id));
                        _maybeIds.remove(id);
                        return false;
                    }
                }
            }
        }

        accept(node->statement);

        return false;
    }

    bool visit(AST::IdentifierExpression *node) override
    {
        if (!node->name.isEmpty()) {
            const QString &name = node->name.toString();

            if (_ids.contains(name))
                _ids[name].append(node->identifierToken);
            else
                _maybeIds[name].append(node->identifierToken);
        }
        return false;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visiting AST in FindIdDeclarations");
    }

private:
    Result _ids;
    Result _maybeIds;
};

class FindDeclarations: protected Visitor
{
    QList<Declaration> _declarations;
    int _depth;

public:
    QList<Declaration> operator()(AST::Node *node)
    {
        _depth = -1;
        _declarations.clear();
        accept(node);
        return _declarations;
    }

protected:
    using Visitor::visit;
    using Visitor::endVisit;

    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (!id->name.isEmpty())
                text += id->name.toString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node) { AST::Node::accept(node, this); }

    void init(Declaration *decl, AST::UiObjectMember *member)
    {
        const SourceLocation first = member->firstSourceLocation();
        const SourceLocation last = member->lastSourceLocation();
        decl->startLine = first.startLine;
        decl->startColumn = first.startColumn;
        decl->endLine = last.startLine;
        decl->endColumn = last.startColumn + last.length;
    }

    void init(Declaration *decl, AST::ExpressionNode *expressionNode)
    {
        const SourceLocation first = expressionNode->firstSourceLocation();
        const SourceLocation last = expressionNode->lastSourceLocation();
        decl->startLine = first.startLine;
        decl->startColumn = first.startColumn;
        decl->endLine = last.startLine;
        decl->endColumn = last.startColumn + last.length;
    }

    bool visit(AST::UiObjectDefinition *node) override
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    void endVisit(AST::UiObjectDefinition *) override
    {
        --_depth;
    }

    bool visit(AST::UiObjectBinding *node) override
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);

        decl.text.append(asString(node->qualifiedId));
        decl.text.append(QLatin1String(": "));

        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    void endVisit(AST::UiObjectBinding *) override
    {
        --_depth;
    }

    bool visit(AST::UiScriptBinding *) override
    {
        ++_depth;

#if 0 // ### ignore script bindings for now.
        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text.append(asString(node->qualifiedId));

        _declarations.append(decl);
#endif

        return false; // more more bindings in this subtree.
    }

    void endVisit(AST::UiScriptBinding *) override
    {
        --_depth;
    }

    bool visit(AST::TemplateLiteral *ast) override
    {
        // avoid? finds function declarations in templates
        AST::Node::accept(ast->expression, this);
        return true;
    }

    bool visit(AST::FunctionExpression *) override
    {
        return false;
    }

    bool visit(AST::FunctionDeclaration *ast) override
    {
        if (ast->name.isEmpty())
            return false;

        Declaration decl;
        init(&decl, ast);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name.toString();

        decl.text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (!it->element->bindingIdentifier.isEmpty())
                decl.text += it->element->bindingIdentifier.toString();

            if (it->next)
                decl.text += QLatin1String(", ");
        }

        decl.text += QLatin1Char(')');

        _declarations.append(decl);

        return false;
    }

    bool visit(AST::PatternElement *ast) override
    {
        if (!ast->isVariableDeclaration() || ast->bindingIdentifier.isEmpty())
            return false;

        Declaration decl;
        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->bindingIdentifier.toString();

        const SourceLocation first = ast->identifierToken;
        decl.startLine = first.startLine;
        decl.startColumn = first.startColumn;
        decl.endLine = first.startLine;
        decl.endColumn = first.startColumn + first.length;

        _declarations.append(decl);

        return false;
    }

    bool visit(AST::BinaryExpression *ast) override
    {
        auto field = AST::cast<const AST::FieldMemberExpression *>(ast->left);
        auto funcExpr = AST::cast<const AST::FunctionExpression *>(ast->right);

        if (field && funcExpr && funcExpr->body && (ast->op == QSOperator::Assign)) {
            Declaration decl;
            init(&decl, ast);

            decl.text.fill(QLatin1Char(' '), _depth);
            decl.text += field->name.toString();

            decl.text += QLatin1Char('(');
            for (FormalParameterList *it = funcExpr->formals; it; it = it->next) {
                if (!it->element->bindingIdentifier.isEmpty())
                    decl.text += it->element->bindingIdentifier.toString();

                if (it->next)
                    decl.text += QLatin1String(", ");
            }
            decl.text += QLatin1Char(')');

            _declarations.append(decl);
        }

        return true;
    }
};

class CreateRanges: protected AST::Visitor
{
    QTextDocument *_textDocument;
    QList<Range> _ranges;

public:
    QList<Range> operator()(QTextDocument *textDocument, Document::Ptr doc)
    {
        _textDocument = textDocument;
        _ranges.clear();
        if (doc && doc->ast() != nullptr)
            doc->ast()->accept(this);
        return _ranges;
    }

protected:
    using AST::Visitor::visit;

    bool visit(AST::UiObjectBinding *ast) override
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    bool visit(AST::UiObjectDefinition *ast) override
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    bool visit(AST::FunctionExpression *ast) override
    {
        _ranges.append(createRange(ast));
        return true;
    }

    bool visit(AST::TemplateLiteral *ast) override
    {
        AST::Node::accept(ast->expression, this);
        return true;
    }

    bool visit(AST::FunctionDeclaration *ast) override
    {
        _ranges.append(createRange(ast));
        return true;
    }

    bool visit(AST::BinaryExpression *ast) override
    {
        auto field = AST::cast<AST::FieldMemberExpression *>(ast->left);
        auto funcExpr = AST::cast<AST::FunctionExpression *>(ast->right);

        if (field && funcExpr && funcExpr->body && (ast->op == QSOperator::Assign))
            _ranges.append(createRange(ast, ast->firstSourceLocation(), ast->lastSourceLocation()));
        return true;
    }

    bool visit(AST::UiScriptBinding *ast) override
    {
        if (auto block = AST::cast<AST::Block *>(ast->statement))
            _ranges.append(createRange(ast, block));
        return true;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visiting AST in CreateRanges");
    }

    Range createRange(AST::UiObjectMember *member, AST::UiObjectInitializer *ast)
    {
        return createRange(member, member->firstSourceLocation(), ast->rbraceToken);
    }

    Range createRange(AST::FunctionExpression *ast)
    {
        return createRange(ast, ast->lbraceToken, ast->rbraceToken);
    }

    Range createRange(AST::UiScriptBinding *ast, AST::Block *block)
    {
        return createRange(ast, block->lbraceToken, block->rbraceToken);
    }

    Range createRange(AST::Node *ast, SourceLocation start, SourceLocation end)
    {
        Range range;

        range.ast = ast;

        range.begin = QTextCursor(_textDocument);
        range.begin.setPosition(start.begin());

        range.end = QTextCursor(_textDocument);
        range.end.setPosition(end.end());

        return range;
    }
};

}

namespace QmlJS {

namespace {

// ### does not necessarily give the full AST path!
// intentionally does not contain lists like
// UiHeaderItemList, SourceElements, UiObjectMemberList
class AstPath: protected AST::Visitor
{
    QList<AST::Node *> _path;
    unsigned _offset;

public:
    QList<AST::Node *> operator()(AST::Node *node, unsigned offset)
    {
        _offset = offset;
        _path.clear();
        accept(node);
        return _path;
    }

protected:
    using AST::Visitor::visit;

    void accept(AST::Node *node)
    {
        if (node)
            node->accept(this);
    }

    bool containsOffset(SourceLocation start, SourceLocation end)
    {
        return _offset >= start.begin() && _offset <= end.end();
    }

    bool handle(AST::Node *ast,
                SourceLocation start, SourceLocation end,
                bool addToPath = true)
    {
        if (containsOffset(start, end)) {
            if (addToPath)
                _path.append(ast);
            return true;
        }
        return false;
    }

    template <class T>
    bool handleLocationAst(T *ast, bool addToPath = true)
    {
        return handle(ast, ast->firstSourceLocation(), ast->lastSourceLocation(), addToPath);
    }

    bool preVisit(AST::Node *node) override
    {
        if (Statement *stmt = node->statementCast())
            return handleLocationAst(stmt);
        else if (ExpressionNode *exp = node->expressionCast())
            return handleLocationAst(exp);
        else if (UiObjectMember *ui = node->uiObjectMemberCast())
            return handleLocationAst(ui);
        return true;
    }

    bool visit(AST::UiQualifiedId *ast) override
    {
        SourceLocation first = ast->identifierToken;
        SourceLocation last;
        for (AST::UiQualifiedId *it = ast; it; it = it->next)
            last = it->identifierToken;
        if (containsOffset(first, last))
            _path.append(ast);
        return false;
    }

    bool visit(AST::UiProgram *ast) override
    {
        _path.append(ast);
        return true;
    }

    bool visit(AST::Program *ast) override
    {
        _path.append(ast);
        return true;
    }

    bool visit(AST::UiImport *ast) override
    {
        return handleLocationAst(ast);
    }

    bool visit(AST::TemplateLiteral *ast) override
    {
        AST::Node::accept(ast->expression, this);
        return true;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth when visiting the AST in AstPath");
    }
};

} // anonmymous

AST::Node *SemanticInfo::rangeAt(int cursorPosition) const
{
    AST::Node *declaringMember = nullptr;

    for (int i = ranges.size() - 1; i != -1; --i) {
        const Range &range = ranges.at(i);

        if (range.begin.isNull() || range.end.isNull()) {
            continue;
        } else if (cursorPosition >= range.begin.position() && cursorPosition <= range.end.position()) {
            declaringMember = range.ast;
            break;
        }
    }

    return declaringMember;
}

// ### the name and behavior of this function is dubious
Node *SemanticInfo::declaringMemberNoProperties(int cursorPosition) const
{
    AST::Node *node = rangeAt(cursorPosition);

    if (auto objectDefinition = cast<const UiObjectDefinition*>(node)) {
        const QStringView name = objectDefinition->qualifiedTypeNameId->name;
        if (!name.isEmpty() && name.at(0).isLower()) {
            QList<AST::Node *> path = rangePath(cursorPosition);
            if (path.size() > 1)
                return path.at(path.size() - 2);
        } else if (name.contains(QLatin1String("GradientStop"))) {
            QList<AST::Node *> path = rangePath(cursorPosition);
            if (path.size() > 2)
                return path.at(path.size() - 3);
        }
    } else if (auto objectBinding = cast<const UiObjectBinding*>(node)) {
        const QStringView name = objectBinding->qualifiedTypeNameId->name;
        if (name.contains(QLatin1String("Gradient"))) {
            QList<AST::Node *> path = rangePath(cursorPosition);
            if (path.size() > 1)
                return path.at(path.size() - 2);
        }
    }

    return node;
}

QList<AST::Node *> SemanticInfo::rangePath(int cursorPosition) const
{
    QList<AST::Node *> path;

    for (const Range &range : std::as_const(ranges)) {
        if (range.begin.isNull() || range.end.isNull())
            continue;
        else if (cursorPosition >= range.begin.position() && cursorPosition <= range.end.position())
            path += range.ast;
    }

    return path;
}

ScopeChain SemanticInfo::scopeChain(const QList<Node *> &path) const
{
    Q_ASSERT(m_rootScopeChain);

    if (path.isEmpty())
        return *m_rootScopeChain;

    ScopeChain scope = *m_rootScopeChain;
    ScopeBuilder builder(&scope);
    builder.push(path);
    return scope;
}

void SemanticInfo::setRootScopeChain(QSharedPointer<const ScopeChain> rootScopeChain)
{
    Q_ASSERT(m_rootScopeChain.isNull());
    m_rootScopeChain = rootScopeChain;
}

QList<AST::Node *> SemanticInfo::astPath(int pos) const
{
    QList<AST::Node *> result;
    if (! document)
        return result;

    AstPath astPath;
    return astPath(document->ast(), pos);
}

AST::Node *SemanticInfo::astNodeAt(int pos) const
{
    const QList<AST::Node *> path = astPath(pos);
    if (path.isEmpty())
        return nullptr;
    return path.last();
}

SemanticInfo::SemanticInfo(ScopeChain *rootScopeChain)
    : m_rootScopeChain(rootScopeChain)
{
}

bool SemanticInfo::isValid() const
{
    if (document && context && m_rootScopeChain)
        return true;

    return false;
}

int SemanticInfo::revision() const
{
    if (document)
        return document->editorRevision();

    return 0;
}



SemanticInfo SemanticInfo::makeNewSemanticInfo(const Document::Ptr &doc, const Snapshot &snapshot, QTextDocumentPtr qdoc)
{
    using namespace QmlJS;

    SemanticInfo semanticInfo;
    semanticInfo.document = doc;
    semanticInfo.snapshot = snapshot;

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    Link link(semanticInfo.snapshot, modelManager->defaultVContext(doc->language(), doc), modelManager->builtins(doc));
    semanticInfo.context = link(doc, &semanticInfo.semanticMessages);

    auto scopeChain = new ScopeChain(doc, semanticInfo.context);
    semanticInfo.setRootScopeChain(QSharedPointer<const ScopeChain>(scopeChain));

/*     if (doc->language() == Dialect::Json) {
        JsonSchema *schema = jsonManager()->schemaForFile(
            doc->fileName().toString());
        if (schema) {
            JsonCheck jsonChecker(doc);
            semanticInfo.staticAnalysisMessages = jsonChecker(schema);
        }
    } else  */
    {
        auto setting = createUserSettings();
        Check checker(doc, semanticInfo.context, &setting);
        semanticInfo.staticAnalysisMessages = checker();
    }

    semanticInfo.doc = qdoc;

    // create the ranges
    CreateRanges createRanges;
    semanticInfo.ranges = createRanges(qdoc.data(), doc);

    // Refresh the ids
    FindIdDeclarations updateIds;
    semanticInfo.idLocations = updateIds(doc);
    return semanticInfo;
}

} // namespace QmlJSTools
