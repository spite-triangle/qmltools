#ifndef DEBUGGERCLIENT_H
#define DEBUGGERCLIENT_H


#include "interactiveinterpreter.h"
#include "qmlinspectoragent.h"
#include "qmlv8debuggerclientconstants.h"
#include "qmlengineutils.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmldebug/qmldebugconnection.h>
#include <qmldebug/qpacketprotocol.h>


#include <utils/basetreeview.h>
#include <utils/fileinprojectfinder.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTimer>

#include <memory.h>

#define DEBUG_QML 0
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

#define CB(callback) [this](const QVariantMap &r) { callback(r); }
#define CHECK_STATE(s) do { checkState(s, __FILE__, __LINE__); } while (0)

using namespace ProjectExplorer;
using namespace QmlDebug;
using namespace QmlJS;
using namespace Utils;

namespace Debugger::Internal {

enum Exceptions
{
    NoExceptions,
    UncaughtExceptions,
    AllExceptions
};

enum StepAction
{
    Continue,
    StepIn,
    StepOut,
    Next
};

struct QmlV8ObjectData
{
    int handle = -1;
    int expectedProperties = -1;
    QString name;
    QString type;
    QVariant value;
    QVariantList properties;

    bool hasChildren() const
    {
        return expectedProperties > 0 || !properties.isEmpty();
    }
};

using QmlCallback = std::function<void(const QVariantMap &)>;

struct LookupData
{
    QString iname;
    QString name;
    QString exp;
};

using LookupItems = QHash<int, LookupData>; // id -> (iname, exp)

static void setWatchItemHasChildren(WatchItem *item, bool hasChildren)
{
    item->setHasChildren(hasChildren);
    item->valueEditable = !hasChildren;
}

class QmlEnginePrivate : public QmlDebugClient
{
public:
    QmlEnginePrivate(QmlEngine *engine_, QmlDebugConnection *connection)
        : QmlDebugClient("V8Debugger", connection),
          engine(engine_),
          inspectorAgent(engine, connection)
    {}

    void messageReceived(const QByteArray &data) override;
    void stateChanged(State state) override;

    void continueDebugging(StepAction stepAction);

    void evaluate(const QString expr, qint64 context, const QmlCallback &cb);
    void lookup(const LookupItems &items);
    void backtrace();
    void updateLocals(bool focusOnFrame = true);
    void scope(int number, int frameNumber = -1);
    void scripts(int types = 4, const QList<int> ids = QList<int>(),
                 bool includeSource = false, const QVariant filter = QVariant());

    void setBreakpoint(const QString type, const QString target,
                       bool enabled = true,int line = 0, int column = 0,
                       const QString condition = QString(), int ignoreCount = -1);
    void clearBreakpoint(const Breakpoint &bp);

    bool canChangeBreakpoint() const;
    void changeBreakpoint(const Breakpoint &bp, bool enabled);

    void setExceptionBreak(Exceptions type, bool enabled = false);

    void flushSendBuffer();

    void handleBacktrace(const QVariantMap &response);
    void handleLookup(const QVariantMap &response);
    void handleExecuteDebuggerCommand(const QVariantMap &response);
    void handleEvaluateExpression(const QVariantMap &response, const QString &iname, const QString &expr);
    void handleFrame(const QVariantMap &response);
    void handleScope(const QVariantMap &response);
    void handleVersion(const QVariantMap &response);
    StackFrame extractStackFrame(const QVariant &bodyVal);

    bool canEvaluateScript(const QString &script);
    void updateScriptSource(const QString &fileName, int lineOffset, int columnOffset, const QString &source);

    void runCommand(const DebuggerCommand &command, const QmlCallback &cb = QmlCallback());
    void runDirectCommand(const QString &type, const QByteArray &msg = QByteArray());

    void clearRefs() { refVals.clear(); }
    void memorizeRefs(const QVariant &refs);
    QmlV8ObjectData extractData(const QVariant &data) const;
    void insertSubItems(WatchItem *parent, const QVariantList &properties);
    void checkForFinishedUpdate();
    ConsoleItem *constructLogItemTree(const QmlV8ObjectData &objectData);

public:
    QHash<int, QmlV8ObjectData> refVals; // The mapping of target object handles to retrieved values.
    int sequence = -1;
    QmlEngine *engine;
    QHash<int, Breakpoint> breakpointsSync;
    QList<QString> breakpointsTemp;

    LookupItems currentlyLookingUp; // Id -> inames

    //Cache
    QList<int> currentFrameScopes;
    QHash<int, int> stackIndexLookup;

    StepAction previousStepAction = Continue;

    QList<QByteArray> sendBuffer;

    QHash<QString, QTextDocument*> sourceDocuments;
    InteractiveInterpreter interpreter;
    Process process;
    QmlInspectorAgent inspectorAgent;

    QList<quint32> queryIds;
    bool retryOnConnectFail = false;
    bool automaticConnect = false;
    bool unpausedEvaluate = false;
    bool contextEvaluate = false;
    bool supportChangeBreakpoint = false;

    QTimer connectionTimer;
    QmlDebug::QDebugMessageClient *msgClient = nullptr;

    QHash<int, QmlCallback> callbackForToken;

    FileInProjectFinder fileFinder;

    bool skipFocusOnNextHandleFrame = false;

private:
    ConsoleItem *constructLogItemTree(const QmlV8ObjectData &objectData, QList<int> &seenHandles);
    void constructChildLogItems(ConsoleItem *item, const QmlV8ObjectData &objectData,
                             QList<int> &seenHandles);
};

static void updateDocument(IDocument *document, const QTextDocument *textDocument)
{
    if (auto baseTextDocument = qobject_cast<TextDocument *>(document))
        baseTextDocument->document()->setPlainText(textDocument->toPlainText());
}

bool compareConsoleItems(const ConsoleItem *a, const ConsoleItem *b)
{
    if (a == nullptr)
        return true;
    if (b == nullptr)
        return false;
    return a->text() < b->text();
}

static ConsoleItem *constructLogItemTree(const QVariant &result, const QString &key = {})
{
    const bool sorted = settings().sortStructMembers();
    if (!result.isValid())
        return nullptr;

    QString text;
    ConsoleItem *item = nullptr;
    if (result.typeId() == QVariant::Map) {
        if (key.isEmpty())
            text = "Object";
        else
            text = key + " : Object";

        const QMap<QString, QVariant> resultMap = result.toMap();
        QVarLengthArray<ConsoleItem *> children(resultMap.size());
        auto it = children.begin();
        for (auto i = resultMap.cbegin(), end = resultMap.cend(); i != end; ++i) {
            *(it++) = constructLogItemTree(i.value(), i.key());
        }

        // Sort before inserting as ConsoleItem::sortChildren causes a whole cascade of changes we
        // may not want to handle here.
        if (sorted)
            std::sort(children.begin(), children.end(), compareConsoleItems);

        item = new ConsoleItem(ConsoleItem::DefaultType, text);
        for (ConsoleItem *child : std::as_const(children)) {
            if (child)
                item->appendChild(child);
        }

    } else if (result.typeId() == QVariant::List) {
        if (key.isEmpty())
            text = "List";
        else
            text = QString("[%1] : List").arg(key);

        QVariantList resultList = result.toList();
        QVarLengthArray<ConsoleItem *> children(resultList.size());
        for (int i = 0; i < resultList.count(); i++)
            children[i] = constructLogItemTree(resultList.at(i), QString::number(i));

        if (sorted)
            std::sort(children.begin(), children.end(), compareConsoleItems);

        item = new ConsoleItem(ConsoleItem::DefaultType, text);
        for (ConsoleItem *child : std::as_const(children)) {
            if (child)
                item->appendChild(child);
        }
    } else if (result.canConvert(QVariant::String)) {
        item = new ConsoleItem(ConsoleItem::DefaultType, result.toString());
    } else {
        item = new ConsoleItem(ConsoleItem::DefaultType, "Unknown Value");
    }

    return item;
}

void QmlEnginePrivate::updateScriptSource(const QString &fileName, int lineOffset, int columnOffset,
                                          const QString &source)
{
    QTextDocument *document = nullptr;
    if (sourceDocuments.contains(fileName)) {
        document = sourceDocuments.value(fileName);
    } else {
        document = new QTextDocument(this);
        sourceDocuments.insert(fileName, document);
    }

    // We're getting an unordered set of snippets that can even interleave
    // Therefore we've to carefully update the existing document

    QTextCursor cursor(document);
    for (int i = 0; i < lineOffset; ++i) {
        if (!cursor.movePosition(QTextCursor::NextBlock))
            cursor.insertBlock();
    }
    QTC_CHECK(cursor.blockNumber() == lineOffset);

    for (int i = 0; i < columnOffset; ++i) {
        if (!cursor.movePosition(QTextCursor::NextCharacter))
            cursor.insertText(" ");
    }
    QTC_CHECK(cursor.positionInBlock() == columnOffset);

    const QStringList lines = source.split('\n');
    for (QString line : lines) {
        if (line.endsWith('\r'))
            line.remove(line.size() -1, 1);

        // line already there?
        QTextCursor existingCursor(cursor);
        existingCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (existingCursor.selectedText() != line)
            cursor.insertText(line);

        if (!cursor.movePosition(QTextCursor::NextBlock))
            cursor.insertBlock();
    }

    //update open editors
    QString titlePattern = Tr::tr("JS Source for %1").arg(fileName);
    //Check if there are open editors with the same title
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *doc: documents) {
        if (doc->displayName() == titlePattern) {
            updateDocument(doc, document);
            break;
        }
    }
}

bool QmlEnginePrivate::canEvaluateScript(const QString &script)
{
    interpreter.clearText();
    interpreter.appendText(script);
    return interpreter.canEvaluate();
}

void QmlEngine::connectionFailed()
{
    // this is only an error if we are already connected and something goes wrong.
    if (isConnected()) {
        showMessage(Tr::tr("QML Debugger: Connection failed."), StatusBar);
        notifyInferiorSpontaneousStop();
        notifyInferiorIll();
    } else {
        d->connectionTimer.stop();
        connectionStartupFailed();
    }
}

void QmlEngine::checkConnectionState()
{
    if (!isConnected()) {
        closeConnection();
        connectionStartupFailed();
    }
}

bool QmlEngine::isConnected() const
{
    if (QmlDebugConnection *connection = d->connection())
        return connection->isConnected();
    else
        return false;
}

void QmlEngine::showConnectionStateMessage(const QString &message)
{
    if (isDying())
        return;
    showMessage("QML Debugger: " + message, LogStatus);
}

void QmlEngine::logServiceStateChange(const QString &service, float version,
                                      QmlDebugClient::State newState)
{
    switch (newState) {
    case QmlDebugClient::Unavailable: {
        showConnectionStateMessage(QString("Status of \"%1\" Version: %2 changed to 'unavailable'.").
                                   arg(service).arg(version));
        break;
    }
    case QmlDebugClient::Enabled: {
        showConnectionStateMessage(QString("Status of \"%1\" Version: %2 changed to 'enabled'.").
                                    arg(service).arg(version));
        break;
    }

    case QmlDebugClient::NotConnected: {
        showConnectionStateMessage(QString("Status of \"%1\" Version: %2 changed to 'not connected'.").
                                    arg(service).arg(version));
        break;
    }
    }
}

void QmlEngine::logServiceActivity(const QString &service, const QString &logMessage)
{
    showMessage(service + ' ' + logMessage, LogDebug);
}

void QmlEnginePrivate::continueDebugging(StepAction action)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "continue",
    //      "arguments" : { "stepaction" : <"in", "next" or "out">,
    //                      "stepcount"  : <number of steps (default 1)>
    //                    }
    //    }

    DebuggerCommand cmd(CONTINEDEBUGGING);

    if (action == StepIn)
        cmd.arg(STEPACTION, IN);
    else if (action == StepOut)
        cmd.arg(STEPACTION, OUT);
    else if (action == Next)
        cmd.arg(STEPACTION, NEXT);

    runCommand(cmd);

    previousStepAction = action;
}

void QmlEnginePrivate::evaluate(const QString expr, qint64 context, const QmlCallback &cb)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "evaluate",
    //      "arguments" : { "expression"    : <expression to evaluate>,
    //                      "frame"         : <number>,
    //                      "global"        : <boolean>,
    //                      "disable_break" : <boolean>,
    //                      "context"       : <object id>
    //                    }
    //    }

    // The Qt side Q_ASSERTs otherwise. So ignore the request and hope
    // it will be repeated soon enough (which it will, e.g. in updateLocals)
    QTC_ASSERT(unpausedEvaluate || engine->state() == InferiorStopOk, return);

    DebuggerCommand cmd(EVALUATE);

    cmd.arg(EXPRESSION, expr);
    StackHandler *handler = engine->stackHandler();
    if (handler->currentFrame().isUsable())
        cmd.arg(FRAME, handler->currentIndex());

    if (context >= 0)
        cmd.arg(CONTEXT, context);

    runCommand(cmd, cb);
}

void QmlEnginePrivate::handleEvaluateExpression(const QVariantMap &response,
                                                const QString &iname,
                                                const QString &exp)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "evaluate",
    //      "body"        : ...
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    QVariant bodyVal = response.value(BODY).toMap();
    QmlV8ObjectData body = extractData(bodyVal);
    WatchHandler *watchHandler = engine->watchHandler();

    auto item = new WatchItem;
    item->iname = iname;
    item->name = exp;
    item->exp = exp;
    item->id = body.handle;
    bool success = response.value("success").toBool();
    if (success) {
        item->type = body.type;
        item->value = body.value.toString();
        setWatchItemHasChildren(item, body.hasChildren());
    } else {
        //Do not set type since it is unknown
        item->setError(body.value.toString());
    }
    insertSubItems(item, body.properties);
    watchHandler->insertItem(item);
    watchHandler->updateLocalsWindow();
}

void QmlEnginePrivate::lookup(const LookupItems &items)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "lookup",
    //      "arguments" : { "handles"       : <array of handles>,
    //                      "includeSource" : <boolean indicating whether
    //                                          the source will be included when
    //                                          script objects are returned>,
    //                    }
    //    }

    if (items.isEmpty())
        return;

    QList<int> handles;
    for (auto it = items.begin(); it != items.end(); ++it) {
        const int handle = it.key();
        if (!currentlyLookingUp.contains(handle)) {
            currentlyLookingUp.insert(handle, it.value());
            handles.append(handle);
        }
    }

    DebuggerCommand cmd(LOOKUP);
    cmd.arg(HANDLES, handles);
    runCommand(cmd, CB(handleLookup));
}

void QmlEnginePrivate::backtrace()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "backtrace",
    //      "arguments" : { "fromFrame" : <number>
    //                      "toFrame" : <number>
    //                      "bottom" : <boolean, set to true if the bottom of the
    //                          stack is requested>
    //                    }
    //    }

    DebuggerCommand cmd(BACKTRACE);
    runCommand(cmd, CB(handleBacktrace));
}

void QmlEnginePrivate::updateLocals(bool focusOnFrame)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "frame",
    //      "arguments" : { "number" : <frame number> }
    //    }
    skipFocusOnNextHandleFrame = focusOnFrame;

    DebuggerCommand cmd(FRAME);
    cmd.arg(NUMBER, stackIndexLookup.value(engine->stackHandler()->currentIndex()));
    runCommand(cmd, CB(handleFrame));
}

void QmlEnginePrivate::scope(int number, int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scope",
    //      "arguments" : { "number" : <scope number>
    //                      "frameNumber" : <frame number, optional uses selected
    //                                      frame if missing>
    //                    }
    //    }

    DebuggerCommand cmd(SCOPE);
    cmd.arg(NUMBER, number);
    if (frameNumber != -1)
        cmd.arg(FRAMENUMBER, frameNumber);

    runCommand(cmd, CB(handleScope));
}

void QmlEnginePrivate::scripts(int types, const QList<int> ids, bool includeSource,
                               const QVariant filter)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scripts",
    //      "arguments" : { "types"         : <types of scripts to retrieve
    //                                           set bit 0 for native scripts
    //                                           set bit 1 for extension scripts
    //                                           set bit 2 for normal scripts
    //                                         (default is 4 for normal scripts)>
    //                      "ids"           : <array of id's of scripts to return. If this is not specified all scripts are requrned>
    //                      "includeSource" : <boolean indicating whether the source code should be included for the scripts returned>
    //                      "filter"        : <string or number: filter string or script id.
    //                                         If a number is specified, then only the script with the same number as its script id will be retrieved.
    //                                         If a string is specified, then only scripts whose names contain the filter string will be retrieved.>
    //                    }
    //    }

    DebuggerCommand cmd(SCRIPTS);
    cmd.arg(TYPES, types);

    if (!ids.isEmpty())
        cmd.arg(IDS, ids);

    if (includeSource)
        cmd.arg(INCLUDESOURCE, includeSource);

    if (filter.typeId() == QVariant::String)
        cmd.arg(FILTER, filter.toString());
    else if (filter.typeId() == QVariant::Int)
        cmd.arg(FILTER, filter.toInt());
    else
        QTC_CHECK(!filter.isValid());

    runCommand(cmd);
}

static QString targetFile(const FilePath &original)
{
    auto projectTree = ProjectExplorer::ProjectTree::instance();
    auto node = projectTree->nodeForFile(original);

    if (auto resourceNode = dynamic_cast<ProjectExplorer::ResourceFileNode *>(node))
        return QLatin1String("qrc:") + resourceNode->qrcPath();

    return original.fileName();
}

void QmlEnginePrivate::setBreakpoint(const QString type, const QString target,
                                     bool enabled, int line, int column,
                                     const QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setbreakpoint",
    //      "arguments" : { "type"        : <"function" or "script" or "scriptId" or "scriptRegExp">
    //                      "target"      : <function expression or script identification>
    //                      "line"        : <line in script or function>
    //                      "column"      : <character position within the line>
    //                      "enabled"     : <initial enabled state. True or false, default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits to ignore, default value is 0>
    //                    }
    //    }
    if (type == EVENT) {
        QPacket rs(dataStreamVersion());
        rs <<  target.toUtf8() << enabled;
        engine->showMessage(QString("%1 %2 %3")
                            .arg(QString(BREAKONSIGNAL), target, QLatin1String(enabled ? "enabled" : "disabled")), LogInput);
        runDirectCommand(BREAKONSIGNAL, rs.data());

    } else {
        DebuggerCommand cmd(SETBREAKPOINT);
        cmd.arg(TYPE, type);
        cmd.arg(ENABLED, enabled);

        if (type == SCRIPTREGEXP)
            cmd.arg(TARGET, targetFile(FilePath::fromString(target)));
        else
            cmd.arg(TARGET, target);

        if (line)
            cmd.arg(LINE, line - 1);
        if (column)
            cmd.arg(COLUMN, column - 1);
        if (!condition.isEmpty())
            cmd.arg(CONDITION, condition);
        if (ignoreCount != -1)
            cmd.arg(IGNORECOUNT, ignoreCount);

        runCommand(cmd);
    }
}

void QmlEnginePrivate::clearBreakpoint(const Breakpoint &bp)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "clearbreakpoint",
    //      "arguments" : { "breakpoint" : <number of the break point to clear>
    //                    }
    //    }

    DebuggerCommand cmd(CLEARBREAKPOINT);
    cmd.arg(BREAKPOINT, bp->responseId().toInt());
    runCommand(cmd);
}

bool QmlEnginePrivate::canChangeBreakpoint() const
{
    return supportChangeBreakpoint;
}

void QmlEnginePrivate::changeBreakpoint(const Breakpoint &bp, bool enabled)
{
    DebuggerCommand cmd(CHANGEBREAKPOINT);
    cmd.arg(BREAKPOINT, bp->responseId().toInt());
    cmd.arg(ENABLED, enabled);
    runCommand(cmd);
}

void QmlEnginePrivate::setExceptionBreak(Exceptions type, bool enabled)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setexceptionbreak",
    //      "arguments" : { "type"    : <string: "all", or "uncaught">,
    //                      "enabled" : <optional bool: enables the break type if true>
    //                    }
    //    }

    DebuggerCommand cmd(SETEXCEPTIONBREAK);
    if (type == AllExceptions)
        cmd.arg(TYPE, ALL);

    //Not Supported:
    // else if (type == UncaughtExceptions)
    //    cmd.args(TYPE, UNCAUGHT);

    if (enabled)
        cmd.arg(ENABLED, enabled);

    runCommand(cmd);
}

QmlV8ObjectData QmlEnginePrivate::extractData(const QVariant &data) const
{
    //    { "handle" : <handle>,
    //      "type"   : <"undefined", "null", "boolean", "number", "string", "object", "function" or "frame">
    //    }

    //    {"handle":<handle>,"type":"undefined"}

    //    {"handle":<handle>,"type":"null"}

    //    { "handle":<handle>,
    //      "type"  : <"boolean", "number" or "string">
    //      "value" : <JSON encoded value>
    //    }

    //    {"handle":7,"type":"boolean","value":true}

    //    {"handle":8,"type":"number","value":42}

    //    { "handle"              : <handle>,
    //      "type"                : "object",
    //      "className"           : <Class name, ECMA-262 property [[Class]]>,
    //      "constructorFunction" : {"ref":<handle>},
    //      "protoObject"         : {"ref":<handle>},
    //      "prototypeObject"     : {"ref":<handle>},
    //      "properties" : [ {"name" : <name>,
    //                        "ref"  : <handle>
    //                       },
    //                       ...
    //                     ]
    //    }

    //        { "handle" : <handle>,
    //          "type"                : "function",
    //          "className"           : "Function",
    //          "constructorFunction" : {"ref":<handle>},
    //          "protoObject"         : {"ref":<handle>},
    //          "prototypeObject"     : {"ref":<handle>},
    //          "name"                : <function name>,
    //          "inferredName"        : <inferred function name for anonymous functions>
    //          "source"              : <function source>,
    //          "script"              : <reference to function script>,
    //          "scriptId"            : <id of function script>,
    //          "position"            : <function begin position in script>,
    //          "line"                : <function begin source line in script>,
    //          "column"              : <function begin source column in script>,
    //          "properties" : [ {"name" : <name>,
    //                            "ref"  : <handle>
    //                           },
    //                           ...
    //                         ]
    //        }

    QmlV8ObjectData objectData;
    const QVariantMap dataMap = data.toMap();

    objectData.name = dataMap.value(NAME).toString();

    QString type = dataMap.value(TYPE).toString();
    objectData.handle = dataMap.value(HANDLE).toInt();

    if (type == "undefined") {
        objectData.type = "undefined";
        objectData.value = "undefined";

    } else if (type == "null") { // Deprecated. typeof(null) == "object" in JavaScript
        objectData.type = "object";
        objectData.value = "null";

    } else if (type == "boolean") {
        objectData.type = "boolean";
        objectData.value = dataMap.value(VALUE);

    } else if (type == "number") {
        objectData.type = "number";
        objectData.value = dataMap.value(VALUE);

    } else if (type == "string") {
        QChar quote('"');
        objectData.type = "string";
        objectData.value = QString(quote + dataMap.value(VALUE).toString() + quote);

    } else if (type == "object") {
        objectData.type = "object";
        // ignore "className": it doesn't make any sense.

        if (dataMap.contains("value")) {
            QVariant value = dataMap.value("value");
            // The QVariant representation of null has changed across various Qt versions
            // 5.6, 5.7: QVariant::Invalid
            // 5.8: isValid(), !isNull(), type() == 51; only typeName() is unique: "std::nullptr_t"
            // 5.9: isValid(), isNull(); We can then use isNull()
            if (!value.isValid() || value.isNull()
                    || strcmp(value.typeName(), "std::nullptr_t") == 0) {
                objectData.value = "null"; // Yes, null is an object.
            } else if (value.isValid()) {
                objectData.expectedProperties = value.toInt();
            }
        }

        if (dataMap.contains("properties"))
            objectData.properties = dataMap.value("properties").toList();
    } else if (type == "function") {
        objectData.type = "function";
        objectData.value = dataMap.value(NAME);
        objectData.properties = dataMap.value("properties").toList();
        QVariant value = dataMap.value("value");
        if (value.isValid())
            objectData.expectedProperties = value.toInt();

    } else if (type == "script") {
        objectData.type = "script";
        objectData.value = dataMap.value(NAME);
    }

    if (dataMap.contains(REF)) {
        objectData.handle = dataMap.value(REF).toInt();
        if (refVals.contains(objectData.handle)) {
            QmlV8ObjectData data = refVals.value(objectData.handle);
            if (objectData.type.isEmpty())
                objectData.type = data.type;
            if (!objectData.value.isValid())
                objectData.value = data.value;
            if (objectData.properties.isEmpty())
                objectData.properties = data.properties;
            if (objectData.expectedProperties < 0)
                objectData.expectedProperties = data.expectedProperties;
        }
    }

    return objectData;
}

void QmlEnginePrivate::runCommand(const DebuggerCommand &command, const QmlCallback &cb)
{
    QJsonObject object;
    object.insert("seq", ++sequence);
    object.insert("type", "request");
    object.insert("command", command.function);
    object.insert("arguments", command.args);
    if (cb)
        callbackForToken[sequence] = cb;

    runDirectCommand(V8REQUEST, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void QmlEnginePrivate::runDirectCommand(const QString &type, const QByteArray &msg)
{
    // Leave item as variable, serialization depends on it.
    QByteArray cmd = V8DEBUG;

    engine->showMessage(QString("%1 %2").arg(type, QString::fromLatin1(msg)), LogInput);

    QPacket rs(dataStreamVersion());
    rs << cmd << type.toLatin1() << msg;

    if (state() == Enabled)
        sendMessage(rs.data());
    else
        sendBuffer.append(rs.data());
}

void QmlEnginePrivate::memorizeRefs(const QVariant &refs)
{
    if (refs.isValid()) {
        const QList<QVariant> refList = refs.toList();
        for (const QVariant &ref : refList) {
            const QVariantMap refData = ref.toMap();
            int handle = refData.value(HANDLE).toInt();
            refVals[handle] = extractData(refData);
        }
    }
}

void QmlEnginePrivate::messageReceived(const QByteArray &data)
{
    QPacket ds(dataStreamVersion(), data);
    QByteArray command;
    ds >> command;

    if (command == V8DEBUG) {
        QByteArray type;
        QByteArray response;
        ds >> type >> response;

        engine->showMessage(QLatin1String(type), LogOutput);
        if (type == CONNECT) {
            //debugging session started

        } else if (type == INTERRUPT) {
            //debug break requested

        } else if (type == BREAKONSIGNAL) {
            //break on signal handler requested

        } else if (type == V8MESSAGE) {
            SDEBUG(response);
            engine->showMessage(QString(V8MESSAGE) + ' ' + QString::fromLatin1(response), LogOutput);

            const QVariantMap resp = QJsonDocument::fromJson(response).toVariant().toMap();

            const QString type = resp.value(TYPE).toString();

            if (type == "response") {

                const QString debugCommand(resp.value(COMMAND).toString());

                memorizeRefs(resp.value(REFS));

                bool success = resp.value("success").toBool();
                if (!success) {
                    SDEBUG("Request was unsuccessful");
                }

                int requestSeq = resp.value("request_seq").toInt();
                if (callbackForToken.contains(requestSeq)) {
                    callbackForToken[requestSeq](resp);

                } else if (debugCommand == DISCONNECT) {
                    //debugging session ended

                } else if (debugCommand == CONTINEDEBUGGING) {
                    //do nothing, wait for next break

                } else if (debugCommand == SETBREAKPOINT) {
                    //                { "seq"         : <number>,
                    //                  "type"        : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "setbreakpoint",
                    //                  "body"        : { "type"       : <"function" or "script">
                    //                                    "breakpoint" : <break point number of the new break point>
                    //                                  }
                    //                  "running"     : <is the VM running after sending this response>
                    //                  "success"     : true
                    //                }

                    int seq = resp.value("request_seq").toInt();
                    const QVariantMap breakpointData = resp.value(BODY).toMap();
                    const QString index = QString::number(breakpointData.value("breakpoint").toInt());

                    if (breakpointsSync.contains(seq)) {
                        Breakpoint bp = breakpointsSync.take(seq);
                        QTC_ASSERT(bp, return);
                        bp->setParameters(bp->requestedParameters()); // Assume it worked.
                        bp->setResponseId(index);

                        //Is actual position info present? Then breakpoint was
                        //accepted
                        const QVariantList actualLocations =
                                breakpointData.value("actual_locations").toList();
                        const int line = breakpointData.value("line").toInt() + 1;
                        if (!actualLocations.isEmpty()) {
                            //The breakpoint requested line should be same as
                            //actual line
                            if (bp && bp->state() != BreakpointInserted) {
                                bp->setTextPosition({line, -1});
                                bp->setPending(false);
                                engine->notifyBreakpointInsertOk(bp);
                            }
                        }


                    } else {
                        breakpointsTemp.append(index);
                    }


                } else if (debugCommand == CLEARBREAKPOINT) {
                    // DO NOTHING

                } else if (debugCommand == SETEXCEPTIONBREAK) {
                    //                { "seq"               : <number>,
                    //                  "type"              : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "setexceptionbreak",
                    //                  "body"        : { "type"    : <string: "all" or "uncaught" corresponding to the request.>,
                    //                                    "enabled" : <bool: true if the break type is currently enabled as a result of the request>
                    //                                  }
                    //                  "running"     : true
                    //                  "success"     : true
                    //                }


                } else if (debugCommand == SCRIPTS) {
                    //                { "seq"         : <number>,
                    //                  "type"        : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "scripts",
                    //                  "body"        : [ { "name"             : <name of the script>,
                    //                                      "id"               : <id of the script>
                    //                                      "lineOffset"       : <line offset within the containing resource>
                    //                                      "columnOffset"     : <column offset within the containing resource>
                    //                                      "lineCount"        : <number of lines in the script>
                    //                                      "data"             : <optional data object added through the API>
                    //                                      "source"           : <source of the script if includeSource was specified in the request>
                    //                                      "sourceStart"      : <first 80 characters of the script if includeSource was not specified in the request>
                    //                                      "sourceLength"     : <total length of the script in characters>
                    //                                      "scriptType"       : <script type (see request for values)>
                    //                                      "compilationType"  : < How was this script compiled:
                    //                                                               0 if script was compiled through the API
                    //                                                               1 if script was compiled through eval
                    //                                                            >
                    //                                      "evalFromScript"   : <if "compilationType" is 1 this is the script from where eval was called>
                    //                                      "evalFromLocation" : { line   : < if "compilationType" is 1 this is the line in the script from where eval was called>
                    //                                                             column : < if "compilationType" is 1 this is the column in the script from where eval was called>
                    //                                  ]
                    //                  "running"     : <is the VM running after sending this response>
                    //                  "success"     : true
                    //                }

                    if (success) {
                        const QVariantList body = resp.value(BODY).toList();

                        QStringList sourceFiles;
                        for (int i = 0; i < body.size(); ++i) {
                            const QVariantMap entryMap = body.at(i).toMap();
                            const int lineOffset = entryMap.value("lineOffset").toInt();
                            const int columnOffset = entryMap.value("columnOffset").toInt();
                            const QString name = entryMap.value("name").toString();
                            const QString source = entryMap.value("source").toString();

                            if (name.isEmpty())
                                continue;

                            if (!sourceFiles.contains(name))
                                sourceFiles << name;

                            updateScriptSource(name, lineOffset, columnOffset, source);
                        }

                        QMap<QString, FilePath> files;
                        for (const QString &file : std::as_const(sourceFiles)) {
                            QString shortName = file;
                            FilePath fullName = engine->toFileInProject(file);
                            files.insert(shortName, fullName);
                        }

                        engine->sourceFilesHandler()->setSourceFiles(files);
                        //update open editors
                    }
                } else {
                    // DO NOTHING
                }

            } else if (type == EVENT) {
                const QString eventType(resp.value(EVENT).toString());

                if (eventType == "break") {

                    clearRefs();
                    const QVariantMap breakData = resp.value(BODY).toMap();
                    const QString invocationText = breakData.value("invocationText").toString();
                    const QString scriptUrl = breakData.value("script").toMap().value("name").toString();
                    const QString sourceLineText = breakData.value("sourceLineText").toString();

                    bool inferiorStop = true;

                    QList<Breakpoint> v8Breakpoints;

                    const QVariantList v8BreakpointIdList = breakData.value("breakpoints").toList();
                    if (engine->state() != InferiorStopRequested) {
                        // skip debug break if no breakpoint and we have not done a single step as
                        // last action - likely stopped in another file with same naming
                        if (v8BreakpointIdList.isEmpty() && previousStepAction == Continue) {
                            inferiorStop = false;
                            continueDebugging(Continue);
                        }
                    }

                    for (const QVariant &breakpointId : v8BreakpointIdList) {
                        const QString responseId = QString::number(breakpointId.toInt());
                        Breakpoint bp = engine->breakHandler()->findBreakpointByResponseId(responseId);
                        QTC_ASSERT(bp, continue);
                        v8Breakpoints << bp;
                    }

                    if (!v8Breakpoints.isEmpty() && invocationText.startsWith("[anonymous]()")
                            && scriptUrl.endsWith(".qml")
                            && sourceLineText.trimmed().startsWith('(')) {

                        // we hit most likely the anonymous wrapper function automatically generated for bindings
                        // -> relocate the breakpoint to column: 1 and continue

                        int newColumn = sourceLineText.indexOf('(') + 1;

                        for (const Breakpoint &bp : std::as_const(v8Breakpoints)) {
                            QTC_ASSERT(bp, continue);
                            const BreakpointParameters &params = bp->requestedParameters();

                            clearBreakpoint(bp);
                            setBreakpoint(SCRIPTREGEXP,
                                          params.fileName.toString(),
                                          params.enabled,
                                          params.textPosition.line,
                                          newColumn,
                                          params.condition,
                                          params.ignoreCount);
                            breakpointsSync.insert(sequence, bp);
                        }
                        continueDebugging(Continue);
                        inferiorStop = false;
                    }

                    //Skip debug break if this is an internal function
                    if (sourceLineText == INTERNAL_FUNCTION) {
                        continueDebugging(previousStepAction);
                        inferiorStop = false;
                    }

                    if (inferiorStop) {
                        //Update breakpoint data
                        for (const Breakpoint &bp : std::as_const(v8Breakpoints)) {
                            QTC_ASSERT(bp, continue);
                            if (bp->functionName().isEmpty()) {
                                bp->setFunctionName(invocationText);
                            }
                            if (bp->state() != BreakpointInserted) {
                                bp->setTextPosition({breakData.value("sourceLine").toInt() + 1, -1});
                                bp->setPending(false);
                                engine->notifyBreakpointInsertOk(bp);
                            }
                        }

                        if (engine->state() == InferiorRunOk) {
                            for (const Breakpoint &bp : std::as_const(v8Breakpoints)) {
                                QTC_ASSERT(bp, continue);
                                if (breakpointsTemp.contains(bp->responseId()))
                                    clearBreakpoint(bp);
                            }
                            engine->notifyInferiorSpontaneousStop();
                            backtrace();
                        } else if (engine->state() == InferiorStopRequested) {
                            engine->notifyInferiorStopOk();
                            backtrace();
                        }
                    }

                } else if (eventType == "exception") {
                    const QVariantMap body = resp.value(BODY).toMap();
                    int lineNumber = body.value("sourceLine").toInt() + 1;

                    const QVariantMap script = body.value("script").toMap();
                    QUrl fileUrl(script.value(NAME).toString());
                    FilePath filePath = engine->toFileInProject(fileUrl);

                    const QVariantMap exception = body.value("exception").toMap();
                    QString errorMessage = exception.value("text").toString();

                    const QStringList messages = highlightExceptionCode(lineNumber, filePath, errorMessage);
                    for (const QString &msg : messages)
                        engine->showMessage(msg, ConsoleOutput);

                    if (engine->state() == InferiorRunOk) {
                        engine->notifyInferiorSpontaneousStop();
                        backtrace();
                    }

                    if (engine->state() == InferiorStopOk)
                        backtrace();

                } else if (eventType == "afterCompile") {
                    //Currently break point relocation is disabled.
                    //Uncomment the line below when it will be enabled.
//                    listBreakpoints();
                }

                //Sometimes we do not get event type!
                //This is most probably due to a wrong eval expression.
                //Redirect output to console.
                if (eventType.isEmpty()) {
                    debuggerConsole()->printItem(new ConsoleItem(
                                                     ConsoleItem::ErrorType,
                                                     resp.value(MESSAGE).toString()));
                }

            } //EVENT
        } //V8MESSAGE

    } else {
        //DO NOTHING
    }
}

void QmlEnginePrivate::handleBacktrace(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "backtrace",
    //      "body"        : { "fromFrame" : <number>
    //                        "toFrame" : <number>
    //                        "totalFrames" : <number>
    //                        "frames" : <array of frames - see frame request for details>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = response.value(BODY).toMap();
    const QVariantList frames = body.value("frames").toList();

    int fromFrameIndex = body.value("fromFrame").toInt();

    QTC_ASSERT(0 == fromFrameIndex, return);

    StackHandler *stackHandler = engine->stackHandler();
    StackFrames stackFrames;
    int i = 0;
    stackIndexLookup.clear();
    for (const QVariant &frame : frames) {
        StackFrame stackFrame = extractStackFrame(frame);
        if (stackFrame.level.isEmpty())
            continue;
        stackIndexLookup.insert(i, stackFrame.level.toInt());
        stackFrames << stackFrame;
        i++;
    }
    stackHandler->setFrames(stackFrames);
    stackHandler->setCurrentIndex(0);

    updateLocals();
}

StackFrame QmlEnginePrivate::extractStackFrame(const QVariant &bodyVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "frame",
    //      "body"        : { "index"          : <frame number>,
    //                        "receiver"       : <frame receiver>,
    //                        "func"           : <function invoked>,
    //                        "script"         : <script for the function>,
    //                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
    //                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
    //                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
    //                                               value: <value of the argument>
    //                                             },
    //                                             ... <the array contains all the arguments>
    //                                           ],
    //                        "locals"         : [ { name: <name of the local variable>,
    //                                               value: <value of the local variable>
    //                                             },
    //                                             ... <the array contains all the locals>
    //                                           ],
    //                        "position"       : <source position>,
    //                        "line"           : <source line>,
    //                        "column"         : <source column within the line>,
    //                        "sourceLineText" : <text for current source line>,
    //                        "scopes"         : [ <array of scopes, see scope request below for format> ],

    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = bodyVal.toMap();

    StackFrame stackFrame;
    stackFrame.level = body.value("index").toString();
    //Do not insert the frame corresponding to the internal function
    if (body.value("sourceLineText") == INTERNAL_FUNCTION) {
        stackFrame.level.clear();
        return stackFrame;
    }

    auto extractString = [this](const QVariant &item) {
        return (item.typeId() == QVariant::String ? item : extractData(item).value).toString();
    };

    stackFrame.function = extractString(body.value("func"));
    if (stackFrame.function.isEmpty())
        stackFrame.function = Tr::tr("Anonymous Function");
    stackFrame.file = engine->toFileInProject(extractString(body.value("script")));
    stackFrame.usable = stackFrame.file.isReadableFile();
    stackFrame.receiver = extractString(body.value("receiver"));
    stackFrame.line = body.value("line").toInt() + 1;

    return stackFrame;
}

void QmlEnginePrivate::handleFrame(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "frame",
    //      "body"        : { "index"          : <frame number>,
    //                        "receiver"       : <frame receiver>,
    //                        "func"           : <function invoked>,
    //                        "script"         : <script for the function>,
    //                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
    //                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
    //                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
    //                                               value: <value of the argument>
    //                                             },
    //                                             ... <the array contains all the arguments>
    //                                           ],
    //                        "locals"         : [ { name: <name of the local variable>,
    //                                               value: <value of the local variable>
    //                                             },
    //                                             ... <the array contains all the locals>
    //                                           ],
    //                        "position"       : <source position>,
    //                        "line"           : <source line>,
    //                        "column"         : <source column within the line>,
    //                        "sourceLineText" : <text for current source line>,
    //                        "scopes"         : [ <array of scopes, see scope request below for format> ],

    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    QVariantMap body = response.value(BODY).toMap();

    StackHandler *stackHandler = engine->stackHandler();
    WatchHandler * watchHandler = engine->watchHandler();
    watchHandler->notifyUpdateStarted();

    const int frameIndex = stackHandler->currentIndex();
    if (frameIndex < 0)
        return;
    const StackFrame frame = stackHandler->currentFrame();
    if (!frame.isUsable())
        return;

    // Always add a "this" variable
    {
        QString iname = "local.this";
        QString exp = "this";
        QmlV8ObjectData objectData = extractData(body.value("receiver"));

        auto item = new WatchItem;
        item->iname = iname;
        item->name = exp;
        item->id = objectData.handle;
        item->type = objectData.type;
        item->value = objectData.value.toString();
        setWatchItemHasChildren(item, objectData.hasChildren());
        // In case of global object, we do not get children
        // Set children nevertheless and query later.
        if (item->value == "global") {
            setWatchItemHasChildren(item, true);
            item->id = 0;
        }
        watchHandler->insertItem(item);
        evaluate(exp, -1, [this, iname, exp](const QVariantMap &response) {
            handleEvaluateExpression(response, iname, exp);

            // If there are no scopes, "this" may be the only thing to look up.
            if (currentFrameScopes.isEmpty())
                checkForFinishedUpdate();
        });
    }

    currentFrameScopes.clear();
    const QVariantList scopes = body.value("scopes").toList();
    for (const QVariant &scope : scopes) {
        //Do not query for global types (0)
        //Showing global properties increases clutter.
        if (scope.toMap().value("type").toInt() == 0)
            continue;
        int scopeIndex = scope.toMap().value("index").toInt();
        currentFrameScopes.append(scopeIndex);
        this->scope(scopeIndex);
    }

    if (skipFocusOnNextHandleFrame)
        engine->gotoLocation(stackHandler->currentFrame());

    // Send watchers list
    if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
        const QStringList watchers = WatchHandler::watchedExpressions();
        for (const QString &exp : watchers) {
            const QString iname = watchHandler->watcherName(exp);
            evaluate(exp, -1, [this, iname, exp](const QVariantMap &response) {
                handleEvaluateExpression(response, iname, exp);
            });
        }
    }
}

void QmlEnginePrivate::handleScope(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "scope",
    //      "body"        : { "index"      : <index of this scope in the scope chain. Index 0 is the top scope
    //                                        and the global scope will always have the highest index for a
    //                                        frame>,
    //                        "frameIndex" : <index of the frame>,
    //                        "type"       : <type of the scope:
    //                                         0: Global
    //                                         1: Local
    //                                         2: With
    //                                         3: Closure
    //                                         4: Catch >,
    //                        "object"     : <the scope object defining the content of the scope.
    //                                        For local and closure scopes this is transient objects,
    //                                        which has a negative handle value>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    QVariantMap bodyMap = response.value(BODY).toMap();

    //Check if the frameIndex is same as current Stack Index
    StackHandler *stackHandler = engine->stackHandler();
    if (bodyMap.value("frameIndex").toInt() != stackHandler->currentIndex())
        return;

    const QmlV8ObjectData objectData = extractData(bodyMap.value("object"));

    LookupItems itemsToLookup;
    for (const QVariant &property : objectData.properties) {
        QmlV8ObjectData localData = extractData(property);
        std::unique_ptr<WatchItem> item(new WatchItem);
        item->exp = localData.name;
        //Check for v8 specific local data
        if (item->exp.startsWith('.') || item->exp.isEmpty())
            continue;

        item->name = item->exp;
        item->iname = "local." + item->exp;
        item->id = localData.handle;
        item->type = localData.type;
        item->value = localData.value.toString();
        setWatchItemHasChildren(item.get(), localData.hasChildren());

        if (localData.value.isValid() || item->wantsChildren || localData.expectedProperties == 0) {
            WatchHandler *watchHandler = engine->watchHandler();
            if (watchHandler->isExpandedIName(item->iname))
                itemsToLookup.insert(int(item->id), {item->iname, item->name, item->exp});
            watchHandler->insertItem(item.release());
        } else {
            itemsToLookup.insert(int(item->id), {item->iname, item->name, item->exp});
        }
    }
    lookup(itemsToLookup);
    checkForFinishedUpdate();
}

void QmlEnginePrivate::checkForFinishedUpdate()
{
    if (currentlyLookingUp.isEmpty())
        engine->watchHandler()->notifyUpdateFinished();
}

ConsoleItem *QmlEnginePrivate::constructLogItemTree(const QmlV8ObjectData &objectData)
{
    QList<int> handles;
    return constructLogItemTree(objectData, handles);
}

void QmlEnginePrivate::constructChildLogItems(ConsoleItem *item, const QmlV8ObjectData &objectData,
                                              QList<int> &seenHandles)
{
    // We cannot sort the children after attaching them to the parent as that would cause layout
    // changes, invalidating cached indices. So we presort them before inserting.
    QVarLengthArray<ConsoleItem *> children(objectData.properties.size());
    auto it = children.begin();
    for (const QVariant &property : objectData.properties)
        *(it++) = constructLogItemTree(extractData(property), seenHandles);

    if (settings().sortStructMembers())
        std::sort(children.begin(), children.end(), compareConsoleItems);

    for (ConsoleItem *child : std::as_const(children))
        item->appendChild(child);
}

ConsoleItem *QmlEnginePrivate::constructLogItemTree(const QmlV8ObjectData &objectData,
                                                    QList<int> &seenHandles)
{
    QString text;
    if (objectData.value.isValid()) {
        text = objectData.value.toString();
    } else if (!objectData.type.isEmpty()) {
        text = objectData.type;
    } else {
        int handle = objectData.handle;
        ConsoleItem *item = new ConsoleItem(ConsoleItem::DefaultType,
                                            objectData.name,
                                            [this, handle](ConsoleItem *item)
        {
            DebuggerCommand cmd(LOOKUP);
            cmd.arg(HANDLES, QList<int>() << handle);
            runCommand(cmd, [this, item, handle](const QVariantMap &response) {
                const QVariantMap body = response.value(BODY).toMap();
                const QStringList handlesList = body.keys();
                for (const QString &handleString : handlesList) {
                    if (handle != handleString.toInt())
                        continue;

                    QmlV8ObjectData objectData = extractData(body.value(handleString));

                    // keep original name, if possible
                    QString name = item->expression();
                    if (name.isEmpty())
                        name = objectData.name;

                    QString value = objectData.value.isValid() ?
                                objectData.value.toString() : objectData.type;

                    // We can do setData() and cause dataChanged() here, but only because this
                    // callback is executed after fetchMore() has returned.
                    item->model()->setData(item->index(),
                                           QString("%1: %2").arg(name, value),
                                           ConsoleItem::ExpressionRole);

                    QList<int> newHandles;
                    constructChildLogItems(item, objectData, newHandles);

                    break;
                }
            });
        });
        return item;
    }

    if (!objectData.name.isEmpty())
        text = QString("%1: %2").arg(objectData.name, text);

    if (objectData.properties.isEmpty())
        return new ConsoleItem(ConsoleItem::DefaultType, text);

    if (seenHandles.contains(objectData.handle)) {
        ConsoleItem *item = new ConsoleItem(ConsoleItem::DefaultType, text,
                                            [this, objectData](ConsoleItem *item)
        {
            QList<int> newHandles;
            constructChildLogItems(item, objectData, newHandles);
        });
        return item;
    }

    seenHandles.append(objectData.handle);
    ConsoleItem *item = new ConsoleItem(ConsoleItem::DefaultType, text);
    constructChildLogItems(item, objectData, seenHandles);
    seenHandles.removeLast();

    return item;
}

void QmlEnginePrivate::insertSubItems(WatchItem *parent, const QVariantList &properties)
{
    QTC_ASSERT(parent, return);
    LookupItems itemsToLookup;

    for (const QVariant &property : properties) {
        QmlV8ObjectData propertyData = extractData(property);
        std::unique_ptr<WatchItem> item(new WatchItem);
        item->name = propertyData.name;

        // Check for v8 specific local data
        if (item->name.startsWith('.') || item->name.isEmpty())
            continue;
        if (parent->type == "object") {
            if (parent->value == "Array")
                item->exp = parent->exp + '[' + item->name + ']';
            else if (parent->value == "Object")
                item->exp = parent->exp + '.' + item->name;
        } else {
            item->exp = item->name;
        }

        item->iname = parent->iname + '.' + item->name;
        item->id = propertyData.handle;
        item->type = propertyData.type;
        item->value = propertyData.value.toString();
        if (item->type.isEmpty() || engine->watchHandler()->isExpandedIName(item->iname))
            itemsToLookup.insert(propertyData.handle, {item->iname, item->name, item->exp});
        setWatchItemHasChildren(item.get(), propertyData.hasChildren());
        parent->appendChild(item.release());
    }

    if (settings().sortStructMembers()) {
        parent->sortChildren([](const WatchItem *item1, const WatchItem *item2) {
            return item1->name < item2->name;
        });
    }

    lookup(itemsToLookup);
}

void QmlEnginePrivate::handleExecuteDebuggerCommand(const QVariantMap &response)
{
    auto it = response.constFind(SUCCESS);
    if (it != response.constEnd() && it.value().toBool()) {
        debuggerConsole()->printItem(constructLogItemTree(extractData(response.value(BODY))));

        // Update the locals
        for (int index : std::as_const(currentFrameScopes))
            scope(index);
    } else {
        debuggerConsole()->printItem(new ConsoleItem(ConsoleItem::ErrorType,
                                                     response.value(MESSAGE).toString()));
    }
}

void QmlEnginePrivate::handleLookup(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "lookup",
    //      "body"        : <array of serialized objects indexed using their handle>
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    const QVariantMap body = response.value(BODY).toMap();

    const QStringList handlesList = body.keys();
    for (const QString &handleString : handlesList) {
        const int handle = handleString.toInt();
        const QmlV8ObjectData bodyObjectData = extractData(body.value(handleString));
        const LookupData res = currentlyLookingUp.value(handle);
        currentlyLookingUp.remove(handle);

        auto item = new WatchItem;
        item->exp = res.exp;
        item->iname = res.iname;
        item->name = res.name;
        item->id = handle;

        item->type = bodyObjectData.type;
        item->value = bodyObjectData.value.toString();

        setWatchItemHasChildren(item, bodyObjectData.hasChildren());
        insertSubItems(item, bodyObjectData.properties);

        engine->watchHandler()->insertItem(item);
    }
    checkForFinishedUpdate();
}

void QmlEnginePrivate::stateChanged(State state)
{
    engine->logServiceStateChange(name(), serviceVersion(), state);

    if (state == QmlDebugClient::Enabled) {
        BreakpointManager::claimBreakpointsForEngine(engine);

        // Since the breakpoint claiming is deferred, we need to also defer the connecting
        QTimer::singleShot(0, this, [this] {
            /// Start session.
            flushSendBuffer();
            QJsonObject parameters;
            parameters.insert("redundantRefs", false);
            parameters.insert("namesAsObjects", false);
            runDirectCommand(CONNECT, QJsonDocument(parameters).toJson());
            runCommand({VERSION}, CB(handleVersion));
        });
    }
}

void QmlEnginePrivate::handleVersion(const QVariantMap &response)
{
    const QVariantMap body = response.value(BODY).toMap();
    unpausedEvaluate = body.value("UnpausedEvaluate", false).toBool();
    contextEvaluate = body.value("ContextEvaluate", false).toBool();
    supportChangeBreakpoint = body.value("ChangeBreakpoint", false).toBool();
}

void QmlEnginePrivate::flushSendBuffer()
{
    QTC_ASSERT(state() == Enabled, return);
    for (const QByteArray &msg : std::as_const(sendBuffer))
        sendMessage(msg);
    sendBuffer.clear();
}

FilePath QmlEngine::toFileInProject(const QUrl &fileUrl)
{
    // make sure file finder is properly initialized
    const DebuggerRunParameters &rp = runParameters();
    d->fileFinder.setProjectDirectory(rp.projectSourceDirectory);
    d->fileFinder.setProjectFiles(rp.projectSourceFiles);
    d->fileFinder.setAdditionalSearchDirectories(rp.additionalSearchDirectories);
    d->fileFinder.setSysroot(rp.sysRoot);

    return d->fileFinder.findFile(fileUrl).constFirst();
}

DebuggerEngine *createQmlEngine()
{
    return new QmlEngine;
}

} // Debugger::Internal

#endif // DEBUGGERCLIENT_H