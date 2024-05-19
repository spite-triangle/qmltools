// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "watchdata.h"
#include "stackframe.h"

#include <QVector>

namespace Debugger {
namespace Internal {

class DebuggerCommand;
class DebuggerEngine;
class WatchModel;

using DisplayFormats = QVector<DisplayFormat>;


class Location
{
public:
    Location() = default;
    Location(quint64 address) { m_address = address; }
    Location(const Utils::FilePath &file) { m_fileName = file; }
    Location(const Utils::FilePath &file, int line, bool marker = true)
        { m_textPosition = {line, -1}; m_fileName = file; m_needsMarker = marker; }
    Location(const Utils::FilePath &file, const Utils::Text::Position &pos, bool marker = true)
        { m_textPosition = pos; m_fileName = file; m_needsMarker = marker; }
    Location(const StackFrame &frame, bool marker = true);
    Utils::FilePath fileName() const { return m_fileName; }
    QString functionName() const { return m_functionName; }
    QString from() const { return m_from; }
    Utils::Text::Position textPosition() const { return m_textPosition; }
    void setNeedsRaise(bool on) { m_needsRaise = on; }
    void setNeedsMarker(bool on) { m_needsMarker = on; }
    void setFileName(const Utils::FilePath &fileName) { m_fileName = fileName; }
    void setUseAssembler(bool on) { m_hasDebugInfo = !on; }
    bool needsRaise() const { return m_needsRaise; }
    bool needsMarker() const { return m_needsMarker; }
    bool hasDebugInfo() const { return m_hasDebugInfo; }
    bool canBeDisassembled() const
        { return m_address != quint64(-1) || !m_functionName.isEmpty(); }
    quint64 address() const { return m_address; }

private:
    bool m_needsMarker = false;
    bool m_needsRaise = true;
    bool m_hasDebugInfo = true;
    Utils::Text::Position m_textPosition;
    Utils::FilePath m_fileName;
    QString m_functionName;
    QString m_from;
    quint64 m_address = 0;
};


class WatchModelBase : public Utils::TreeModel<WatchItem, WatchItem>
{
    Q_OBJECT

public:
    WatchModelBase() = default;

    enum { NameColumn, TimeColumn, ValueColumn, TypeColumn };

signals:
    void currentIndexRequested(const QModelIndex &idx);
    void itemIsExpanded(const QModelIndex &idx);
    void inameIsExpanded(const QString &iname);
    void updateStarted();
    void updateFinished();
};

class WatchHandler
{
    Q_DISABLE_COPY_MOVE(WatchHandler)

public:
    explicit WatchHandler(DebuggerEngine *engine);
    ~WatchHandler();

    WatchModelBase *model() const;

    void cleanup();
    void grabWidget(QWidget *viewParent);
    void watchExpression(const QString &exp, const QString &name = QString(),
                         bool temporary = false);
    void updateWatchExpression(WatchItem *item, const QString &newExp);
    void watchVariable(const QString &exp);

    const WatchItem *watchItem(const QModelIndex &) const;
    void fetchMore(const QString &iname) const;
    WatchItem *findItem(const QString &iname) const;
    const WatchItem *findCppLocalVariable(const QString &name) const;

    void loadSessionDataForEngine();

    bool isExpandedIName(const QString &iname) const;
    QSet<QString> expandedINames() const;
    int maxArrayCount(const QString &iname) const;

    static QStringList watchedExpressions();
    static QMap<QString, int> watcherNames();

    void appendFormatRequests(DebuggerCommand *cmd) const;
    void appendWatchersAndTooltipRequests(DebuggerCommand *cmd) const;

    QString typeFormatRequests() const;
    QString individualFormatRequests() const;

    int format(const QString &iname) const;
    static QString nameForFormat(int format);

    void addDumpers(const GdbMi &dumpers);
    void addTypeFormats(const QString &type, const DisplayFormats &formats);

    QString watcherName(const QString &exp);

    void scheduleResetLocation();

    void setCurrentItem(const QString &iname);
    void updateLocalsWindow();

    bool insertItem(WatchItem *item); // Takes ownership, returns whether item was added, not overwritten.
    void insertItems(const GdbMi &data);

    void removeItemByIName(const QString &iname);
    void removeAllData(bool includeInspectData = false);
    void resetValueCache();
    void resetWatchers();

    // void notifyUpdateStarted(const UpdateParameters &updateParameters = UpdateParameters());
    // void notifyUpdateFinished();

    void reexpandItems();
    void recordTypeInfo(const GdbMi &typeInfo);

    void setLocation(const Location &loc);

private:
    DebuggerEngine * const m_engine; // Not owned
    WatchModel *m_model; // Owned.
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::DisplayFormat)
