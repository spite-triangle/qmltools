#ifndef QMLMODEL_H
#define QMLMODEL_H

#include <mutex>
#include <QObject>

#include "qmljs/qmljsdocument.h"
#include "qmljs/qmljsmodelmanagerinterface.h"

#include "common/utils.h"
#include "common/singleton.hpp"
#include "qmlModel/qmljssemanticinfo.h"
#include "qmlModel/qmljsbundleprovider.h"

class QmlLanguageModel : public QObject, public Singleton<QmlLanguageModel>{
    Q_OBJECT

public:
    using ProjectInfo = QmlJS::ModelManagerInterface::ProjectInfo;
public:

    QmlLanguageModel(QObject* parent = nullptr);

    /* 更新配置信息 */
    bool updateProjectInfo();

    /* 根据 Project 配置，创建 ProjectInfo */
    ProjectInfo creatProjectInfo();
    
    bool updateSourceFile(const QString & strFile);

    MUT_FUNC_SET_GET(m_muteSemantic, QmlJS::SemanticInfo,  m_currentSemantic, CurrentSemantic);

signals:
    void sigDiagnosticMessageUpdated(const QList<QmlJS::DiagnosticMessage> & msgs);

public slots:

    /* 有 qml 文件解析完成便会调用 */
    void onDocumentUpdated(QmlJS::Document::Ptr doc);

    /* ProjectInfo 更新完成便会调用 */
    void onProjectInfoUpdated(const ProjectInfo &pinfo);

private:

    static QList<Utils::FilePath> querySources(const QString & strFolder);
    static QList<Utils::FilePath> querySources(const QStringList & lstFolder);

private:
    BasicBundleProvider m_basicBundleProvider;

    std::mutex m_muteSemantic;
    QmlJS::SemanticInfo m_currentSemantic; // 当前正编辑文本的 SemanticInfo


};

#endif // QMLMODEL_H
