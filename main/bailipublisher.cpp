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
        mJobs.enqueue(SyncJob(QString(), QString(), 0));
        mWorkCondition.wakeOne();
    }
}

void BsPublisher::stopWait()
{
    mBookWaiting = false;
    bookLogout();
}

void BsPublisher::addJob(const QString &shop, const QString &relSheetTable, const int relSheetId)
{
    if ( !shop.isEmpty() ) {
        mJobs.enqueue(SyncJob(shop, relSheetTable, relSheetId));
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
            SyncJob job;

            //等待任务
            {
                QMutexLocker locker(&mWorkMutex);
                if (mJobs.isEmpty()) {
                    mWorkCondition.wait(&mWorkMutex);
                }

                //取得店名
                job = mJobs.dequeue();
                if ( job.mShop.isEmpty() ) {
                    break;
                }
            }

            //处理
            QSqlDatabase db = QSqlDatabase::database(dbConnName);
            QSqlQuery qry(db);
            qry.setForwardOnly(true);
            qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);

            //是否定过坐标
            qint64 x = 0, y = 0;
            QString sql = QStringLiteral("select amgeo from shop where kname='%1';").arg(job.mShop);
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

            //本店全部标签库存
            QMap<QString, QStringList> mapPubs;   //tag, cargos
            sql = QStringLiteral("select b.amtag, a.cargo, sum(a.qty) as stock "
                                 "from vi_stock as a "
                                 "inner join cargo as b on a.cargo=b.hpcode "
                                 "where a.shop='%1' and chktime>0 and length(b.amtag) > 0 "
                                 "group by b.amtag, a.cargo "
                                 "having sum(a.qty)>0;").arg(job.mShop);
            qry.exec(sql);
            while ( qry.next() ) {
                QString k = qry.value(0).toString();
                QStringList v = mapPubs.value(k);
                v << qry.value(1).toString().trimmed();
                mapPubs.insert(k, v);
            }
            qry.finish();

            //准备参数
            QString dels;
            QStringList tags;
            QStringList goods;

            //判断是否新的一天
            QDateTime lastSync = QDateTime::fromSecsSinceEpoch(mSyncStockLog.value(job.mShop, 0));
            QDateTime thisSync = QDateTime::currentDateTime();
            if ( thisSync.date() != lastSync.date() ) {
                //全部标签都提交（爱美平台特殊约定使用*标记dels）
                dels = QStringLiteral("*");
                QMapIterator<QString, QStringList> it(mapPubs);
                while ( it.hasNext() ) {
                    it.next();
                    tags << it.key();
                    goods << it.value().join(QChar(9));
                }
            }
            else {
                //仅提交本单涉及标签
                QSet<QString> setRels;
                sql = QStringLiteral("select b.amtag, sum(a.qty) as sumqty "
                                     "from vi_%1 as a inner join cargo as b on a.cargo=b.hpcode "
                                     "where a.sheetid=%2 and length(b.amtag)>0 "
                                     "group by b.amtag;")
                        .arg(job.mRelSheetTable).arg(job.mRelSheetId);
                qry.exec(sql);
                while ( qry.next() ) {
                    setRels.insert(qry.value(0).toString());
                }
                qry.finish();

                //整理待删除标签参数
                QStringList delTags;
                QSetIterator<QString> st(setRels);
                while ( st.hasNext() ) {
                    QString tag = st.next();
                    if (!mapPubs.contains(tag)) {
                        delTags << tag;
                    }
                }
                dels = delTags.join(QChar(9));

                //整理新库存参数（如此设计是为大量减少提交数据量）
                QMapIterator<QString, QStringList> it(mapPubs);
                while ( it.hasNext() ) {
                    it.next();
                    if (setRels.contains(it.key())) {
                        tags << it.key();
                        goods << it.value().join(QChar(9));
                    }
                }
            }

            //无打标签库存不请求提交
            if ( tags.isEmpty() ) {
                continue;
            }

            //准备数据
            QStringList params;
            params << QStringLiteral("backer\x01%1").arg(dogNetName)
                   << QStringLiteral("shop\x01%1").arg(job.mShop)
                   << QStringLiteral("x\x01%1").arg(x)
                   << QStringLiteral("y\x01%1").arg(y)
                   << QStringLiteral("dels\x01%1").arg(dels)
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

