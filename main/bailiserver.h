#ifndef BSSERVER_H
#define BSSERVER_H

#include <QtNetwork>
#include <QThread>
#include "bailiterminator.h"

namespace BailiSoft {

class BsServer : public QObject
{
    Q_OBJECT
public:
    BsServer(QObject *parent = Q_NULLPTR);
    ~BsServer();

    void startServer(const QString &backerName, const QString &backerVcode, const int frontCount);
    void stopServer();
    void stopAutoKeeper();
    bool isRunning();

signals:
    void serverStarted(const qint64 licDate);
    void serverStopped();
    void startFailed(const QString &errMsg);
    void shopStockChanged(const QString &shop);

private slots:
    void lookupAddressFinished();
    void tcpError(QAbstractSocket::SocketError socketError);
    void tcpConnected();
    void tcpDisconnected();
    void tcpReadReady();
    void reconnectHost();
    void sendBeating();
    void queueSocketWrite(const QByteArray &toServerData);
    void workerFinished();

private:
    BsTerminator*   hireWorker();

    QUrl                        mBailiSiteUrl;
    QString                     mTransferHost;
    quint16                     mTransferPort;

    QTcpSocket*                 mpSocket;
    QMutex                      mSendMutex;
    int                         mReadLen;   //收据长度头
    QByteArray                  mReading;   //读缓存buffer

    QList<BsTerminator*>            mThreads;
    int                         mThreadCount;
    int                         mWorkings;
    QTimer                      mKeeper;
    QTimer                      mBeater;
};

}

#endif // BSSERVER_H
