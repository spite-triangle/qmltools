#ifndef QMLJSFINDERREFERENCE_H
#define QMLJSFINDERREFERENCE_H

#include <QFuture>


#include "common/lspDefine.h"

#include "utils/filepath.h"

namespace QmlJS
{

/* 
    qmljseditor.h  findLinkAt
    qmljsfindreferences.h
 */
class FindReferences{
public:
    class Usage
    {
    public:
        Usage(const Utils::FilePath &path, const QString &lineText, int line, int col, int len)
            : path(path)
            , lineText(lineText)
            , line(line)
            , col(col)
            , len(len)
        {}

    public:
        Utils::FilePath path;
        QString lineText;
        int line = 0;
        int col = 0;
        int len = 0;
    };


public:
    QFuture<Usage> findUsages(const Utils::FilePath &fileName, const POSITION_S & pos);
    QFuture<Usage> renameUsages(const Utils::FilePath &fileName, const QString & replacement, const POSITION_S & pos);

    Json findDeclaration(const Utils::FilePath &fileName, const POSITION_S & pos);

    Json convertRename(const QFuture<Usage> & res);
    Json convertReference(const QFuture<Usage> & res);


    static QList<Usage> findUsageOfType(const Utils::FilePath &fileName, const QString &typeName);
};

} // namespace QmlJS


#endif // QMLJSFINDERREFERENCE_H
