#ifndef BAILIPUBLISHER_H
#define BAILIPUBLISHER_H

#include <QtCore>
#include <QThread>

namespace BailiSoft {

class SyncJob {
public:
    SyncJob(){}
    SyncJob(const QString &shop, const QString &relSheetTable, const int relSheetId)
        : mShop(shop), mRelSheetTable(relSheetTable), mRelSheetId(relSheetId) {}
    QString mShop;
    QString mRelSheetTable;
    int     mRelSheetId;
};

class BsPublisher : public QThread
{
    Q_OBJECT
public:
    BsPublisher(QObject *parent) : QThread(parent) {}

    void bookLogin(const QString &dbfile);
    void bookLogout();
    void stopWait();
    bool logined() { return !mDatabaseFile.isEmpty(); }

public slots:
    void addJob(const QString &shop, const QString &relSheetTable, const int relSheetId);

private:
    void    run() override;

    QString                 mDatabaseFile;

    QMutex                  mBookMutex;
    QWaitCondition          mBookCondition;
    bool                    mBookWaiting = true;

    QQueue<SyncJob>         mJobs;
    QMutex                  mWorkMutex;
    QWaitCondition          mWorkCondition;

    QMap<QString, qint64>   mSyncStockLog;
};

}

#endif // BAILIPUBLISHER_H
