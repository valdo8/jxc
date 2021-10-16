#include "bailiserver.h"
#include "bailicode.h"
#include "bailifunc.h"
#include "bailishare.h"
#include "bailidata.h"

namespace BailiSoft {

BsServer::BsServer(QObject *parent) : QObject(parent)
{
    mThreadCount = 3;
    mWorkings = 0;

    mKeeper.setInterval(75000);   //服务器60秒超时后才真正下线，所以，得设置超过60秒
    mKeeper.setSingleShot(false);
    mKeeper.stop();
    connect(&mKeeper, SIGNAL(timeout()), this, SLOT(reconnectHost()));

    mBeater.setInterval(5000);
    mBeater.setSingleShot(false);
    mBeater.stop();
    connect(&mBeater, SIGNAL(timeout()), this, SLOT(sendBeating()));

    mpSocket = new QTcpSocket(this);
    mpSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(mpSocket, SIGNAL(connected()), this, SLOT(tcpConnected()));
    connect(mpSocket, SIGNAL(readyRead()), this, SLOT(tcpReadReady()));
    connect(mpSocket, SIGNAL(disconnected()), this, SLOT(tcpDisconnected()));
    connect(mpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(tcpError(QAbstractSocket::SocketError)));
}

BsServer::~BsServer()
{
    stopServer();
}

void BsServer::startServer(const QString &backerName, const QString &backerVcode, const int frontCount)
{
    //计算使用线程数量
    int threads = frontCount / 100;
    if ( threads < 3 )
        mThreadCount = 3;
    else if ( threads > 16 )
        mThreadCount = 16;
    else
        mThreadCount = threads;

    //加载后台信息（基本参数与保密码）
    BsBackerInfo::loadUpdate(backerName, backerVcode);

    //启动寻址
    mBailiSiteUrl = QStringLiteral("https://www.bailisoft.com/cmd/mids?backer=%1").arg(backerName);
    QNetworkRequest request(mBailiSiteUrl);
    QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConf.setProtocol(QSsl::TlsV1SslV3);
    request.setSslConfiguration(sslConf);

    netManager.clearAccessCache();
    QNetworkReply *netReply = netManager.get(request);
    connect(netReply, SIGNAL(finished()), this, SLOT(lookupAddressFinished()));
}

void BsServer::stopServer()
{
    mBeater.stop();
    for ( int i = 0, iLen = mThreads.length(); i < iLen; ++i ) {
        BsTerminator *worker = mThreads.at(i);
        worker->taskStop();
    }
    mThreads.clear();
}

void BsServer::stopAutoKeeper()
{
    mKeeper.stop();
}

bool BsServer::isRunning()
{
    return mpSocket->state() == QAbstractSocket::ConnectedState;
}

void BsServer::lookupAddressFinished()
{
    QNetworkReply *netReply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(netReply);
    netReply->deleteLater();

    //获取失败
    if ( netReply->error() ) {
        mTransferHost.clear();
        mTransferPort = 0;
        emit startFailed(QStringLiteral("网络错误：%1").arg(netReply->errorString()));
    }
    else {
        //检查地址重定向
        QVariant redirectionTarget = netReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        //无重定向
        if ( redirectionTarget.isNull() ) {
            QStringList flds = QString::fromLatin1(netReply->readAll()).split(QChar(':'));
            if ( flds.length() >= 4 ) {

                //解析公服地址端口
                mTransferHost = flds.at(0);
                mTransferPort = QString(flds.at(1)).toUShort();
                qDebug() << "mTransferHost: " << mTransferHost << ", mTransferPort: " << mTransferPort;

                //加载前端字典（本类不具体调用，但需要在本类中加载，主次线程中都还要随时更新）
                BsFronterMap::loadUpdate();

                //加载聊天群组
                BsMeetingMap::loadUpdate();

                //连接公服，并设置网络触发事件
                mpSocket->connectToHost(mTransferHost, mTransferPort);
            }
            else {
                mTransferHost.clear();
                mTransferPort = 0;
                emit startFailed(QStringLiteral("外网服务错误！"));
            }
        }
        //有重定向，需重新请求
        else {
            qDebug() << "redirecting...";
            mBailiSiteUrl = mBailiSiteUrl.resolved(redirectionTarget.toUrl());
            QNetworkRequest request(mBailiSiteUrl);
            QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
            sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConf.setProtocol(QSsl::TlsV1SslV3);
            request.setSslConfiguration(sslConf);
            netManager.clearAccessCache();
            netReply = netManager.get(request);
            connect(netReply, SIGNAL(finished()), this, SLOT(lookupAddressFinished()));
        }
    }
}

void BsServer::tcpError(QAbstractSocket::SocketError socketError)
{
    if ( QAbstractSocket::TemporaryError == socketError ) {
        qDebug() << "tcpError: TemporaryError.";
        return;                 //是否应当忽略？
    }

    //尽早停
    mBeater.stop();
    qDebug() << "tcpError: " << socketError;

    if ( mThreads.count() == 0 ) {
        //公服端主动关闭（必为非法原因）
        if ( QAbstractSocket::RemoteHostClosedError == socketError )
            emit startFailed(QStringLiteral("启动失败，请确保软件狗与前端账号发放数量都没超出服务授权！"));
        //服务未开、地址不对、或网络故障原因
        else
            emit startFailed(QStringLiteral("外网服务故障！"));
    }
}

void BsServer::tcpConnected()
{
    mKeeper.stop();

    //按协议构造登记包【selfId(16)backerId(16)epoch(8)randomBytes(64)vhash(64)DataLen(4)frontListData(x)】
    QByteArray frontsList = BsFronterMap::getAllIdsBytes();  //用户增减与密码更新
    qint64 epoch = QDateTime::currentMSecsSinceEpoch();
    qint32 dataLen = frontsList.length();
    QByteArray backerId = BsBackerInfo::getBackerId().toLatin1();
    QByteArray netPasscode = BsBackerInfo::getNetCode().toLatin1();
    QByteArray epochBytes = QByteArray(8, '\0');
    QByteArray dataLenBytes = QByteArray(4, '\0');
    qToBigEndian(epoch, epochBytes.data());
    qToBigEndian(dataLen, dataLenBytes.data());
    QByteArray randomBytes = generateRandomAsciiBytes(64);
    QByteArray verify = backerId + QString::number(epoch).toLatin1() + netPasscode + randomBytes;
    QByteArray vhash = QCryptographicHash::hash(verify, QCryptographicHash::Sha256).toHex();
    QByteArray reqData = backerId + backerId + epochBytes + randomBytes + vhash + dataLenBytes + frontsList;

    //清空缓存
    mReadLen = 0;
    mReading.clear();

    //保证写完
    const qint64 total = reqData.length();
    qint64 sends = 0;
    while ( sends < total ) {
        sends += mpSocket->write(reqData.mid(sends));
    }
}

void BsServer::tcpDisconnected()
{
    qDebug() << "socket disconnected";

    //通知界面
    emit serverStopped();

    //如仍有线程，说明为故障掉线
    if ( mThreads.count() > 0 ) {
        mKeeper.start();
    }
}

void BsServer::tcpReadReady()
{
    //读取数据
    mReading += mpSocket->readAll();

    //理论上有可能包括多个任务数据，所以要用while
    while ( mReading.length() >= mReadLen + 4 ) {

        //取长度值
        mReadLen = qFromBigEndian<qint32>(mReading.left(4).constData());

        //收齐触发
        if ( mReading.length() >= mReadLen + 4 ) {

            //取出数据
            QByteArray readyData = mReading.mid(4, mReadLen);
            mReading = mReading.mid(mReadLen + 4);  //待下一循环处理后续数据
            mReadLen = 0;

            //公服不返回错误，有错误服务器会主动Close同时以日志方式记录错误；这里则会有SocketError触发结束。

            //非请求性信息处理
            if ( readyData.startsWith("OK") && readyData.length() == 18 ) {

                //检查公服反馈授权数（grantOffiShops and grantCustomers）
                int respOffiShops = qFromBigEndian<qint32>(readyData.mid(10, 4));
                int respCustomers = qFromBigEndian<qint32>(readyData.mid(14, 4));
                if ( grantOffiShops > respOffiShops ||
                     grantOffiShops + grantCustomers > respOffiShops + respCustomers ) {
                    mpSocket->disconnectFromHost();
                    emit startFailed(QStringLiteral("启动失败，请确保前端账号发放类型和数量都没超出购买授权！"));
                    return;
                }

                //准备服务线程池
                if ( mThreads.isEmpty() ) {
                    mWorkings = 0;
                    for ( int i = 0; i < mThreadCount; ++i ) {
                        BsTerminator* worker = new BsTerminator(this, QStringLiteral("jydbconn%1").arg(i));
                        connect(worker, SIGNAL(responseReady(QByteArray)), this, SLOT(queueSocketWrite(QByteArray)));
                        connect(worker, SIGNAL(transferReady(QByteArray)), this, SLOT(queueSocketWrite(QByteArray)));
                        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
                        connect(worker, SIGNAL(finished()), this, SLOT(workerFinished()));
                        connect(worker, &BsTerminator::shopStockChanged, this, &BsServer::shopStockChanged);
                        worker->start();
                        mThreads << worker;
                        mWorkings++;
                    }
                }

                //心跳启动
                mBeater.start();

                //通知窗口
                emit serverStarted(qFromBigEndian<qint64>(readyData.mid(2, 8)));

                //继续等数据
                continue;
            }

            //分配线程处理
            if ( readyData.length() > 3 ) {  //REQ开头或RPT开头
                hireWorker()->taskAdd(readyData);
            }
        }
    }
}

void BsServer::reconnectHost()
{
    if ( mpSocket->state() == QAbstractSocket::UnconnectedState ) {
        qDebug() << "reconnecting...";
        mpSocket->connectToHost(mTransferHost, mTransferPort);
    }
    qDebug() << "reconnectHost state:" << mpSocket->state();
}

void BsServer::sendBeating()
{
    //各类系统防火墙原因，心跳包机制不可少。否则稍过一会，就收不到QTcpSocket::readyRead信号。
    QByteArray beatData = QByteArray(4, '\0');
    queueSocketWrite(beatData);
}

void BsServer::queueSocketWrite(const QByteArray &toServerData)
{
      //非法请求约定必须用空字节表示。既然非法，不要回复。
    if ( toServerData.length() < 4 ) {
        return;
    }

    //检查避免公服宕机
    qint32 dataLen = qFromBigEndian<qint32>(toServerData.left(4).constData());
    if ( dataLen != toServerData.length() - 4 ) {
        return;
    }

    //因为写由多线程触发，必须加锁
    QMutexLocker locker(&mSendMutex);

    //保证写完
    int pos = 0;
    int len = toServerData.length();
    while ( pos < len ) {
        qint64 written = mpSocket->write(toServerData.mid(pos));
        if ( written >= 0 ) {
            pos += written;
        } else {
            QSqlDatabase db = QSqlDatabase::database();
            db.exec(QStringLiteral("insert into serverfail(timeid, faildata) values(%1, '%2');")
                    .arg(QDateTime::currentMSecsSinceEpoch()).arg(QString(toServerData.toHex())));
            return;
        }
    }
}

void BsServer::workerFinished()
{
    mWorkings--;
    if ( mWorkings == 0 ) {
        emit serverStopped();
        if ( mpSocket->state() == QTcpSocket::ConnectedState ) {
            mpSocket->disconnectFromHost();
        }
    }
}

BsTerminator *BsServer::hireWorker()
{
    //遍历取闲
    for ( int i = 0, iLen = mThreads.length(); i < iLen; ++i ) {
        if ( mThreads.at(i)->isIdle() ) {
            return mThreads.at(i);
        }
    }
    //无闲则随机
    return mThreads.at( qrand() % mThreads.length() );
}

}
