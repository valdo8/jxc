#ifndef BAILIPUBLISHER_H
#define BAILIPUBLISHER_H

#include <QtCore>
#include <QThread>

namespace BailiSoft {

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
    void addJob(const QString &shop);

private:
    void    run() override;

    QString                 mDatabaseFile;

    QMutex                  mBookMutex;
    QWaitCondition          mBookCondition;
    bool                    mBookWaiting = true;

    QQueue<QString>         mJobs;
    QMutex                  mWorkMutex;
    QWaitCondition          mWorkCondition;

    QString                 mLastShop;
    qint64                  mLastSeconds;
};

}

#endif // BAILIPUBLISHER_H
