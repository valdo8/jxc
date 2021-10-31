#ifndef BAILITERMINATOR_H
#define BAILITERMINATOR_H

#include <QThread>
#include "bailishare.h"

namespace BailiSoft {

class BsTerminator : public QThread
{
    Q_OBJECT
public:
    BsTerminator(QObject *parent, const QString &databaseConnectionName);
    void taskAdd(const QByteArray &fromServerData);
    void taskStop();
    bool isIdle();
    void run();

signals:
    void responseReady(const QByteArray &toServerData);
    void transferReady(const QByteArray &toServerData);
    void shopStockChanged(const QString &shop, const QString &relSheet, const int relId);

private:
    QStringList getMessageReceiverIds(const QString &chatTo, BsFronter *sender);
    QStringList calcNamesToIds(const QStringList &names);

    QByteArray dataDecrypt(const QByteArray &data);
    QByteArray dataEncrypt(const QByteArray &data);
    QByteArray dataDozip(const QByteArray &data, const bool removeLenHead = false);
    QByteArray dataUnzip(const QByteArray &data);
    QString buildSpecHSum(const QString &sql);
    QString buildSpecVSum(const QString &sql, const QString &limSizer = QString());
    QString buildSqlData(const QString &sql,
                         const char replaceTabChar = 0,
                         const char replaceLineChar = 0);

    void checkRecordTransReport(const QByteArray &rptData);
    void serverLog(const QString &reqMan, const int reqType, const QString &reqInfo);

    //以下函数注意：如果返回空字符串，则前端永久等待失去响应。
    QString reqLogin(const QString &packstr, BsFronter *user);

    QString reqQrySheet(const QString &packstr, const BsFronter *user);     //only desk client
    QString reqQryPick(const QString &packstr, const BsFronter *user);      //only desk client
    QString reqBizOpen(const QString &packstr, const BsFronter *user);      //only desk client
    QString reqBizEdit(const QString &packstr, const BsFronter *user);      //only desk client
    QString reqBizDelete(const QString &packstr, const BsFronter *user, const bool sqlsForEdit = false);    //only desk client 第三参数为复用设计

    QString reqBizInsert(const QString &packstr, const BsFronter *user, const qint64 updSheetId = 0);  //第三参数为复用设计
    QString reqFeeInsert(const QString &packstr, const BsFronter *user);
    QString reqRegInsert(const QString &packstr, const BsFronter *user);
    QString reqRegCargo(const QString &packstr, const BsFronter *user);
    QString reqQrySumm(const QString &packstr, const BsFronter *user);
    QString reqQryCash(const QString &packstr, const BsFronter *user);
    QString reqQryRest(const QString &packstr, const BsFronter *user);
    QString reqQryStock(const QString &packstr, const BsFronter *user);
    QString reqQryView(const QString &packstr, const BsFronter *user);
    QString reqQryObject(const QString &packstr, const BsFronter *user);
    QString reqQryNewPush(const QString &packstr, const BsFronter *user);
    QString reqQryImage(const QString &packstr, const BsFronter *user);
    QString reqQryPrintOwe(const QString &packstr, const BsFronter *user);
    QString reqMessage(const QString &packstr, const BsFronter *user, const QString &toName, qint64 *msgIdPtr);

    QString reqGrpCreate(const QString &packstr);
    QString reqGrpRename(const QString &packstr);
    QString reqGrpDismiss(const QString &packstr);
    QString reqGrpInvite(const QString &packstr);
    QString reqGrpKickoff(const QString &packstr);

    QString mDatabaseConnectionName;

    QQueue<QByteArray>              mTransactions;
    QMutex                          mTransactionMutex;
    QWaitCondition                  mTransactionAdded;
};

}

#endif // BAILITERMINATOR_H
