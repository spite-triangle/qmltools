#ifndef QMLLANGUAGEMODEL_H
#define QMLLANGUAGEMODEL_H

#include <mutex>
#include <atomic>

#include <QTimer>
#include <QObject>
#include <QFuture>
#include <QByteArray>

#include "qmljs/qmljsdocument.h"
#include "qmlModel/qmljsmodelmanager.h"

#include "common/utils.h"
#include "common/lspDefine.h"
#include "common/singleton.hpp"
#include "qmlModel/qmljssemanticinfo.h"
#include "qmlModel/qmljsbundleprovider.h"

class QmlLanguageModel : public QObject, public Singleton<QmlLanguageModel>{
    Q_OBJECT

public:
    using ProjectInfo = QmlJS::ModelManagerInterface::ProjectInfo;
public:

    QmlLanguageModel(QObject* parent = nullptr);

    /* 根据 Project 配置，创建 ProjectInfo */
    ProjectInfo creatProjectInfo();

    /* 更新模型 */
    bool restProjectInfo();
    void resetModle();
    void waitModleUpdate();
    void waitModelManagerUpdate();
    
    bool updateSourceFile();
    bool updateSourceFile(const QString & strFile);
    bool appendSourceFile(const QStringList & lstFile);

    /* 文档 */
    void openFile(const QString & path, int revision);
    void closeFile(const QString & path);
    QString fileContent(const QString & path);
    void updateFile(const QString & path, const QString & content,int revision);
    Json formatFile(const QString & path, uint32_t uTableSize);
    Json queryColor(const QString & path);

    static size_t convertPosition(const QString & content,const POSITION_S & pos);
    static std::string filePathToUrl(const QString & strPath);

    bool isValid();
    void setValid(bool bValid){m_bValid.store(bValid);};

    MUT_FUNC_SET_GET(m_muteSemantic, QmlJS::SemanticInfo,  m_currSemantic, CurrentSemantic);
    MUT_FUNC_SET_GET(m_muteFocus, QString, m_currFocusFile, CurrFocusFile);

signals:
    void sigDiagnosticMessageUpdated(const JsonPtr & json);

public slots:

    /* 有 qml 文件解析完成便会调用 */
    void onDocumentUpdated(QmlJS::Document::Ptr doc);

    /* ProjectInfo 更新完成便会调用 */
    void onProjectInfoUpdated(const ProjectInfo &pinfo);

private:
    static QList<Utils::FilePath> querySources(const QString & strFolder);
    static QList<Utils::FilePath> querySources(const QStringList & lstFolder);

    JsonPtr diagnosticMsgToJson(const QString & path, const QList<QmlJS::DiagnosticMessage> & lstMsg);
    JsonPtr diagnosticMsgToJson(const QString & path, const QStringList & lstMsg);
    JsonPtr diagnosticMsgToJson(const QString & path, const QList<QmlJS::StaticAnalysis::Message> & lstAnalysisMsg, const QList<QmlJS::DiagnosticMessage> & lstDiagnostMsg);

    QByteArray loadFile(const QString &strPath, bool &bRes);

private:
    std::atomic_bool m_bValid; // 当前模型是否有效

    std::mutex m_muteFocus;    
    QString m_currFocusFile; // 当前正专注的文件

    std::mutex m_muteModelUpdate;
    QFuture<void> m_futureModelUpdate; // 模型创建

    BasicBundleProvider m_basicBundleProvider;
    std::mutex m_muteSemantic;
    QmlJS::SemanticInfo m_currSemantic; // 当前正编辑文本的 SemanticInfo
};

#endif /* QMLLANGUAGEMODEL_H */
