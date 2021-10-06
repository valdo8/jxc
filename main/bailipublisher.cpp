#include "bailipublisher.h"
#include "bailifunc.h"
#include "bailicustom.h"

#include <QtSql>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpPart>

//决定报告频率，考虑服务器压力
#define MIN_JOB_SPACE       3

namespace BailiSoft {


void BsPublisher::bookLogin(const QString &dbfile)
{
    mDatabaseFile = dbfile;
    mBookCondition.wakeOne();
}

void BsPublisher::bookLogout()
{
    if ( mDatabaseFile.isEmpty() ) {
        mBookCondition.wakeOne();
    } else {
        mJobs.enqueue(QString());
        mWorkCondition.wakeOne();
    }
}

void BsPublisher::stopWait()
{
    mBookWaiting = false;
    bookLogout();
}

void BsPublisher::addJob(const QString &shop)
{
    if ( !shop.isEmpty() ) {
        mJobs.enqueue(shop);
        mWorkCondition.wakeOne();
    }
}

void BsPublisher::run()
{
    //专用
    QNetworkAccessManager* netMan = new QNetworkAccessManager(nullptr);

    //循环
    while (mBookWaiting) {

        mBookMutex.lock();
        mBookCondition.wait(&mBookMutex);
        mBookMutex.unlock();

        //约定
        if ( mDatabaseFile.isEmpty() ) {
            break;
        }

        //login
        QString dbConnName = generateRandomString(8) + mDatabaseFile;
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), dbConnName);
            db.setDatabaseName(mDatabaseFile);
            db.setConnectOptions("QSQLITE_OPEN_READONLY;");
            if ( !db.open() ) {
                qDebug() << "conn database failed in web thread." << db.lastError() << mDatabaseFile;
                break;
            }
        }

        forever {

            //处理对象
            QString shop;

            //等待任务
            {
                QMutexLocker locker(&mWorkMutex);
                if (mJobs.isEmpty()) {
                    mWorkCondition.wait(&mWorkMutex);
                }

                //取得店名
                shop = mJobs.dequeue();
                if ( shop.isEmpty() ) {
                    break;
                }
            }

            //防频
            if ( shop == mLastShop && QDateTime::currentSecsSinceEpoch() - mLastSeconds < MIN_JOB_SPACE ) {
                continue;
            }

            //处理
            QSqlDatabase db = QSqlDatabase::database(dbConnName);
            QSqlQuery qry(db);
            qry.setForwardOnly(true);
            qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);

            //是否定过坐标
            qint64 x = 0, y = 0;
            QString sql = QStringLiteral("select amgeo from shop where kname='%1';").arg(shop);
            qry.exec(sql);
            if ( qry.next() ) {
                QStringList pairs = qry.value(0).toString().split(QChar(','));
                if ( pairs.length() == 2 ) {
                    double fx = QString(pairs.at(0)).toDouble();
                    double fy = QString(pairs.at(1)).toDouble();
                    x = 1000000 * (fx + 0.00000001);
                    y = 1000000 * (fy + 0.00000001);
                }
            }
            if ( x == 0 && y == 0 ) {
                qry.finish();
                continue;
            }

            //检索货品
            QMap<QString, QStringList> mapPubs;   //tag, cargos
            sql = QStringLiteral("select b.amtag, a.cargo, sum(a.qty) as stock "
                                 "from vi_stock as a "
                                 "inner join cargo as b on a.cargo=b.hpcode "
                                 "where a.shop='%1' and length(b.amtag) > 0 "  //TODO...待改限已审...通知信号也改为审核时发出
                                 "group by b.amtag, a.cargo "
                                 "having sum(a.qty)>0;").arg(shop);
            qry.exec(sql);
            while ( qry.next() ) {
                QString k = qry.value(0).toString();
                QStringList v = mapPubs.value(k);
                v << qry.value(1).toString().trimmed();
                mapPubs.insert(k, v);
            }
            qry.finish();

            //整理结果
            QStringList tags;
            QStringList goods;
            QMapIterator<QString, QStringList> it(mapPubs);
            while ( it.hasNext() ) {
                it.next();
                QString k = it.key();
                QStringList v = it.value();
                tags << k;
                goods << v.join(QChar(9));
            }

            //准备数据
            QStringList params;
            params << QStringLiteral("backer\x01%1").arg(dogNetName)
                   << QStringLiteral("shop\x01%1").arg(shop)
                   << QStringLiteral("x\x01%1").arg(x)
                   << QStringLiteral("y\x01%1").arg(y)
                   << QStringLiteral("tags\x01%1").arg(tags.join(QChar(10)))
                   << QStringLiteral("goods\x01%1").arg(goods.join(QChar(10)));
            QString reqData = params.join(QChar(2));

            //网络提交
            QNetworkRequest req(QUrl::fromUserInput("https://www.aimeiwujia.com/pc/synstock"));
            QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
            sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConf.setProtocol(QSsl::TlsV1SslV3);
            req.setSslConfiguration(sslConf);

            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            setVerifyHeader(&req, httpUserName, httpPassHash);
            QNetworkReply* netReply = netMan->post(req, reqData.toUtf8());

            //等待返回
            QEventLoop loop;
            connect(netReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            netReply->deleteLater();

            if ( netReply->error() ) {
                int sttCode = netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug() << QStringLiteral("网络错误%1，状态码%2").arg(netReply->error()).arg(sttCode);
            } else {
                QByteArray resp = netReply->readAll();
                qDebug() << QString::fromUtf8(resp);
            }

            //暂记
            mLastShop = shop;
            mLastSeconds = QDateTime::currentSecsSinceEpoch();
        }

        //logout
        if ( QSqlDatabase::database(dbConnName, false).isValid() ) {
            QSqlDatabase::removeDatabase(dbConnName);
        }
    }

    //清理
    delete netMan;
}

}

