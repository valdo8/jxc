#include "bailiterminator.h"
#include "bailicode.h"
#include "bailidata.h"
#include "bailifunc.h"
#include "bailiwins.h"
#include "bailishare.h"
#include "third/tinyAES/aes.hpp"

#include <QtSql>

namespace BailiSoft {

BsTerminator::BsTerminator(QObject *parent, const QString &databaseConnectionName) : QThread(parent)
{
    mDatabaseConnectionName = databaseConnectionName;
}

void BsTerminator::taskAdd(const QByteArray &fromServerData)
{
    QMutexLocker locker(&mTransactionMutex);
    mTransactions.enqueue(fromServerData);
    mTransactionAdded.wakeOne();
}

void BsTerminator::taskStop()
{
    mTransactions.enqueue(QByteArray());    //约定空数据结束
    mTransactionAdded.wakeOne();
    wait();
}

bool BsTerminator::isIdle()
{
    return mTransactions.isEmpty();
}

void BsTerminator::run()
{
    //准备
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), mDatabaseConnectionName);
        db.setDatabaseName(loginFile);
        if ( !db.open() ) {
            qDebug() << "open database failed in net thread.";
            return;
        }
    }

    //循环工作
    forever {
        //接收数据
        QByteArray fromServerData;

        //检查事务队列
        {
            //加锁（花括号解锁）
            QMutexLocker locker(&mTransactionMutex);

            //阻塞等事务
            if (mTransactions.isEmpty())
                mTransactionAdded.wait(&mTransactionMutex);

            //取出事务
            fromServerData = mTransactions.dequeue();
            if (fromServerData.isEmpty()) {
                break;
            }
        }

        //报告只是记录即可
        if ( fromServerData.left(3) == QStringLiteral("RPT") ) {
            checkRecordTransReport(fromServerData.mid(3));
            continue;
        }

        //取得请求用户BsFronter*
        BsFronter *requester = BsFronterMap::frontOfId(fromServerData.mid(3, 16));  //exclude header REQ
        if ( ! requester ) {
            qDebug() << "Invalid net requester";
            continue;
        }

        //解密
        QByteArray baDes = dataDecrypt(fromServerData.mid(19));

        //解压
        QByteArray baUnz = dataUnzip(baDes);

        //转码
        QString strPack = QString::fromUtf8(baUnz);

        //解包
        QStringList packFields = strPack.split(QChar('\f'));

        //预处理
        if ( packFields.length() < 2 ) {
            qDebug() << "Invalid packFields";
            continue;
        }

        //准备
        QString reqType = packFields.at(0);
        QString respContent;
        QStringList transToIds;
        qint64 msgId = 0;

        //SQL
        if ( reqType == QStringLiteral("LOGIN") )
            respContent = reqLogin(strPack, requester);

        if ( reqType == QStringLiteral("QRYSHEET") )
            respContent = reqQrySheet(strPack, requester);

        if ( reqType == QStringLiteral("QRYPICK") )
            respContent = reqQryPick(strPack, requester);

        if ( reqType == QStringLiteral("BIZOPEN") )
            respContent = reqBizOpen(strPack, requester);

        if ( reqType == QStringLiteral("BIZEDIT") )
            respContent = reqBizEdit(strPack, requester);

        if ( reqType == QStringLiteral("BIZDELETE") )
            respContent = reqBizDelete(strPack, requester);

        if ( reqType == QStringLiteral("BIZINSERT") )
            respContent = reqBizInsert(strPack, requester);

        if ( reqType == QStringLiteral("FEEINSERT") )
            respContent = reqFeeInsert(strPack, requester);

        if ( reqType == QStringLiteral("REGINSERT") )
            respContent = reqRegInsert(strPack, requester);

        if ( reqType == QStringLiteral("REGCARGO") )
            respContent = reqRegCargo(strPack, requester);

        if ( reqType == QStringLiteral("QRYSUMM") )
            respContent = reqQrySumm(strPack, requester);

        if ( reqType == QStringLiteral("QRYCASH") )
            respContent = reqQryCash(strPack, requester);

        if ( reqType == QStringLiteral("QRYREST") )
            respContent = reqQryRest(strPack, requester);

        if ( reqType == QStringLiteral("QRYSTOCK") )
            respContent = reqQryStock(strPack, requester);

        if ( reqType == QStringLiteral("QRYVIEW") )
            respContent = reqQryView(strPack, requester);

        if ( reqType == QStringLiteral("GETOBJECT") )
            respContent = reqQryObject(strPack, requester);

        if ( reqType == QStringLiteral("GETIMAGE") )
            respContent = reqQryImage(strPack, requester);

        if ( reqType == QStringLiteral("QRYPRINTOWE") )
            respContent = reqQryPrintOwe(strPack, requester);

        if ( reqType == QStringLiteral("MESSAGE") && packFields.length() == 6 ) {
            QString sendTo = packFields.at(4);
            QString recName = (sendTo.length() == 16)
                    ? BsFronterMap::frontOfId(sendTo)->mName
                    : BsMeetingMap::meetingOfId(sendTo.toLongLong())->mMeetName;
            respContent = reqMessage(strPack, requester, recName, &msgId);
            transToIds = getMessageReceiverIds(sendTo, requester);
            if ( msgId == 0 ) {
                continue;   //这是msglog微秒主键重复冲突，几无可能的事件，丢弃没问题。
            }
        }

        //老板管理
        if ( requester->mBosss ) {

            //建群
            if ( reqType == QStringLiteral("GRPCREATE") && packFields.length() > 3 ) {
                respContent = reqGrpCreate(strPack);
                transToIds = calcNamesToIds(QString(packFields.at(4)).split(QChar('\t')));
            }

            //群改名
            if ( reqType == QStringLiteral("GRPRENAME") && packFields.length() > 2 ) {
                respContent = reqGrpRename(strPack);
                transToIds = BsMeetingMap::memberIdsOfMeet(QString(packFields.at(2)).toLongLong());
            }

            //解散群
            if ( reqType == QStringLiteral("GRPDISMISS") && packFields.length() > 2 ) {
                transToIds = BsMeetingMap::memberIdsOfMeet(QString(packFields.at(2)).toLongLong());  //注意要在删除前获取
                respContent = reqGrpDismiss(strPack);
            }

            //拉人
            if ( reqType == QStringLiteral("GRPINVITE") && packFields.length() > 3 ) {
                respContent = reqGrpInvite(strPack);
                transToIds = BsMeetingMap::memberIdsOfMeet(QString(packFields.at(2)).toLongLong());
                transToIds << calcNamesToIds(QString(packFields.at(3)).split(QChar('\t')));
            }

            //踢人
            if ( reqType == QStringLiteral("GRPKICKOFF") && packFields.length() > 2 ) {
                transToIds = BsMeetingMap::memberIdsOfMeet(QString(packFields.at(2)).toLongLong());  //注意要在踢人前获取
                respContent = reqGrpKickoff(strPack);
            }
        }

        //没有约定头部从而没有处理结果，因此要么是因为没有正确解密，要么是因为格式违反约定
        if ( respContent.isEmpty() ) {
            qDebug() << "Bad request: " << reqType << " fields: " << packFields.length();
            continue;
        }

        //压缩（此时requester->versionDate已经过reqLogin函数的重新赋值）
        QByteArray baZip = dataDozip(respContent.toUtf8(), requester->versionDate < 20200920);

        //加密
        QByteArray baEnc = dataEncrypt(baZip);

        //回复
        int packLen = 1                 //R、G、M 类型，公服使用
                + 8                     //msgId.int64，公服报告回传
                + 2                     //转发接受前端数int16，公服使用
                + 16 * 1                //转发接受前端id排列，公服使用
                + baEnc.length();       //内容数据，前端使用
        QByteArray packLenBytes = QByteArray(4, '\0');
        QByteArray frontsCountBytes = QByteArray(2, '\0');
        qToBigEndian<quint32>(packLen, packLenBytes.data());
        qToBigEndian<quint16>(1, frontsCountBytes.data());
        QByteArray pack;
        pack = packLenBytes;
        pack += QByteArray("R");
        pack += generateRandomString(8).toLatin1();  //无意义统一长度占位
        pack += frontsCountBytes;
        pack += requester->mFrontId;
        pack += baEnc;
        emit responseReady(pack);

        //转发
        if ( ! transToIds.isEmpty() ) {

            QByteArray transData = fromServerData.mid(19);
            QByteArray msgIdBytes = QByteArray(8, '\0');
            qToBigEndian<quint64>(quint64(msgId), msgIdBytes.data());
            int packLen = 1                             //R、G、M 类型，公服使用
                    + 8                                 //msgId.int64，公服报告回传
                    + 2                                 //转发接受前端数int16，公服使用
                    + 16 * transToIds.length()          //转发接受前端id排列，公服使用
                    + transData.length();               //内容数据，前端使用
            QByteArray packLenBytes = QByteArray(4, '\0');
            QByteArray frontsCountBytes = QByteArray(2, '\0');
            qToBigEndian<quint32>(quint32(packLen), packLenBytes.data());
            qToBigEndian<quint16>(quint16(transToIds.length()), frontsCountBytes.data());
            QByteArray pack;
            pack = packLenBytes;
            pack += (msgId > 0) ? QByteArray("M") : QByteArray("G");
            pack += msgIdBytes;
            pack += frontsCountBytes;
            pack += transToIds.join(QString());
            pack += transData;
            emit transferReady(pack);
        }
    }

    //结束
    if ( QSqlDatabase::database(mDatabaseConnectionName, false).isValid() ) {
        QSqlDatabase::removeDatabase(mDatabaseConnectionName);
    }
}


QStringList BsTerminator::getMessageReceiverIds(const QString &chatTo, BsFronter *sender)
{
    //接受方表
    QStringList toIds;

    //群组milliSecondsSinceEpochID数字长度一定小于16位
    if ( chatTo.length() == 16 ) {
        //一对一聊天对方shortHashId长度一定为16位
        toIds << chatTo;
    } else {
        //BsMeetingMap初始化加载时已加入总经理
        toIds << BsMeetingMap::memberIdsOfMeet(chatTo.toLongLong());
    }

    //禁止发送自己
    toIds.removeAll(sender->mFrontId);

    //返回
    return toIds;
}

QStringList BsTerminator::calcNamesToIds(const QStringList &names)
{
    QStringList ids;
    foreach (QString name, names) {
        ids << BsFronter::calcShortMd5(name);
    }
    return ids;
}

QByteArray BsTerminator::dataDecrypt(const QByteArray &data)
{
    if ( ! BsBackerInfo::usingCryption() )
        return data;

    if ( data.length() <= AES_BLOCKLEN || (data.length() % AES_BLOCKLEN) )
        return QByteArray();

    //Header block is IV data, so buffLen minus one block size.
    size_t buffLen = size_t(data.length()) - AES_BLOCKLEN;
    uint8_t iv[ AES_BLOCKLEN ];
    memcpy(iv, data.left(AES_BLOCKLEN).constData(), AES_BLOCKLEN);
    struct AES_ctx ctx;

    //malloc memory
    void *heap = malloc(buffLen);
    uint8_t *buff = reinterpret_cast<uint8_t *>(heap);
    memcpy(buff, data.mid(AES_BLOCKLEN).constData(), buffLen);

    //tinyAES does not throw exception, but return empty result.
    try {
        //Flutter's encrypt package uses PKCS7 padding which just as tinyAES.
        AES_init_ctx_iv(&ctx, BsBackerInfo::getCryptionKey(), iv);
        AES_CBC_decrypt_buffer(&ctx, buff, uint32_t(buffLen));
    }
    catch (...) {
        buff[0] = 0;
    }

    //deep copy then free
    QByteArray result = QByteArray(reinterpret_cast<char*>(buff), int(buffLen));
    free(heap);
    heap = nullptr;
    return result;
}

QByteArray BsTerminator::dataEncrypt(const QByteArray &data)
{
    if ( ! BsBackerInfo::usingCryption() )
        return data;

    if ( data.isEmpty() )
        return QByteArray();

    int dataLen = data.length();
    int padding = AES_BLOCKLEN - (dataLen % AES_BLOCKLEN);
    int buffLen = dataLen + padding;
    struct AES_ctx ctx;

    //malloc memory
    void *heap = malloc(AES_BLOCKLEN + size_t(buffLen));                     //Prepending header block of IV data
    uint8_t *buff = reinterpret_cast<uint8_t *>(heap);

    memset(buff + AES_BLOCKLEN + dataLen, padding, size_t(padding));            //PKCS7 padding
    memcpy(buff, generateRandomBytes(AES_BLOCKLEN).constData(), AES_BLOCKLEN);  //prepend IV header
    memcpy(buff + AES_BLOCKLEN, data.constData(), size_t(dataLen));             //data

    AES_init_ctx_iv(&ctx, BsBackerInfo::getCryptionKey(), buff);
    AES_CBC_encrypt_buffer(&ctx, buff + AES_BLOCKLEN, uint32_t(buffLen));       //第三参数应为buffLen，而不是dataLen！

    //deep copy then free
    QByteArray result = QByteArray(reinterpret_cast<char*>(buff), AES_BLOCKLEN + buffLen);
    free(heap);
    heap = nullptr;
    return result;
}

QByteArray BsTerminator::dataDozip(const QByteArray &data, const bool removeLenHead)
{
    return (removeLenHead) ? qCompress(data).mid(4) : qCompress(data);         //结果带有4字节长度头
}

QByteArray BsTerminator::dataUnzip(const QByteArray &data)
{
    QByteArray result;
    try {
        result = qUncompress(data);  //NOTICE: Flutter must prepend bigendian Uint32 of len.
    }
    catch (...) {
        result = QByteArray();
    }
    return result;
}

QString BsTerminator::buildSpecHSum(const QString &sql)
{
    /*
        为节省流量，特在服务端处理好合计。视图sizers明细字段数据格式：
            \r\f负   码名\t数量 \n 码名\t数量 \n 码名\t数量 ...
            \r\v正   码名\t数量 \n 码名\t数量 \n 码名\t数量 ...
    */

    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    QSqlQuery qry(db);
    qry.setForwardOnly(true);
    qry.exec(sql);

    //列定义
    QSqlRecord rec = qry.record();
    int fcount = rec.count();
    int idxSizers = rec.indexOf(QStringLiteral("sizers"));
    int idxQty = rec.indexOf(QStringLiteral("qty"));
    Q_ASSERT(idxSizers >= 0 && idxQty >= 0);
    QStringList fldNames;
    QStringList fldDefines;
    for ( int i = 0; i < fcount; ++i ) {
        QString fname = rec.fieldName(i);
        bool fldQtyy = (fname.toLower() == QStringLiteral("qty"));
        QString dtype = (fldQtyy) ? QStringLiteral("integer default 0") : QStringLiteral("text default ''");
        fldNames << fname;
        fldDefines << QStringLiteral("%1 %2").arg(fname).arg(dtype);
    }
    Q_ASSERT(fldNames.length() >= 2);
    QString fldNamesSql = fldNames.join(QChar(','));
    QString fldDefineSql = fldDefines.join(QChar(','));
    QString batchPattern = QStringLiteral("insert into tmpnetspecpick(%1) values(%2);");

    QStringList batches;
    batches << QStringLiteral("drop table if exists temp.tmpnetspecpick;");
    batches << QStringLiteral("create temp table tmpnetspecpick(%1);").arg(fldDefineSql);

    //行值
    while ( qry.next() ) {
        QStringList fldValues;
        for ( int i = 0; i < fcount; ++i ) {
            if ( i == idxSizers )
                fldValues << QStringLiteral("__sizer__");
            else
                fldValues << QStringLiteral("'%1'").arg(qry.value(i).toString());
        }
        QString valuesPattern = fldValues.join(QChar(','));

        QMap<QString, qint64>   mapSizers;
        QStringList sizersList = qry.value(idxSizers).toString().split(QChar('\r'));

        for ( int i = 0, iLen = sizersList.length(); i < iLen; ++i ) {
            QString sizers = sizersList.at(i);
            if ( sizers.length() > 4 ) {
                bool minuss = sizers.at(0) == QChar('\f');
                QStringList pairs = sizers.mid(1).split(QChar('\n'));
                for ( int j = 0, jLen = pairs.length(); j < jLen; ++j ) {
                    QStringList pair = QString(pairs.at(j)).split(QChar('\t'));
                    QString sizer = pair.at(0);
                    qint64 qty = (minuss)
                            ? (0 - QString(pair.at(1)).toLongLong())
                            : QString(pair.at(1)).toLongLong();
                    qint64 existSum = ( mapSizers.contains(sizer) ) ? mapSizers.value(sizer) : 0;
                    mapSizers.insert(sizer, existSum + qty );
                }
            }
        }
        QStringList lstSizers;
        QMapIterator<QString, qint64> it(mapSizers);
        while ( it.hasNext() ) {
            it.next();
            QString sizer = it.key();
            qint64 qty = it.value();
            lstSizers << QStringLiteral("%1:%2").arg(sizer).arg(qty);
        }
        QString sumedSizers = lstSizers.join(QChar(';'));

        QString values = valuesPattern;
        values.replace(QStringLiteral("__sizer__"), QStringLiteral("'%1'").arg(sumedSizers));
        batches << batchPattern.arg(fldNamesSql).arg(values);
    }
    qry.finish();

    //临时表生成与执行
    db.transaction();
    for ( int i = 0, iLen = batches.length(); i < iLen; ++i ) {
        QString line = batches.at(i);
        db.exec(line);
        if ( db.lastError().isValid() ) {
            qDebug() << db.lastError().text();
            qDebug() << line;
            db.rollback();
            return QString();
        }
    }
    db.commit();

    //按正常方式构造返回
    QString lastSql = QStringLiteral("select %1 from tmpnetspecpick;").arg(fldNames.join(QChar(',')));
    return buildSqlData(lastSql);
}

QString BsTerminator::buildSpecVSum(const QString &sql, const QString &limSizer)
{
    /*
        为节省流量，特在服务端处理好合计。视图sizers明细字段数据格式：
            \r\f负   码名\t数量 \n 码名\t数量 \n 码名\t数量 ...
            \r\v正   码名\t数量 \n 码名\t数量 \n 码名\t数量 ...
    */

    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    QSqlQuery qry(db);
    qry.setForwardOnly(true);
    qry.exec(sql);

    //列定义
    QSqlRecord rec = qry.record();
    int fcount = rec.count();
    int idxSizers = rec.indexOf(QStringLiteral("sizers"));
    int idxQty = rec.indexOf(QStringLiteral("qty"));
    //Q_ASSERT(idxSizers >= 0 && idxQty >= 0);
    if ( idxSizers < 0 || idxQty < 0 ) {
        return QStringLiteral("Fatal error when buildSpecVSum");
    }
    QStringList fldNames;
    QStringList fldDefines;
    QStringList fldSels;
    for ( int i = 0; i < fcount; ++i ) {
        QString fname = rec.fieldName(i);
        bool fldQtyy = (fname.toLower() == QStringLiteral("qty"));
        QString dtype = (fldQtyy) ? QStringLiteral("integer default 0") : QStringLiteral("text default ''");
        if ( fname == QStringLiteral("sizers") )
            fname = QStringLiteral("sizer");
        fldNames << fname;
        fldDefines << QStringLiteral("%1 %2").arg(fname).arg(dtype);
        if ( !fldQtyy ) {
            fldSels << fname;
        }
    }
    QString fldNamesSql = fldNames.join(QChar(','));
    QString fldDefineSql = fldDefines.join(QChar(','));
    QString batchPattern = QStringLiteral("insert into tmpnetspecqry(%1) values(%2);");

    QStringList batches;
    batches << QStringLiteral("drop table if exists temp.tmpnetspecqry;");
    batches << QStringLiteral("create temp table tmpnetspecqry(%1);").arg(fldDefineSql);

    //行值
    while ( qry.next() ) {
        QStringList fldValues;
        for ( int i = 0; i < fcount; ++i ) {
            if ( i == idxSizers )
                fldValues << QStringLiteral("__sizer__");
            else if ( i == idxQty )
                fldValues << QStringLiteral("__qty__");
            else
                fldValues << QStringLiteral("'%1'").arg(qry.value(i).toString());
        }
        QString valuesPattern = fldValues.join(QChar(','));

        QStringList sizersList = qry.value(idxSizers).toString().split(QChar('\r'));
        for ( int i = 0, iLen = sizersList.length(); i < iLen; ++i ) {
            QString sizers = sizersList.at(i);
            if ( sizers.length() > 4 ) {
                bool minuss = sizers.at(0) == QChar('\f');
                QStringList pairs = sizers.mid(1).split(QChar('\n'));
                for ( int j = 0, jLen = pairs.length(); j < jLen; ++j ) {
                    QStringList pair = QString(pairs.at(j)).split(QChar('\t'));
                    QString sizer = pair.at(0);
                    QString szqty = pair.at(1);
                    if ( limSizer.isEmpty() || sizer == limSizer ) {
                        qint64 qty = (minuss) ? 0 - szqty.toLongLong() : szqty.toLongLong();
                        QString values = valuesPattern;
                        values.replace(QStringLiteral("__sizer__"), QStringLiteral("'%1'").arg(sizer))
                                .replace(QStringLiteral("__qty__"), QString::number(qty));
                        batches << batchPattern
                                   .arg(fldNamesSql)
                                   .arg(values);
                    }
                }
            }
        }
    }
    qry.finish();

    //临时表生成与执行
    db.transaction();
    for ( int i = 0, iLen = batches.length(); i < iLen; ++i ) {
        QString line = batches.at(i);
        db.exec(line);
        if ( db.lastError().isValid() ) {
            qDebug() << db.lastError().text();
            qDebug() << line;
            db.rollback();
            return QString();
        }
    }
    db.commit();

    //按正常方式构造返回
    QString lastSels = fldSels.join(QChar(','));
    QString lastSql = ( fldSels.length() > 0 )
            ? QStringLiteral("select %1, sum(tmpnetspecqry.qty) as qty "
                             "from tmpnetspecqry "
                             "group by %1 "
                             "having sum(tmpnetspecqry.qty)<>0;").arg(lastSels)
            : QStringLiteral("select sum(tmpnetspecqry.qty) as qty "
                             "from tmpnetspecqry;");
    return buildSqlData(lastSql);
}

QString BsTerminator::buildSqlData(const QString &sql, const char replaceTabChar, const char replaceLineChar)
{
    QStringList rows;
    QSqlQuery qry(QSqlDatabase::database(mDatabaseConnectionName));
    qry.setForwardOnly(true);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) {
        qDebug() << qry.lastError().text();
        qDebug() << sql;
    }

    //列名
    QSqlRecord rec = qry.record();
    int fcount = rec.count();
    QStringList flds;
    for ( int i = 0; i < fcount; ++i ) {
        flds << rec.fieldName(i);
    }
    rows << flds.join(QChar('\t'));

    //行值
    while ( qry.next() ) {
        QStringList cols;
        for ( int i = 0; i < fcount; ++i ) {
            QString fvalue = qry.value(i).toString();
            if ( replaceTabChar ) fvalue.replace(QChar('\t'), QChar(replaceTabChar));
            if ( replaceLineChar ) fvalue.replace(QChar('\n'), QChar(replaceLineChar));
            cols << fvalue;
        }
        rows << cols.join(QChar('\t'));
    }
    qry.finish();

    //返回行格式
    return rows.join(QChar('\n'));
}

void BsTerminator::checkRecordTransReport(const QByteArray &rptData)
{
    QSqlQuery qry(QSqlDatabase::database(mDatabaseConnectionName));
    qry.prepare("insert into msgfail(msgid, tofrontid) values (?, ?);");

    qint64 msgId = qFromBigEndian<qint64>(rptData.left(8).constData());
    QByteArray lostBytes = rptData.mid(8);

    QVariantList msgIds;
    QVariantList frontIds;
    for ( int i = 0; i <= lostBytes.length() - 17; i += 17 ) {
        if ( lostBytes.at(i + 16) == 'N' ) {
            msgIds << msgId;
            frontIds << QString::fromLatin1(lostBytes.mid(i, 16));
        }
    }
    if ( msgIds.length() > 0 ) {
        qry.addBindValue(msgIds);
        qry.addBindValue(frontIds);
        qry.execBatch();
        if ( qry.lastError().isValid() ) {
            qDebug() << "msgfail: " << qry.lastError().text();
        }
    }
}

void BsTerminator::serverLog(const QString &reqMan, const int reqType, const QString &reqInfo)
{
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    QString content = reqInfo;
    content.replace(QChar(39), QString());
    QString sql = QStringLiteral("insert into serverlog(reqtime, reqman, reqtype, reqinfo) "
                                 "values(%1, '%2', %3, '%4');")
            .arg(QDateTime::currentMSecsSinceEpoch()).arg(reqMan).arg(reqType).arg(content);
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        qDebug() << db.lastError().text();
        qDebug() << sql;
    }
}

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ 协议通用参数 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 *$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
【REQUEST】
    0：请求标志，固定为大写三字母请求名
    1：请求ID(EpochMicroSeconds)，用于防止重复事务

【RESPONSE】
    0：返回标志，同请求标志
    1：返回ID，同请求ID，用于防止重复事务
    last：OK或错误信息
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

QString BsTerminator::reqLogin(const QString &packstr, BsFronter *user)
{
/*  【REQUEST】
        2：请求时间EpochMilliSeconds，0表示新登录，需返回全部登记；正数时间则只需返回该时间以后有更新的登记。
        3: 前端类型————mobile或desk

    【RESPONSE】
        2：barcodeRule 值行表...\n...\n...（\n \t）首行字段名
        3：sizertype 值行表...\n...\n...（\n \t）首行字段名
        4：colortype 值行表...\n...\n...（\n \t）首行字段名
        5：cargo 值行表...\n...\n...（\n \t）首行字段名
        6：shop 值行表...\n...\n...（\n \t）首行字段名
        7: customer ( user->bindTrader.isEmpty() && user->canLott )
        8: supplier ( user->bindTrader.isEmpty() && user->canBuyy )
        9：staff 值行表...\n...\n...（\n \t）首行字段名
        10：subject 值行表...\n...\n...（\n \t）首行字段名
        11：stypes
        12：bailioption
        13：loginer(only row of self for query sheet rights) 值行表...\n...\n...（\n \t）首行字段名
        14: meeting 值行表...\n...\n...（\n \t）首行字段名
        15: 可单聊对象（仅总经理返回，其他普通账号返回空内容）
        16: 离线消息
        17: 价格政策（迭代升级增加都放在此后）
        18: 总经理账号名称
*/

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() < 3 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //请求时间（用于节省流量）
    qint64 reqEpoch = QString(params.at(2)).toULongLong() / 1000;
    qint64 conEpoch = reqEpoch - 3600 * 24 * 2;  //提前2天，确保时间误差不会漏反馈（允许多反馈）
    QString uptimeExp = (reqEpoch) ? QStringLiteral(" where uptime>=%1 ").arg(conEpoch) : QString();

    //前端类型
    if ( params.length() > 3 ) {

        //协议版本标识，用于前端兼容识别
        user->versionDate = 20200923;

        //前端类型检查
        if ( ! user->mBosss ) {
            if ( (params.at(3) == QStringLiteral("desk")) ) {
                if ( user->mDeskPass.isEmpty() ) {
                    respList << QStringLiteral("手机平板账号禁止使用电脑端登录！");
                    return respList.join(QChar('\f'));
                }
            } else {
                if ( !user->mDeskPass.isEmpty() ) {
                    respList << QStringLiteral("电脑端账号禁止使用手机平板登录！");
                    return respList.join(QChar('\f'));
                }
            }
        }
    }
    else {
        //防止前端换设备登录，并且从新版本换到老版本设备登录，因此，必须重置。
        //目前应该没有未升级的前端了，一年后直接改为非法请求就可————2020-11-20记
        user->versionDate = 0;
    }

    //货号敏感字段
    QString cargoFlds = QStringLiteral("hpcode, hpname, sizertype, colortype, unit, setprice");
    if ( user->canBuyy ) cargoFlds += QStringLiteral(", buyprice"); else cargoFlds += QStringLiteral(", 0 as buyprice");
    if ( user->canLott ) cargoFlds += QStringLiteral(", lotprice"); else cargoFlds += QStringLiteral(", 0 as lotprice");
    if ( user->canRett ) cargoFlds += QStringLiteral(", retprice"); else cargoFlds += QStringLiteral(", 0 as retprice");

    //barcoderule
    respList << buildSqlData(QStringLiteral("select "
                                                "barcodexp,sizermiddlee, barcodemark "
                                                "from barcoderule %1;")
                             .arg(uptimeExp));

    //sizertype
    respList << buildSqlData(QStringLiteral("select "
                                                "tname, namelist, codelist "
                                                "from sizertype %1;")
                             .arg(uptimeExp));

    //colortype
    respList << buildSqlData(QStringLiteral("select "
                                                "tname, namelist, codelist "
                                                "from colortype %1;")
                             .arg(uptimeExp));

    //cargo
    QStringList cargoWhereExps;
    if ( ! user->limCargoExp.isEmpty() ) cargoWhereExps << QStringLiteral("cargo like '%1'").arg(user->limCargoExp);
    if ( reqEpoch > 0 ) cargoWhereExps << QStringLiteral("uptime>=%1 ").arg(conEpoch);
    QString cargoWhere = (cargoWhereExps.length() > 0)
            ? QStringLiteral("where %1").arg(cargoWhereExps.join(QStringLiteral(" and ")))
            : QString();
    respList << buildSqlData(QStringLiteral("select "
                                                "%1 "
                                                "from cargo %2;")
                             .arg(cargoFlds).arg(cargoWhere));

    //shop
    respList << buildSqlData(QStringLiteral("select kname, regdis, regman, regaddr, regtele "
                                                "from shop %1;")
                             .arg(uptimeExp));

    //customer
    if ( user->bindTrader.isEmpty() && user->canLott ) {
        respList << buildSqlData(QStringLiteral("select kname, regdis, regman, regaddr, regtele "
                                                "from customer "
                                                "where uptime>=%1 and kname not like '1__________';")
                                 .arg(reqEpoch - 3600 * 24 * 2));  //手机1字头11位数字
    }
    else {
        respList << QString();
    }

    //supplier
    if ( user->bindTrader.isEmpty() && user->canBuyy ) {
        respList << buildSqlData(QStringLiteral("select kname, regdis, regman, regaddr, regtele "
                                                "from supplier "
                                                "where uptime>=%1 and kname not like '1__________';")
                                 .arg(reqEpoch - 3600 * 24 * 2));  //手机1字头11位数字
    }
    else {
        respList << QString();
    }

    //staff
    respList << buildSqlData(QStringLiteral("select "
                                            "kname "
                                            "from staff %1;")
                             .arg(uptimeExp));

    //subject（此处虽然设计返回，但目前为止前端事实上接收加载，但收支单只提交备注，并没有使用）
    QStringList subjectWhereExps;
    if ( ! user->mBosss ) subjectWhereExps << QStringLiteral("adminboss=0");
    if ( reqEpoch > 0 ) subjectWhereExps << QStringLiteral("uptime>=%1 ").arg(conEpoch);
    QString subjectWhere = (subjectWhereExps.length() > 0)
            ? QStringLiteral("where %1").arg(subjectWhereExps.join(QStringLiteral(" and ")))
            : QString();
    respList << buildSqlData(QStringLiteral("select kname as hpcode from subject %1;")
                            .arg(subjectWhere));

    //单据分类
    respList << buildSqlData(QStringLiteral("select vsetting from bailioption where optcode='stypes_cgd' union all "
                                                "select vsetting from bailioption where optcode='stypes_cgj' union all "
                                                "select vsetting from bailioption where optcode='stypes_cgt' union all "
                                                "select vsetting from bailioption where optcode='stypes_dbd' union all "
                                                "select vsetting from bailioption where optcode='stypes_syd' union all "
                                                "select vsetting from bailioption where optcode='stypes_pfd' union all "
                                                "select vsetting from bailioption where optcode='stypes_pff' union all "
                                                "select vsetting from bailioption where optcode='stypes_pft' union all "
                                                "select vsetting from bailioption where optcode='stypes_lsd' union all "
                                                "select vsetting from bailioption where optcode='stypes_szd'"));

    //其他选项（注意与前台协议顺序）
    QString logoPlace = (reqEpoch > 0) ? QStringLiteral("'' as ") : QString();  //节省流量
    respList << buildSqlData(QStringLiteral("select optcode, vsetting from bailioption where optcode='dots_of_qty' union all "
                                                "select optcode, vsetting from bailioption where optcode='dots_of_price' union all "
                                                "select optcode, vsetting from bailioption where optcode='dots_of_money' union all "
                                                "select optcode, vsetting from bailioption where optcode='dots_of_discount' union all "
                                                "select optcode, vsetting from bailioption where optcode='app_company_name' union all "
                                                "select optcode, vsetting from bailioption where optcode='app_company_pcolor' union all "
                                                "select optcode, %1vsetting from bailioption where optcode='app_company_plogo';")
                             .arg(logoPlace));

    //rights of self (loginer fields)
    if ( user->mBosss )
        respList << QString();
    else {
        const QString sql = QStringLiteral(
                    "select "
                    "bindshop, bindcustomer, bindsupplier, "
                    "loginmobile, customAllow, customDeny, "
                    "retprice, lotprice, buyprice, "
                    "cgd, cgj, cgt, pfd, pff, pft, lsd, dbd, syd, szd, "
                    "vicgd, vicgj, vicgt, vipfd, vipff, vipft, vilsd, vidbd, visyd, viszd, "  //qty, actmoney, dismoney (general)
                    "vicg, vipf, vixs, "                //qty, actmoney, dismoney (general)
                    "vicgcash, vipfcash, vixscash, "    //sumqty, summoney, sumdis, actpay, actowe
                    "vicgrest, vipfrest, "              //qty, actmoney, dismoney (general)
                    "vistock, "                         //qty, actmoney, dismoney (general)
                    "viall "                            //qty(手机禁查，因为复杂)
                    "from baililoginer where loginer='%1';").arg(user->mName);
        respList << buildSqlData(sql);
    }

    //所在群
    if ( user->mBosss )
        respList << buildSqlData(QStringLiteral("select meetid, meetname, members from meeting;"), '\v');
    else
        respList << buildSqlData(QStringLiteral("select meetid, meetname, members from meeting "
                                                    "where (members like '%\t%1\t%') "
                                                    "or (members like '%1\t%') "
                                                    "or (members like '%\t%1');")
                                 .arg(user->mName), '\v');

    //其他前端
    if ( user->mBosss )
        respList << buildSqlData(QStringLiteral("select loginer from baililoginer where length(passhash)>0;"));
    else
        respList << QString();

    //离线消息
    respList << buildSqlData(QStringLiteral("select a.msgid, a.senderid, a.sendername, a.receiverid, a.content "
                                            "from msglog a inner join msgfail b on a.msgid=b.msgid "
                                            "where b.tofrontid='%1';").arg(user->mFrontId));
    QString sql = QStringLiteral("delete from msgfail where tofrontid='%1';").arg(user->mFrontId);
    QSqlDatabase::database(mDatabaseConnectionName).exec(sql);

    //价格政策
    if ( user->bindTrader.isEmpty() ) {
        respList << buildSqlData(QStringLiteral("select traderExp, cargoExp, policyDis, useLevel, startDate, endDate "
                                                "from lotpolicy "
                                                "order by useLevel desc; "));
    } else {
        respList << QString();
    }

    //总经理账号
    respList << bossAccount;

    //日志
    serverLog(user->mName, 1, QString::number(reqEpoch));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//only desk client
QString BsTerminator::reqQrySheet(const QString &packstr, const BsFronter *user)
{
/*  【REQUEST】
        2：单据主表名
        3：dateb
        4：datee
        5: shop
        6: trader
        7: stype
        8: staff
        9: checkk (empty不论、0未审、1仅审)

    【RESPONSE】
        2：值行表...\n...\n...（\n \t） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 10 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);
    if ( user->mDeskPass.isEmpty() ) {
        respList << QStringLiteral("手机平板账号禁止使用电脑端登录！");
        return respList.join(QChar('\f'));
    }

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    QString datebText = params.at(3).trimmed();
    QString dateeText = params.at(4).trimmed();
    QString shop = QString(params.at(5)).trimmed();
    QString trader = QString(params.at(6)).trimmed();
    QString stype = QString(params.at(7)).trimmed();
    QString staff = QString(params.at(8)).trimmed();
    int checkk = QString(params.at(9)).trimmed().toInt();

    QDateTime dateb = QDateTime(dateOfFormattedText(datebText, '-'));
    QDateTime datee = QDateTime(dateOfFormattedText(dateeText, '-'));

    //权限
    if ( ! BsFronterMap::actionAllow(user, tname, QStringLiteral("open")) ) {
        respList << QStringLiteral("没有该项操作权限");
        return respList.join(QChar('\f'));
    }

    if ( ! user->bindShop.isEmpty() ) {
        if ( shop != user->bindShop && trader != user->bindShop ) {
            respList << QStringLiteral("Illegal shop bind.");
            return respList.join(QChar('\f'));
        }
    }

    if ( ! user->bindTrader.isEmpty() ) {
        if ( trader != user->bindTrader ) {
            respList << QStringLiteral("Illegal trader bind.");
            return respList.join(QChar('\f'));
        }
    }

    QStringList limExps;
    limExps << QStringLiteral("(dated between %1 and %2)")
             .arg(dateb.toSecsSinceEpoch()).arg(datee.toSecsSinceEpoch());

    if ( !shop.isEmpty() ) {
        limExps << QStringLiteral("(shop='%1')").arg(shop);
    }

    if ( !trader.isEmpty() ) {
        limExps << QStringLiteral("(trader='%1')").arg(trader);
    }

    if ( !stype.isEmpty() ) {
        limExps << QStringLiteral("(stype='%1')").arg(stype);
    }

    if ( !staff.isEmpty() ) {
        limExps << QStringLiteral("(staff='%1')").arg(staff);
    }

    if ( checkk == 1 ) {
        limExps << QStringLiteral("chktime<>0");
    }

    if ( checkk == 2 ) {
        limExps << QStringLiteral("chktime=0");
    }

    //summoney、actpay、actowe值权限不管，因为电脑端有硬件狗控制发放。
    QString sql = QStringLiteral("select sheetid, dated, stype, staff, shop, trader, "
                                 "sumqty, summoney, actpay, actowe, chktime from %1 "
                                 "where %2 order by sheetid;")
            .arg(tname).arg(limExps.join(QStringLiteral(" and ")));

    //db execute
    //qDebug() << "bizlist sql:" << sql;
    respList << buildSqlData(sql);

    //日志
    serverLog(user->mName, 5, QStringLiteral("%1: 查单 %2-%3 %4~%5")
              .arg(tname).arg(shop).arg(trader).arg(datebText).arg(dateeText));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//only desk client
QString BsTerminator::reqQryPick(const QString &packstr, const BsFronter *user)
{
    /*  【REQUEST】
            2: shop ———— 字符串
            3: colortype ———— 色系    => 现改为attrX条件“x \t value \n x \t value ...”
            4: sizertype ———— 码类
            5: datee ———— 字符串格式“2020-01-01” 或空不指定
            6: checkk ———— 0 不管、1 仅审

        【RESPONSE】
            2：值行表...\n...\n...（\n \t）（仅色码明细有多行） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 7 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    QString shop = QString(params.at(2)).trimmed();
    QString colortype = QString(params.at(3)).trimmed();
    QString sizertype = QString(params.at(4)).trimmed();
    QString dateeText = QString(params.at(5)).trimmed();
    QDateTime datee = QDateTime(dateOfFormattedText(dateeText, '-'));
    int checkk = QString(params.at(6)).trimmed().toInt();

    //按权限取值
    QString viRightKey = QStringLiteral("vistock");
    if ( ! BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("qty")) ) {
        respList << QStringLiteral("无此查询权限");
        return respList.join(QChar('\f'));
    }

    //限定范围
    QStringList limExps;
    if ( dateeText.length() >= 8 )
        limExps << QStringLiteral("dated <= %1").arg(datee.toMSecsSinceEpoch() / 1000);

    if ( ! shop.isEmpty() )
        limExps << QStringLiteral("shop='%1'").arg(shop);

    if ( ! colortype.isEmpty() ) {
        if ( colortype.contains(QChar('\t')) ) {
            QStringList attrs = colortype.split(QChar('\n'));
            for ( int i = 0, iLen = attrs.length(); i < iLen; ++i ) {
                QStringList ps = QString(attrs.at(i)).split(QChar('\t'));
                if ( ps.length() == 2 ) {
                    int x = QString(ps.at(0)).toInt();
                    QString v = QString(ps.at(1));
                    v = v.replace(QChar(39), QString());
                    if ( x >= 1 && x <= 6 ) {
                        limExps << QStringLiteral("attr%1='%2'").arg(x).arg(v);
                    }
                }
            }
        }
        //客户端1.16版以前保留，后期可除。
        else {
            limExps << QStringLiteral("colortype='%1'").arg(colortype);
        }
    }

    if ( ! sizertype.isEmpty() )
        limExps << QStringLiteral("sizertype='%1'").arg(sizertype);

    if ( checkk )
        limExps << QStringLiteral("chktime<>0");

    QString strWhere = (limExps.isEmpty())
            ? QString()
            : QStringLiteral("where %1").arg(limExps.join(QStringLiteral(" and ")));

    //sql
    QString sql = QStringLiteral("select cargo, color, group_concat(vi_stock_attr.sizers, '') as sizers, "
                                 "sum(vi_stock_attr.qty) as qty from vi_stock_attr %1 "
                                 "group by cargo, color;").arg(strWhere);

    //db execute
    respList << buildSpecHSum(sql);

    //日志
    serverLog(user->mName, 8, QStringLiteral("%1 （%2）拣货").arg(shop).arg(sizertype));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//only desk client
QString BsTerminator::reqBizOpen(const QString &packstr, const BsFronter *user)
{
/*  【REQUEST】
        2：单据主表名
        3：单据号sheetid

    【RESPONSE】
        2：单据主表名
        3: 单据号sheetid
        4: 主表值单行...\t...\t...
        5：从表值行表...\n...\n...（\n \t）其中sizers字段中的\n\t分别用:;代替 */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 4 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);
    if ( user->mDeskPass.isEmpty() ) {
        respList << QStringLiteral("手机平板账号禁止使用电脑端登录！");
        return respList.join(QChar('\f'));
    }

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    qint64 sheetid = QString(params.at(3)).trimmed().toLongLong();

    //权限
    if ( ! BsFronterMap::actionAllow(user, tname, QStringLiteral("open")) ) {
        respList << QStringLiteral("没有该项操作权限");
        return respList.join(QChar('\f'));
    }

    //主表
    QString limitMain = QStringLiteral("where sheetid=%1").arg(sheetid);

    if ( ! user->bindShop.isEmpty() ) {
        limitMain += QStringLiteral(" and (shop='%1')").arg(user->bindShop);
    }
    if ( ! user->bindTrader.isEmpty() ) {
        limitMain += QStringLiteral(" and (trader='%1')").arg(user->bindTrader);
    }

    QString sqlMain = QStringLiteral("select sheetid, proof, dated, stype, staff, shop, trader, remark, "
                                     "sumqty, summoney, sumdis, actpay, actowe, checker, chktime, upman, uptime "
                                     "from %1 %2;")
            .arg(tname).arg(limitMain);

    respList << tname;
    respList << QString::number(sheetid);
    respList << buildSqlData(sqlMain);

    //从表
    QString limitDetail = QStringLiteral("where parentid=%1").arg(sheetid);

    QString sqlDetail = QStringLiteral("select parentid, rowtime, cargo, color, sizers, qty, price, discount, "
                                       "actmoney, dismoney, rowmark "
                                       "from %1dtl %2;").arg(tname).arg(limitDetail);

    respList << buildSqlData(sqlDetail, ':', ';');

    //日志
    serverLog(user->mName, 5, QStringLiteral("%1: 打开 %2").arg(tname).arg(sheetid));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//only desk client
QString BsTerminator::reqBizEdit(const QString &packstr, const BsFronter *user)
{
/*  【REQUEST】
        2：单据主表名
        3: 单据号sheetid                                                                  【使用全删后全增法，故而如下】
        4：主表值...（\t）    约定字段：shop,trader,stype,staff,remark,actpay              【完全同reqBizInsert约定】
        5：从表值行...\n...\n...（\n \t）  约定字段：cargo,color,sizer,qty,price,rowmark   【完全同reqBizInsert约定】

     【RESPONSE】
        2：新单号
        3：记录时间 */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 6 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);
    if ( user->mDeskPass.isEmpty() ) {
        respList << QStringLiteral("手机平板账号禁止使用电脑端登录！");
        return respList.join(QChar('\f'));
    }

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    qint64 sheetid = QString(params.at(3)).trimmed().toLongLong();

    //权限
    if ( ! BsFronterMap::actionAllow(user, tname, QStringLiteral("upd")) ) {
        respList << QStringLiteral("没有该项操作权限");
        return respList.join(QChar('\f'));
    }

    //sqls
    QStringList batches;

    //取得全删sqls
    QStringList delParams;
    delParams << params.at(0);
    delParams << params.at(1);
    delParams << params.at(2);
    delParams << params.at(3);
    QString delResult = reqBizDelete(delParams.join(QChar('\f')), user, true);  //第三参数重要
    QStringList delSqls = delResult.split(QChar('\f'));
    if ( delSqls.length() != 3 || delSqls.at(0) != QStringLiteral("OK") ) {
        respList << QStringLiteral("全删处理错误");
        return respList.join(QChar('\f'));
    }
    batches << delSqls.mid(1);

    //取得全添sqls
    QStringList insParams;
    insParams << params.at(0);
    insParams << params.at(1);
    insParams << params.at(2);
    insParams << params.at(4);
    insParams << params.at(5);
    QString insResult = reqBizInsert(insParams.join(QChar('\f')), user, sheetid);  //第三参数重要
    QStringList insSqls = insResult.split(QChar('\f'));
    if ( insSqls.length() < 7 || insSqls.at(0) != QStringLiteral("OK") ) {
        respList << QStringLiteral("全增处理错误");
        return respList.join(QChar('\f'));
    }

    QString shopValue = insSqls.at(1);
    QString traderValue = insSqls.at(2);
    qint64 dqtySum = QString(insSqls.at(3)).toLongLong();
    qint64 dmnySum = QString(insSqls.at(4)).toLongLong();
    qint64 actpayValue = QString(insSqls.at(5)).toLongLong();
    qint64 uptimeValue = QString(insSqls.at(6)).toLongLong();
    batches << insSqls.mid(7);

    //执行
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    db.transaction();
    for ( int i = 0, iLen = batches.length(); i < iLen; ++i ) {
        QString line = batches.at(i);
        db.exec(line);
        if ( db.lastError().isValid() ) {
            qDebug() << db.lastError().text();
            qDebug() << line;
            db.rollback();
            respList << QStringLiteral("事务失败");
            return respList.join(QChar('\f'));
        }
    }
    db.commit();

    //日志
    serverLog(user->mName, 2, QStringLiteral("%1改: %2-%3 %4件 %5~%6元")
              .arg(tname)
              .arg(shopValue)
              .arg(traderValue)
              .arg(dqtySum / 10000)
              .arg(dmnySum / 10000)
              .arg(actpayValue / 10000));

    //返回数据
    respList << QString::number(sheetid);
    respList << QString::number(uptimeValue);

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//only desk client
QString BsTerminator::reqBizDelete(const QString &packstr, const BsFronter *user, const bool sqlsForEdit)
{
/*  【REQUEST】
        2：单据主表名
        3：主单据号sheetid

    【RESPONSE】
        2：sheetid
        3: delAsUpdtime */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 4 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);
    if ( user->mDeskPass.isEmpty() ) {
        respList << QStringLiteral("手机平板账号禁止使用电脑端登录！");
        return respList.join(QChar('\f'));
    }

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    qint64 sheetid = QString(params.at(3)).trimmed().toLongLong();

    //权限
    if ( ! sqlsForEdit ) {
        if ( ! BsFronterMap::actionAllow(user, tname, QStringLiteral("del")) ) {
            respList << QStringLiteral("没有该项操作权限");
            return respList.join(QChar('\f'));
        }
    }

    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    QSqlQuery qry(db);
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec(QStringLiteral("SELECT shop, trader FROM %1 WHERE sheetid=%2;").arg(tname).arg(sheetid));
    if ( qry.lastError().isValid() ) {
        respList << QStringLiteral("服务器意外故障");
        return respList.join(QChar('\f'));
    }
    if ( qry.next() ) {
        if ( ! user->bindShop.isEmpty() ) {
            if ( qry.value(0).toString() != user->bindShop ) {
                respList << QStringLiteral("Illegal shop bind.");
                return respList.join(QChar('\f'));
            }
        }
        if ( ! user->bindTrader.isEmpty() ) {
            if ( qry.value(1).toString() != user->bindTrader ) {
                respList << QStringLiteral("Illegal trader bind.");
                return respList.join(QChar('\f'));
            }
        }
    }

    //sqls
    qint64 delAsUpdtime = QDateTime::currentSecsSinceEpoch();
    QStringList sqls;
    if ( sqlsForEdit ) {
        sqls << QStringLiteral("delete from %1 where sheetid=%2;").arg(tname).arg(sheetid);
    } else {
        sqls << QStringLiteral("update %1 set dated=0, proof='', stype='', staff='', shop='', trader='', remark='', "
                               "sumqty=0, summoney=0, sumdis=0, actpay=0, actowe=0, upman='%2', uptime='%3' "
                               "where sheetid=%4;").arg(tname).arg(user->mName).arg(delAsUpdtime).arg(sheetid);
    }

    sqls << QStringLiteral("delete from %1dtl where parentid=%2;").arg(tname).arg(sheetid);

    //特殊调用
    if ( sqlsForEdit ) {
        sqls.prepend(QStringLiteral("OK"));
        return sqls.join(QChar('\f'));
    }

    //执行
    db.transaction();
    for ( int i = 0, iLen = sqls.length(); i < iLen; ++i ) {
        QString line = sqls.at(i);
        db.exec(line);
        if ( db.lastError().isValid() ) {
            qDebug() << db.lastError().text();
            qDebug() << line;
            db.rollback();
            respList << QStringLiteral("事务失败");
            return respList.join(QChar('\f'));
        }
    }
    db.commit();

    //日志
    serverLog(user->mName, 2, QStringLiteral("%1删: %2").arg(tname).arg(sheetid));

    //返回
    respList << QString::number(sheetid);
    respList << QString::number(delAsUpdtime);

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile 由于考虑到手机端，明细行是一码一码的
QString BsTerminator::reqBizInsert(const QString &packstr, const BsFronter *user, const qint64 updSheetId)
{
/*  【REQUEST】
        2：单据主表名
        3：主表值...(\t)         约定字段：shop,trader,stype,staff,remark,actpay
        4：从表值行...(\t\n)     约定字段：cargo,color,sizer,qty,price,rowmark

    【RESPONSE】
        2：新单号
        3：记录时间 */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 5 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    QString tname = QString(params.at(2)).trimmed().toLower();
    QStringList mRawValues = QString(params.at(3)).split(QChar('\t'));
    QStringList dLines = QString(params.at(4)).split(QChar('\n'));
    if ( mRawValues.length() != 6 ) {
        respList << QStringLiteral("参数格式错误");
        return respList.join(QChar('\f'));
    }
    QString shopValue = mRawValues.at(0);
    QString traderValue = mRawValues.at(1);
    QString stypeValue = mRawValues.at(2);
    QString staffValue = mRawValues.at(3);
    QString remarkValue = mRawValues.at(4);
    qint64 actpayValue = QString(mRawValues.at(5)).toLongLong();
    qint64 uptimeValue = QDateTime::currentSecsSinceEpoch();
    qint64 datedValue = QDateTime(QDate::currentDate()).toSecsSinceEpoch();  //必须为0点时

    //权限
    if ( updSheetId == 0 ) {  //updSheetId参数专为reqBizEdit调用设计，为0表示新增
        if ( ! BsFronterMap::actionAllow(user, tname, QStringLiteral("new")) ) {
            respList << QStringLiteral("没有该项操作权限");
            return respList.join(QChar('\f'));
        }
    }
    if ( ! user->bindShop.isEmpty() ) {
        if ( shopValue != user->bindShop ) {
            respList << QStringLiteral("Illegal shop bind.");
            return respList.join(QChar('\f'));
        }
    }
    if ( ! user->bindTrader.isEmpty() ) {
        if ( traderValue != user->bindTrader ) {
            respList << QStringLiteral("Illegal trader bind.");
            return respList.join(QChar('\f'));
        }
    }

    //备用
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    QSqlQuery qry(db);
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);

    //新sheetid
    int sheetId;
    if ( updSheetId > 0 ) {
        sheetId = updSheetId;
    }
    else {
        qry.exec(QStringLiteral("SELECT seq FROM sqlite_sequence WHERE name='%1';").arg(tname));
        if ( qry.lastError().isValid() ) {
            respList << QStringLiteral("服务器意外故障");
            return respList.join(QChar('\f'));
        }
        if ( qry.next() )
            sheetId = qry.value(0).toInt() + 1;
        else
            sheetId = 1;
        qry.finish();
    }

    //准备标牌价字典
    QMap<QString, qint64> setPriceMap;
    if ( dLines.length() > 0 ) {
        qry.exec(QStringLiteral("select hpcode, setprice from cargo;"));
        while ( qry.next() ) {
            setPriceMap.insert(qry.value(0).toString(), qry.value(1).toLongLong());
        }
        qry.finish();
    }

    //合并同款同色同价行，并求出总数量总金额
    qint64 dqtySum = 0;
    qint64 dmnySum = 0;
    QMap<QString, QString> rowMap;
    foreach (QString line, dLines) {
        QStringList cols = line.split(QChar('\t'));
        if ( cols.length() >= 5 ) {
            dqtySum += QString(cols[3]).toLongLong();
            dmnySum += QString(cols[3]).toLongLong() * QString(cols[4]).toLongLong() / 10000;
            QString rowKey = QStringLiteral("%1\t%2\t%3").arg(cols[0]).arg(cols[1]).arg(cols[4]);
            QString rowValue = QStringLiteral("%1\t%2").arg(cols[2]).arg(cols[3]);
            if ( cols.length() > 5 ) { rowValue += QStringLiteral("\t%1").arg(cols[5]); }  //版本升级加的rowmark
            QString valueOld = rowMap.value(rowKey);
            QString valueNew = (valueOld.isEmpty())
                    ? rowValue
                    : QStringLiteral("%1\n%2").arg(valueOld).arg(rowValue);
            rowMap.insert(rowKey, valueNew);
        }
    }

    //合并行map转list，并排序同款同色
    QList<QString> rowList;
    QMapIterator<QString, QString> mapIt(rowMap);
    while (mapIt.hasNext()) {
        mapIt.next();
        rowList << QStringLiteral("%1\f%2").arg(mapIt.key()).arg(mapIt.value());  //约用\f
    }
    std::sort(rowList.begin(), rowList.end());

    //sqls
    QStringList batches;

    //从表sql
    qint64 mkeyRowTime = QDateTime::currentMSecsSinceEpoch();
    qint64 ddisSum = 0;
    foreach (QString row, rowList) {

        //需先合并同尺码
        QStringList sections = row.split(QChar('\f'));  //前面有约\f
        QStringList rowCargoColorPrices = QString(sections.at(0)).split(QChar('\t'));
        QStringList rowSizeQtyMarks = QString(sections.at(1)).split(QChar('\n'));

        QMap<QString, qint64> sqtys;
        qint64 rowQty = 0;
        QString rowmark;
        foreach (QString rowSizeQtyMark, rowSizeQtyMarks) {
            QStringList secs = rowSizeQtyMark.split(QChar('\t'));
            QString sizer = secs.at(0);
            qint64 sv = QString(secs.at(1)).toLongLong();
            rowQty += sv;
            sqtys.insert(sizer, sqtys.value(sizer) + sv);
            if ( secs.length() > 2 ) { rowmark += secs.at(2); }     //版本升级加的rowmark
        }

        QStringList sizers;
        QMapIterator<QString, qint64> itSqtys(sqtys);
        while (itSqtys.hasNext()) {
            itSqtys.next();
            sizers << QStringLiteral("%1\t%2").arg(itSqtys.key()).arg(itSqtys.value());
        }

        QString rowCargo = rowCargoColorPrices.at(0);
        QString rowColor = rowCargoColorPrices.at(1);
        qint64 rowPrice = QString(rowCargoColorPrices.at(2)).toLongLong();
        qint64 setPrice = setPriceMap.value(rowCargo);
        if ( setPrice == 0 ) setPrice = 999999999999;
        qint64 discount = 10000 * rowPrice / setPrice;
        qint64 actmoney = rowPrice * rowQty / 10000;
        qint64 dismoney = (setPrice - rowPrice) * rowQty / 10000;
        ddisSum += dismoney;

        //sql
        const QString dfields = QStringLiteral("parentid, rowtime, cargo, color, sizers, qty, "
                                               "price, discount, actmoney, dismoney, rowmark");
        QStringList dvalues;
        dvalues << QString::number(sheetId);
        dvalues << QString::number(mkeyRowTime++);
        dvalues << QStringLiteral("'%1'").arg(rowCargo);
        dvalues << QStringLiteral("'%1'").arg(rowColor);
        dvalues << QStringLiteral("'%1'").arg(sizers.join(QChar('\n')));
        dvalues << QString::number(rowQty);
        dvalues << QString::number(rowPrice);
        dvalues << QString::number(discount);
        dvalues << QString::number(actmoney);
        dvalues << QString::number(dismoney);
        dvalues << QStringLiteral("'%1'").arg(rowmark);  //版本升级加的rowmark

        batches << QStringLiteral("insert into %1dtl(%2) values(%3);")
                   .arg(tname).arg(dfields).arg(dvalues.join(QChar(',')));
    }

    //主表sqls
    const QString mfields = QStringLiteral("sheetid, dated, shop, trader, stype, staff, remark, "
                                           "sumqty, summoney, sumdis, actpay, actowe, upman, uptime");
    QStringList mvalues;
    mvalues << QString::number(sheetId);
    mvalues << QString::number(datedValue);
    mvalues << QStringLiteral("'%1'").arg(shopValue);
    mvalues << QStringLiteral("'%1'").arg(traderValue);
    mvalues << QStringLiteral("'%1'").arg(stypeValue);
    mvalues << QStringLiteral("'%1'").arg(staffValue);
    mvalues << QStringLiteral("'%1'").arg(remarkValue);
    mvalues << QString::number(dqtySum);
    mvalues << QString::number(dmnySum);
    mvalues << QString::number(ddisSum);
    mvalues << QString::number(actpayValue);
    mvalues << QString::number(dmnySum - actpayValue);
    mvalues << QStringLiteral("'%1'").arg(user->mName);
    mvalues << QString::number(uptimeValue);

    batches << QStringLiteral("insert into %1(%2) values(%3);")
            .arg(tname).arg(mfields).arg(mvalues.join(QChar(',')));

    //特殊调用
    if ( updSheetId ) {
        batches.prepend(QString::number(uptimeValue));
        batches.prepend(QString::number(actpayValue));
        batches.prepend(QString::number(dmnySum));
        batches.prepend(QString::number(dqtySum));
        batches.prepend(traderValue);
        batches.prepend(shopValue);
        batches.prepend(QStringLiteral("OK"));
        return batches.join(QChar('\f'));
    }

    //执行
    db.transaction();
    for ( int i = 0, iLen = batches.length(); i < iLen; ++i ) {
        QString line = batches.at(i);
        db.exec(line);
        if ( db.lastError().isValid() ) {
            qDebug() << db.lastError().text();
            qDebug() << line;
            db.rollback();
            respList << QStringLiteral("事务失败");
            return respList.join(QChar('\f'));
        }
    }
    db.commit();

    //通知库存变动
    if ( tname == QStringLiteral("cgj") ||
         tname == QStringLiteral("cgt") ||
         tname == QStringLiteral("pff") ||
         tname == QStringLiteral("pft") ||
         tname == QStringLiteral("lsd") ||
         tname == QStringLiteral("dbd") ||
         tname == QStringLiteral("syd") ) {
        emit shopStockChanged(shopValue, tname, sheetId);
    }

    //日志
    serverLog(user->mName, 2, QStringLiteral("%1: %2-%3 %4件 %5~%6元")
              .arg(tname)
              .arg(shopValue)
              .arg(traderValue)
              .arg(dqtySum / 10000)
              .arg(dmnySum / 10000)
              .arg(actpayValue / 10000));

    //返回数据
    respList << QString::number(sheetId);
    respList << QString::number(uptimeValue);

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqFeeInsert(const QString &packstr, const BsFronter *user)
{
/*  【REQUEST】
        2：主表值...（\t）                 约定字段：shop,staff,remark
        3：从表值行...\n...\n...（\n \t）  约定字段：rowtime, rowmark, income

    【RESPONSE】
        2：新单号
        3：记录时间 */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 4 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //前置权限数据加载
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);

    //参数解析预备
    QStringList mRawValues = QString(params.at(2)).split(QChar('\t'));
    QStringList dLines = QString(params.at(3)).split(QChar('\n'));
    if ( mRawValues.length() != 3 ) {
        respList << QStringLiteral("参数格式错误");
        return respList.join(QChar('\f'));
    }
    QString shopValue = mRawValues.at(0);
    QString staffValue = mRawValues.at(1);
    QString remarkValue = mRawValues.at(2);
    qint64 uptimeValue = QDateTime::currentMSecsSinceEpoch() / 1000;
    qint64 datedValue = QDateTime(QDate::currentDate()).toMSecsSinceEpoch() / 1000;

    //权限
    if ( ! user->bindShop.isEmpty() ) {
        if ( shopValue != user->bindShop ) {
            respList << QStringLiteral("没有该项操作权限");
            return respList.join(QChar('\f'));
        }
    }

    //新sheetid
    int sheetId;
    QSqlQuery qry(db);
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec(QStringLiteral("SELECT seq FROM sqlite_sequence WHERE name='szd';"));
    if ( qry.lastError().isValid() ) {
        respList << QStringLiteral("服务器意外故障");
        return respList.join(QChar('\f'));
    }
    if ( qry.next() )
        sheetId = qry.value(0).toInt() + 1;
    else
        sheetId = 1;

    //sqls
    QStringList batches;

    //从表sql
    qint64 totalIncome = 0;
    foreach (QString dline, dLines) {

        QStringList cols = dline.split(QChar('\t'));
        if ( cols.length() == 3 ) {
            qint64 rowtime = QString(cols.at(0)).toLongLong();
            QString rowmark = cols.at(1);
            qint64 income = QString(cols.at(2)).toLongLong();
            totalIncome += income;

            const QString dfields = QStringLiteral("parentid, rowtime, subject, income, rowmark ");
            QStringList dvalues;
            dvalues << QString::number(sheetId);
            dvalues << QString::number(rowtime);
            dvalues << QStringLiteral("'%1'").arg(rowmark);
            dvalues << QString::number(income);
            dvalues << QStringLiteral("'%1'").arg(rowmark);

            batches << QStringLiteral("insert into szddtl(%1) values(%2);")
                       .arg(dfields).arg(dvalues.join(QChar(',')));

        }
    }

    //主表sqls
    const QString mfields = QStringLiteral("sheetid, dated, shop, staff, remark, upman, uptime");
    QStringList mvalues;
    mvalues << QString::number(sheetId);
    mvalues << QString::number(datedValue);
    mvalues << QStringLiteral("'%1'").arg(shopValue);
    mvalues << QStringLiteral("'%1'").arg(staffValue);
    mvalues << QStringLiteral("'%1'").arg(remarkValue);
    mvalues << QStringLiteral("'%1'").arg(user->mName);
    mvalues << QString::number(uptimeValue);

    batches << QStringLiteral("insert into szd(%1) values(%2);")
            .arg(mfields).arg(mvalues.join(QChar(',')));

    //执行
    db.transaction();
    for ( int i = 0, iLen = batches.length(); i < iLen; ++i ) {
        QString line = batches.at(i);
        db.exec(line);
        if ( db.lastError().isValid() ) {
            qDebug() << db.lastError().text();
            qDebug() << line;
            db.rollback();
            respList << QStringLiteral("事务失败");
            return respList.join(QChar('\f'));
        }
    }
    db.commit();

    //日志
    serverLog(user->mName, 3, QStringLiteral("%1-%2 ￥%3")
              .arg(shopValue).arg(staffValue).arg(totalIncome / 10000.0));

    //返回数据
    respList << QString::number(sheetId);
    respList << QString::number(uptimeValue);

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqRegInsert(const QString &packstr, const BsFronter *user)
{
/*  【REQUEST】
        2：资料主表名（customer或supplier）
        3：新添insert、改动update
        4：kvalue
        5：主表字段...（\t）
        6：主表值...（\t）

    【RESPONSE】
        kvalue */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 7 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //前置权限数据加载
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);

    //参数解析预备
    QString tname = QString(params.at(2)).trimmed().toLower();
    bool neww = (QString(params.at(3)).trimmed().toLower() == QStringLiteral("insert"));
    QString kvalue = params.at(4);
    QStringList flds = QString(params.at(5)).split(QChar('\t'));
    QStringList vals = QString(params.at(6)).split(QChar('\t'));
    if ( flds.length() != vals.length() || flds.length() < 1 )  {
        respList << QStringLiteral("参数格式错误");
        return respList.join(QChar('\f'));
    }

    //权限
    QString actName = (neww) ? QStringLiteral("new") : QStringLiteral("upd");
    if ( ! BsFronterMap::actionAllow(user, tname, actName) ) {
        respList << QStringLiteral("没有该项操作权限");
        return respList.join(QChar('\f'));
    }

    //sql
    QString sql;
    if ( neww ) {
        QStringList sqlFlds, sqlVals;

        sqlFlds << QStringLiteral("kname") << flds;
        sqlVals << QStringLiteral("'%1'").arg(kvalue);
        for ( int i = 0, iLen = vals.length(); i < iLen; ++i ) {
            QString fld = flds.at(i);
            if ( fld == QStringLiteral("regdis") ) {
                qint64 v = QString(vals.at(i)).toLongLong();
                if ( v == 0 ) v = 10000;
                sqlVals << QString::number(v);
            } else
                sqlVals << QStringLiteral("'%1'").arg(vals.at(i));
        }
        sqlFlds << QStringLiteral("upman") << QStringLiteral("uptime");
        sqlVals << QStringLiteral("'%1'").arg(user->mName) << QString::number(QDateTime::currentMSecsSinceEpoch() / 1000);

        sql = QStringLiteral("insert into %1(%2) values(%3);")
                .arg(tname)
                .arg(sqlFlds.join(QChar(',')))
                .arg(sqlVals.join(QChar(',')));
    }
    else {
        QStringList sqlExps;
        for ( int i = 0, iLen = vals.length(); i < iLen; ++i ) {
            QString fld = flds.at(i);
            if ( fld == QStringLiteral("regdis") )
                sqlExps << QStringLiteral("%1=%2").arg(fld).arg(QString(vals.at(i)).toLongLong());
            else
                sqlExps << QStringLiteral("%1='%2'").arg(fld).arg(vals.at(i));
        }

        sql = QStringLiteral("update %1 set %2 where kname='%3';")
                .arg(tname)
                .arg(sqlExps.join(QChar(',')))
                .arg(kvalue);
    }

    //执行
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        respList << ((neww) ? QStringLiteral("登记失败") : QStringLiteral("更改失败"));
        return respList.join(QChar('\f'));
    }

    //日志
    QString logRegAction = (neww) ? QStringLiteral("添") : QStringLiteral("改");
    serverLog(user->mName, 4, QStringLiteral("%1: %2").arg(logRegAction).arg(kvalue));

    //回复数据
    respList << kvalue;

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqRegCargo(const QString &packstr, const BsFronter *user)
{
/*  【REQUEST】
        2：kvalue（insert为空、update非空）
        3：主表字段...（\t）
        4：主表值...（\t）

    【RESPONSE】
        kvalue */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 5 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //前置权限数据加载
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);

    //参数解析预备
    QString kvalue = QString(params.at(2)).trimmed();
    QStringList flds = QString(params.at(3)).split(QChar('\t'));
    QStringList vals = QString(params.at(4)).split(QChar('\t'));
    if ( flds.length() != vals.length() || flds.length() < 1 )  {
        respList << QStringLiteral("参数格式错误");
        return respList.join(QChar('\f'));
    }

    //权限
    QString actName = (kvalue.isEmpty()) ? QStringLiteral("new") : QStringLiteral("upd");
    if ( ! BsFronterMap::actionAllow(user, QStringLiteral("cargo"), actName) ) {
        respList << QStringLiteral("没有该项操作权限");
        return respList.join(QChar('\f'));
    }

    //sql
    QString sql;
    if ( kvalue.isEmpty() ) {
        QStringList sqlFlds, sqlVals;

        sqlFlds << flds;
        for ( int i = 0, iLen = vals.length(); i < iLen; ++i ) {
            QString fld = flds.at(i);
            if ( fld.endsWith(QStringLiteral("price")) )
                sqlVals << QString::number(QString(vals.at(i)).toLongLong());
            else
                sqlVals << QStringLiteral("'%1'").arg(vals.at(i));
        }
        sqlFlds << QStringLiteral("upman") << QStringLiteral("uptime");
        sqlVals << QStringLiteral("'%1'").arg(user->mName) << QString::number(QDateTime::currentSecsSinceEpoch());

        sql = QStringLiteral("insert into cargo(%1) values(%2);")
                .arg(sqlFlds.join(QChar(',')))
                .arg(sqlVals.join(QChar(',')));
    }
    else {
        QStringList sqlExps;
        for ( int i = 0, iLen = vals.length(); i < iLen; ++i ) {
            QString fld = flds.at(i);
            if ( fld.endsWith(QStringLiteral("price")) )
                sqlExps << QStringLiteral("%1=%2").arg(fld).arg(QString(vals.at(i)).toLongLong());
            else
                sqlExps << QStringLiteral("%1='%2'").arg(fld).arg(vals.at(i));
        }
        sqlExps << QStringLiteral("upman='%1'").arg(user->mName)
                << QStringLiteral("uptime=%1").arg(QDateTime::currentSecsSinceEpoch());

        sql = QStringLiteral("update cargo set %1 where hpcode='%2';")
                .arg(sqlExps.join(QChar(',')))
                .arg(kvalue);
    }

    //执行
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        respList << ((kvalue.isEmpty()) ? QStringLiteral("登记失败") : QStringLiteral("更改失败"));
        return respList.join(QChar('\f'));
    }

    //日志
    QString logRegAction = (kvalue.isEmpty()) ? QStringLiteral("添") : QStringLiteral("改");
    serverLog(user->mName, 4, QStringLiteral("%1: 货号%2").arg(logRegAction).arg(kvalue));

    //回复数据
    respList << kvalue;

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQrySumm(const QString &packstr, const BsFronter *user) {
/*  【REQUEST】
        2: tname ———— 字符串 cgd、cgj、cgt、pfd、pff、pft、dbd、lsd、syd、cg、pf、xs（不含szd，收支功能移动端只有记录需求）
        3: shop ———— 字符串
        4: trader ———— 字符串
        5: cargo ———— hpcode字符串
        6: color ———— color字符串                      <senseless>
        7: sizer ———— sizer字符串                      <senseless>
        8: dateb ———— 字符串格式“2020-01-01”
        9: datee ———— 字符串格式“2020-01-01”
        10: checkk ———— 0 不管、1 仅审、2 仅未审

    【RESPONSE】
        2：值行表...\n...\n...（\n \t）（仅色码明细有多行） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 11 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    QString shop = QString(params.at(3)).trimmed();
    QString trader = QString(params.at(4)).trimmed();
    QString cargo = QString(params.at(5)).trimmed();
    QString datebText = params.at(8).trimmed();
    QString dateeText = params.at(9).trimmed();
    int checkk = QString(params.at(10)).trimmed().toInt();

    QDateTime dateb = QDateTime(dateOfFormattedText(datebText, '-'));
    QDateTime datee = QDateTime(dateOfFormattedText(dateeText, '-'));

    //按权限取值
    QString viRightKey = QStringLiteral("vi%1").arg(tname);
    QStringList vfields;

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("qty")) )
        vfields << QStringLiteral("sum(qty) as summqty");

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("mny")) )
        vfields << QStringLiteral("sum(actmoney) as summmny");

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("dis")) )
        vfields << QStringLiteral("sum(dismoney) as summdis");

    if ( vfields.isEmpty() ) {
        respList << QStringLiteral("无此查询权限");
        return respList.join(QChar('\f'));
    }

    //限定范围与角度
    QStringList grpSels;
    QStringList limExps;
    limExps << QStringLiteral("dated between %1 and %2")
                 .arg(dateb.toMSecsSinceEpoch() / 1000)
                 .arg(datee.toMSecsSinceEpoch() / 1000);

    if ( shop.isEmpty() )
        grpSels << QStringLiteral("shop");
    else
        limExps << QStringLiteral("shop='%1'").arg(shop);

    if ( trader.isEmpty() )
        grpSels << QStringLiteral("trader");
    else
        limExps << QStringLiteral("trader='%1'").arg(trader);

    if ( cargo.isEmpty() )
        grpSels << QStringLiteral("cargo");
    else
        limExps << QStringLiteral("cargo='%1'").arg(cargo);

    if ( checkk == 1 )
        limExps << QStringLiteral("chktime<>0");

    if ( checkk == 2 )
        limExps << QStringLiteral("chktime=0");

    //sql
    QString sql;
    if ( user->mDeskPass.isEmpty() || grpSels.isEmpty() ) {  //手机端不返回多角度
        sql = QStringLiteral("select %1 from vi_%2 where %3;")
                .arg(vfields.join(QChar(',')))
                .arg(tname)
                .arg(limExps.join(QStringLiteral(" and ")));
    }
    else {
        QStringList sels;
        sels << grpSels;
        sels << vfields;
        sql = QStringLiteral("select %1 from vi_%2 where %3 group by %4;")
                .arg(sels.join(QChar(',')))
                .arg(tname)
                .arg(limExps.join(QStringLiteral(" and ")))
                .arg(grpSels.join(QChar(',')));
    }

    //db execute
    qDebug() << "============= summ sql: " << sql;
    respList << buildSqlData(sql);

    //日志
    serverLog(user->mName, 5, QStringLiteral("%1: %2-%3（%4）%5~%6")
              .arg(tname).arg(shop).arg(trader).arg(cargo).arg(datebText).arg(dateeText));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQryCash(const QString &packstr, const BsFronter *user) {
    /*  【REQUEST】
            2: tname ———— 字符串                               pf、xs、cg
            3: shop ———— 字符串                                <senseless>
            4: trader ———— 字符串
            5: cargo ———— hpcode字符串                         <senseless>
            6: color ———— color字符串                          <senseless>
            7: sizer ———— sizer字符串                          <senseless>
            8: dateb ———— 字符串格式“2020-01-01”               <senseless>
            9: datee ———— 字符串格式“2020-01-01”
            10: checkk ———— 0 不管、1 仅审、2 仅未审

        【RESPONSE】
            2：值行表...\n...\n...（\n \t）（仅色码明细有多行） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 11 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    //QString shop = QString(params.at(3)).trimmed();
    QString trader = QString(params.at(4)).trimmed();
    //QString cargo = QString(params.at(5)).trimmed();
    //QDateTime dateb = QDateTime(dateOfFormattedText(QString(params.at(8)).trimmed(), '-'));
    QDateTime datee = QDateTime(dateOfFormattedText(QString(params.at(9)).trimmed(), '-'));
    int checkk = QString(params.at(10)).trimmed().toInt();

    //按权限取值
    QString viRightKey = QStringLiteral("vi%1cash").arg(tname);
    QStringList vfields;

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("qty")) )
        vfields << QStringLiteral("sum(sumqty) as cashqty");

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("mny")) )
        vfields << QStringLiteral("sum(summoney) as cashmny");

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("dis")) )
        vfields << QStringLiteral("sum(sumdis) as cashdis");

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("pay")) )
        vfields << QStringLiteral("sum(actpay) as cashpay");

    if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("owe")) )
        vfields << QStringLiteral("sum(actowe) as cashowe");

    if ( vfields.isEmpty() ) {
        respList << QStringLiteral("无此查询权限");
        return respList.join(QChar('\f'));
    }

    //限定范围
    QStringList limExps;
    limExps << QStringLiteral("dated <= %1").arg(datee.toMSecsSinceEpoch() / 1000);

    if ( ! trader.isEmpty() )
        limExps << QStringLiteral("trader='%1'").arg(trader);

    if ( checkk == 1 )
        limExps << QStringLiteral("chktime<>0");

    if ( checkk == 2 )
        limExps << QStringLiteral("chktime=0");

    //sql
    QString sql = QStringLiteral("select %1 from vi_%2_cash where %3;")
            .arg(vfields.join(QChar(',')))
            .arg(tname)
            .arg(limExps.join(QStringLiteral(" and ")));

    //db execute
    respList << buildSqlData(sql);

    //日志
    serverLog(user->mName, 6, QStringLiteral("%1: %2").arg(tname).arg(trader));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQryRest(const QString &packstr, const BsFronter *user) {
    /*  【REQUEST】
            2: tname ———— 字符串                           pf、xs
            3: shop ———— 字符串                            <senseless>
            4: trader ———— 字符串
            5: cargo ———— hpcode字符串
            6: color ———— color字符串                      <senseless>指定货号查色码，不定货号列全部货号
            7: sizer ———— sizer字符串                      <senseless>指定货号查色码，不定货号列全部货号
            8: dateb ———— 字符串格式“2020-01-01”            <senseless>
            9: datee ———— 字符串格式“2020-01-01”
            10: checkk ———— 0 不管、1 仅审、2 仅未审

        【RESPONSE】
            2：值行表...\n...\n...（\n \t）（仅色码明细有多行） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 11 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //前置权限数据加载
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    //QString shop = QString(params.at(3)).trimmed();
    QString trader = QString(params.at(4)).trimmed();
    QString cargo = QString(params.at(5)).trimmed();
    //QString color = QString(params.at(6)).trimmed();
    //QString sizer = QString(params.at(7)).trimmed();
    //QDateTime dateb = QDateTime(dateOfFormattedText(QString(params.at(8)).trimmed(), '-'));
    QDateTime datee = QDateTime(dateOfFormattedText(QString(params.at(9)).trimmed(), '-'));
    int checkk = QString(params.at(10)).trimmed().toInt();

    //按权限取值
    QString viRightKey = QStringLiteral("vi%1rest").arg(tname);
    QStringList vfields;
    QString gfields;
    QString having;

    if ( cargo.isEmpty() ) {
        vfields << QStringLiteral("cargo");
        gfields = QStringLiteral("cargo");

        if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("qty")) ) {
            vfields << QStringLiteral("sum(qty) as restqty");
            having = QStringLiteral("having sum(qty)<>0");
        }

        if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("mny")) ) {
            vfields << QStringLiteral("sum(actmoney) as restmny");
        }
    }
    else {
        vfields << QStringLiteral("color");
        gfields = QStringLiteral("color");

        if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("qty")) )
            vfields << QStringLiteral("group_concat(vi_%1_rest.sizers, '') as sizers").arg(tname)
                    << QStringLiteral("sum(vi_%1_rest.qty) as qty").arg(tname);
    }

    if ( vfields.length() < 2 ) {
        respList << QStringLiteral("无此查询权限");
        return respList.join(QChar('\f'));
    }

    //限定范围
    QStringList limExps;
    limExps << QStringLiteral("dated <= %1").arg(datee.toMSecsSinceEpoch() / 1000);

    if ( ! trader.isEmpty() )
        limExps << QStringLiteral("trader='%1'").arg(trader);

    if ( ! cargo.isEmpty() )
        limExps << QStringLiteral("cargo='%1'").arg(cargo);

    if ( checkk == 1 )
        limExps << QStringLiteral("chktime<>0");

    if ( checkk == 2 )
        limExps << QStringLiteral("chktime=0");

    //sql
    QString sql = QStringLiteral("select %1 from vi_%2_rest where %3 group by %4 %5;")
            .arg(vfields.join(QChar(',')))
            .arg(tname)
            .arg(limExps.join(QStringLiteral(" and ")))
            .arg(gfields)
            .arg(having);

    //db execute
    if ( cargo.isEmpty() )
        respList << buildSqlData(sql);
    else
        respList << buildSpecVSum(sql);

    //日志
    serverLog(user->mName, 7, QStringLiteral("%1: %2 （%3）").arg(tname).arg(trader).arg(cargo));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQryStock(const QString &packstr, const BsFronter *user) {
    /*  【REQUEST】
            2: tname ———— 字符串                               <senseless>
            3: shop ———— 字符串
            4: trader ———— 字符串                              <senseless>
            5: cargo ———— hpcode字符串
            6: color ———— color字符串（仅用于reqQryStock）
            7: sizer ———— sizer字符串（仅用于reqQryStock）
            8: dateb ———— 字符串格式“2020-01-01”               <senseless>
            9: datee ———— 字符串格式“2020-01-01”
            10: checkk ———— 0 不管、1 仅审、2 仅未审

        【RESPONSE】
            2：值行表...\n...\n...（\n \t）（仅色码明细有多行） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 11 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //前置权限数据加载
    // QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    //qDebug() << "rightMap:";
    //qDebug() << user->rightMap;

    //参数解析预备
    //QString tname = QString(params.at(2)).toLower().trimmed();
    QString shop = QString(params.at(3)).trimmed();
    //QString trader = QString(params.at(4)).trimmed();
    QString cargo = QString(params.at(5)).trimmed();
    QString color = QString(params.at(6)).trimmed();
    QString sizer = QString(params.at(7)).trimmed();
    //QDateTime dateb = QDateTime(dateOfFormattedText(QString(params.at(8)).trimmed(), '-'));
    QDateTime datee = QDateTime(dateOfFormattedText(QString(params.at(9)).trimmed(), '-'));
    int checkk = QString(params.at(10)).trimmed().toInt();

    //按权限取值
    QString viRightKey = QStringLiteral("vistock");
    if ( ! BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("qty")) ) {
        respList << QStringLiteral("无此查询权限");
        return respList.join(QChar('\f'));
    }

    QStringList vfields;
    QStringList gfields;
    QString having;
    bool sumSpecc;

    //未指定货号
    if ( cargo.isEmpty() ) {
        vfields << QStringLiteral("cargo");
        gfields << QStringLiteral("cargo");
        sumSpecc = false;

        if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("qty")) ) {
            vfields << QStringLiteral("sum(qty) as stockqty");
            having = QStringLiteral("having sum(qty)<>0");
        }

        if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("mny")) )
            vfields << QStringLiteral("sum(actmoney) as stockmny");

        if ( BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("dis")) )
            vfields << QStringLiteral("sum(dismoney) as stockdis");
    }
    //指定货号
    else {

        if ( shop.isEmpty() ) {
            vfields << QStringLiteral("shop");
            gfields << QStringLiteral("shop");
        }

        //注意以下 if else 如此最清晰不错，不可乱合并条件！！！ 尤其前面两块，好像可以合并为(!color.isEmpy)，
        //其实不可！！！因为这样，最后的else的情况就不一样了！

        //色码都指定————返回该色所有码（色在后面limExps中限定）
        if ( !color.isEmpty() && !sizer.isEmpty() ) {
            sumSpecc = true;
            vfields << QStringLiteral("group_concat(vi_stock.sizers, '') as sizers")
                    << QStringLiteral("sum(vi_stock.qty) as qty");
        }
        //只指定了颜色————返回该色所有码（色在后面limExps中限定）
        else if ( !color.isEmpty() ) {
            sumSpecc = true;
            vfields << QStringLiteral("group_concat(vi_stock.sizers, '') as sizers")
                    << QStringLiteral("sum(vi_stock.qty) as qty");
        }
        //只指定了尺码————返回全部色码明细
        else if ( !sizer.isEmpty() ) {
            sumSpecc = true;
            vfields << QStringLiteral("color");
            gfields << QStringLiteral("color");
            vfields << QStringLiteral("group_concat(vi_stock.sizers, '') as sizers")
                    << QStringLiteral("sum(vi_stock.qty) as qty");
        }
        //色码都没指定
        else {

            //所有店————返回各店数量，不区分明细（前面vfields和gfields已经区分shop了）
            if ( shop.isEmpty() ) {
                sumSpecc = false;
                vfields << QStringLiteral("sum(vi_stock.qty) as qty");
                having = QStringLiteral("having sum(vi_stock.qty)<>0");
            }
            //指定店————返回全部色码明细
            else {
                sumSpecc = true;
                vfields << QStringLiteral("color");
                gfields << QStringLiteral("color");
                vfields << QStringLiteral("group_concat(vi_stock.sizers, '') as sizers")
                        << QStringLiteral("sum(vi_stock.qty) as qty");
            }
        }
    }

    //限定范围
    QStringList limExps;
    limExps << QStringLiteral("dated <= %1").arg(datee.toMSecsSinceEpoch() / 1000);

    if ( ! shop.isEmpty() )
        limExps << QStringLiteral("shop='%1'").arg(shop);

    if ( ! cargo.isEmpty() )
        limExps << QStringLiteral("cargo='%1'").arg(cargo);

    if ( ! color.isEmpty() )
        limExps << QStringLiteral("color='%1'").arg(color);

    if ( checkk == 1 )
        limExps << QStringLiteral("chktime<>0");

    if ( checkk == 2 )
        limExps << QStringLiteral("chktime=0");

    //sql
    QString sql = QStringLiteral("select %1 from vi_stock where %2")
              .arg(vfields.join(QChar(',')))
              .arg(limExps.join(QStringLiteral(" and ")));
    if ( !gfields.isEmpty() ) {
        sql += QStringLiteral(" group by %1 %2").arg(gfields.join(QChar(','))).arg(having);
    }
    sql += QChar(';');

    //db execute
    if ( sumSpecc )
        respList << buildSpecVSum(sql, sizer);
    else
        respList << buildSqlData(sql);

    //日志
    serverLog(user->mName, 8, QStringLiteral("%1 （%2）").arg(shop).arg(cargo));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQryView(const QString &packstr, const BsFronter *user) {
    /*  【REQUEST】
            2: tname ———— 字符串                               <senseless>
            3: shop ———— 字符串
            4: trader ———— 字符串                              <senseless>
            5: cargo ———— hpcode字符串
            6: color ———— color字符串                          <senseless>
            7: sizer ———— sizer字符串                          <senseless>
            8: dateb ———— 字符串格式“2020-01-01”
            9: datee ———— 字符串格式“2020-01-01”
            10: checkk ———— 0 不管、1 仅审、2 仅未审

        【RESPONSE】
            2：值行表...\n...\n...（\n \t）（仅色码明细有多行） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 11 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    QString shop = QString(params.at(3)).trimmed();
    QString cargo = QString(params.at(5)).trimmed();
    QDateTime dateb = QDateTime(dateOfFormattedText(QString(params.at(8)).trimmed(), '-'));
    QDateTime datee = QDateTime(dateOfFormattedText(QString(params.at(9)).trimmed(), '-'));
    int checkk = QString(params.at(10)).trimmed().toInt();

    //必须指定货号
    if ( cargo.isEmpty() ) {
        respList << QStringLiteral("未指定货号");
        return respList.join(QChar('\f'));
    }

    //前置权限数据加载
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    if ( !BsFronterMap::actionAllow(user, QStringLiteral("viall"), QStringLiteral("qty")) ) {
        respList << QStringLiteral("无此查询权限");
        return respList.join(QChar('\f'));
    }

    //条件Exp
    QString qcSideCon = QStringLiteral("and dated < %1").arg(dateb.toMSecsSinceEpoch() / 1000);
    QString periodCon = QStringLiteral("and dated between %1 and %2")
            .arg(dateb.toMSecsSinceEpoch() / 1000).arg(datee.toMSecsSinceEpoch() / 1000);
    QString shopCon = (shop.isEmpty()) ? QString() : QStringLiteral("and shop='%1'").arg(shop);
    QString shopConEx = (shop.isEmpty()) ? QString() : QStringLiteral("and trader='%1'").arg(shop);
    QString checkkCon = (checkk) ? QStringLiteral("and chktime<>0") : QString();

    //sql准备
    QSqlQuery qry(db);
    qry.setForwardOnly(true);
    QString sql;
    QList<QPair<QString, qint64> > pairs;
    qint64 qmValue;

    //qc
    QString ds = (shop.isEmpty()) ? QStringLiteral("vi_stock_nodb") : QStringLiteral("vi_stock");
    sql = QStringLiteral("select sum(qty) from %1 where cargo='%2' %3 %4 %5;")
            .arg(ds).arg(cargo).arg(qcSideCon).arg(shopCon).arg(checkkCon);
    qry.exec(sql);
    if ( qry.next() ) {
        qmValue = qry.value(0).toLongLong();
        pairs << qMakePair(QStringLiteral("qc"), qry.value(0).toLongLong());
    } else {
        qmValue = 0;
        pairs << qMakePair(QStringLiteral("qc"), 0);
    }

    //gj
    sql = QStringLiteral("select sum(qty) from vi_cgj where cargo='%1' %2 %3 %4;")
            .arg(cargo).arg(periodCon).arg(shopCon).arg(checkkCon);
    qDebug() << "====== gj sql: " << sql;
    qry.exec(sql);
    if ( qry.next() ) {
        qmValue += qry.value(0).toLongLong();
        pairs << qMakePair(QStringLiteral("gj"), qry.value(0).toLongLong());
    }

    //gt
    sql = QStringLiteral("select sum(qty) from vi_cgt where cargo='%1' %2 %3 %4;")
            .arg(cargo).arg(periodCon).arg(shopCon).arg(checkkCon);
    qry.exec(sql);
    if ( qry.next() ) {
        qmValue -= qry.value(0).toLongLong();
        pairs << qMakePair(QStringLiteral("gt"), qry.value(0).toLongLong());
    }

    //sy
    sql = QStringLiteral("select sum(qty) from vi_syd where cargo='%1' %2 %3 %4;")
            .arg(cargo).arg(periodCon).arg(shopCon).arg(checkkCon);
    qry.exec(sql);
    if ( qry.next() ) {
        qmValue += qry.value(0).toLongLong();
        pairs << qMakePair(QStringLiteral("sy"), qry.value(0).toLongLong());
    }

    //dr
    if ( !shop.isEmpty() ) {
        sql = QStringLiteral("select sum(qty) from vi_dbd where cargo='%1' %2 %3 %4;")
                .arg(cargo).arg(periodCon).arg(shopConEx).arg(checkkCon);
        qry.exec(sql);
        if ( qry.next() ) {
            qmValue += qry.value(0).toLongLong();
            pairs << qMakePair(QStringLiteral("dr"), qry.value(0).toLongLong());
        }
    }

    //dc
    if ( !shop.isEmpty() ) {
        sql = QStringLiteral("select sum(qty) from vi_dbd where cargo='%1' %2 %3 %4;")
                .arg(cargo).arg(periodCon).arg(shopCon).arg(checkkCon);
        qry.exec(sql);
        if ( qry.next() ) {
            qmValue -= qry.value(0).toLongLong();
            pairs << qMakePair(QStringLiteral("dc"), qry.value(0).toLongLong());
        }
    }

    //pf
    sql = QStringLiteral("select sum(qty) from vi_pff where cargo='%1' %2 %3 %4;")
            .arg(cargo).arg(periodCon).arg(shopCon).arg(checkkCon);
    qry.exec(sql);
    if ( qry.next() ) {
        qmValue -= qry.value(0).toLongLong();
        pairs << qMakePair(QStringLiteral("pf"), qry.value(0).toLongLong());
    }

    //pt
    sql = QStringLiteral("select sum(qty) from vi_pft where cargo='%1' %2 %3 %4;")
            .arg(cargo).arg(periodCon).arg(shopCon).arg(checkkCon);
    qry.exec(sql);
    if ( qry.next() ) {
        qmValue += qry.value(0).toLongLong();
        pairs << qMakePair(QStringLiteral("pt"), qry.value(0).toLongLong());
    }

    //ls
    sql = QStringLiteral("select sum(qty) from vi_lsd where cargo='%1' %2 %3 %4;")
            .arg(cargo).arg(periodCon).arg(shopCon).arg(checkkCon);
    qry.exec(sql);
    if ( qry.next() ) {
        qmValue -= qry.value(0).toLongLong();
        pairs << qMakePair(QStringLiteral("ls"), qry.value(0).toLongLong());
    }

    //qm
    pairs << qMakePair(QStringLiteral("qm"), qmValue);

    //日志
    serverLog(user->mName, 9, QStringLiteral("%1 （%2）").arg(shop).arg(cargo));

    //构造（仿数据集，第一行列名，第二行列值，无多行）
    QStringList flds;
    QStringList vals;
    for ( int i = 0, iLen = pairs.length(); i < iLen; ++i ) {
        flds << pairs.at(i).first;
        vals << QString::number(pairs.at(i).second);
    }
    respList << QStringLiteral("%1\n%2").arg(flds.join(QChar('\t'))).arg(vals.join(QChar('\t')));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQryObject(const QString &packstr, const BsFronter *user)
{
    /*===========================================查询（登记对象）======================================
    【REQUEST】
        2：表名            //目前只用于customer与supplier
        3：查找值           //kname或regtele

    【RESPONSE】
        2：值行...\t...\t... */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    qDebug() << params;

    //前置检查
    QStringList respList;
    if ( params.length() != 4 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    QString tname = QString(params.at(2)).toLower();
    QString fvalue = params.at(3);
    if ( tname.isEmpty() || fvalue.isEmpty() ) {
        respList << QStringLiteral("无查询信息");
        return respList.join(QChar('\f'));
    }

    //权限
    if ( ! BsFronterMap::actionAllow(user, tname, QStringLiteral("open")) ) {
        respList << QStringLiteral("没有该项操作权限");
        return respList.join(QChar('\f'));
    }

    //sql
    QString sql = QStringLiteral("select kname, regdis, regman, regtele, regaddr "
                                 "from %1 where kname='%2' or regtele='%2';")
            .arg(tname)
            .arg(fvalue);

    //output
    respList << buildSqlData(sql);

    //日志
    serverLog(user->mName, 10, fvalue);

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQryImage(const QString &packstr, const BsFronter *user)
{
    /*===========================================查询（货品图片）======================================
    【REQUEST】
        2：货号

    【RESPONSE】
        2：货号
        3: 图片数据Base64码 */

    Q_UNUSED(user)

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 3 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);
    QString cargo = params.at(2);

    //读图
    QString imgFile = checkCargoImageFile(cargo);
    if ( imgFile.isEmpty() ) {
        respList << QStringLiteral("该货品暂无图片");
        return respList.join(QChar('\f'));
    }       

    QImage imgSrc(imgFile);
    QImage imgDst = imgSrc.scaled(QSize(300, 300), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    //打水印
    QPainter painter(&imgDst);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    QFont font(qApp->font());
    font.setBold(true);
    QString noticeText = QStringLiteral("图片禁止外传");

    QRect noticeRect = imgDst.rect().adjusted(0, 0, 0, -(imgDst.height() * 9) / 10);
    font.setPixelSize(24);
    painter.setFont(font);
    painter.setPen(QPen(QColor(Qt::black)));
    painter.drawText(noticeRect.adjusted(1, 1, 1, 1), Qt::AlignCenter, noticeText);
    painter.setPen(QPen(QColor(Qt::white)));
    painter.drawText(noticeRect, Qt::AlignCenter, noticeText);

    QRect nameRect = imgDst.rect().adjusted(0, imgDst.height() / 2, 0, 0);
    font.setPixelSize(48);
    painter.setFont(font);
    painter.setPen(QPen(QColor(Qt::black)));
    painter.drawText(nameRect.adjusted(4, 4, 4, 4), Qt::AlignCenter, loginer);
    painter.setPen(QPen(QColor(Qt::white)));
    painter.drawText(nameRect, Qt::AlignCenter, loginer);

    //    QRect destRect = imgDst.rect().adjusted(0, imgDst.height() / 2, 0, 0);
    //    QFontMetrics fm = painter.fontMetrics();
    //    qreal sx = destRect.width() * 1.0 / fm.width(loginer);
    //    qreal sy = destRect.height() * 1.0 / fm.height();
    //    qreal ratio = ( sx > sy ) ? sy : sx;
    //    painter.translate(destRect.center());
    //    painter.scale(ratio / 2.0, ratio / 2.0);
    //    painter.translate(-destRect.center());
    //    painter.setPen(QPen(QColor(255, 255, 255)));
    //    painter.setCompositionMode(QPainter::CompositionMode_SoftLight);
    //    painter.drawText(destRect, Qt::AlignCenter, loginer);

    QByteArray imgData;
    QBuffer buffer(&imgData);
    buffer.open(QIODevice::WriteOnly);
    imgDst.save(&buffer, "jpg");
    buffer.close();

    //日志
    serverLog(user->mName, 11, cargo);

    //回复内容
    respList << QString(cargo);
    respList << QString(imgData.toBase64());

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqQryPrintOwe(const QString &packstr, const BsFronter *user) {
    /*  【REQUEST】
            2: tname ———— 字符串                               pf、xs
            3: trader ———— 字符串
            4: checkk ———— 0不管、非0仅审

        【RESPONSE】
            2：值行...\t...\t... */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 5 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    QString tname = QString(params.at(2)).toLower().trimmed();
    QString trader = QString(params.at(3)).trimmed();
    int checkk = QString(params.at(4)).trimmed().toInt();

    if ( tname != QStringLiteral("pf") && tname != QStringLiteral("xs") && tname != QStringLiteral("cg") ) {
        respList << QStringLiteral("非法表名请求");
        return respList.join(QChar('\f'));
    }

    //按权限取值
    QString viRightKey = QStringLiteral("vi%1cash").arg(tname);
    if ( ! BsFronterMap::actionAllow(user, viRightKey, QStringLiteral("owe")) ) {
        respList << QStringLiteral("无此查询权限");
        return respList.join(QChar('\f'));
    }

    //限定范围
    QStringList limExps;
    if ( trader.isEmpty() ) {
        respList << QStringLiteral("未指定查询对象");
        return respList.join(QChar('\f'));
    }
    else {
        limExps << QStringLiteral("trader='%1'").arg(trader);
    }

    if ( checkk )
        limExps << QStringLiteral("chktime<>0");

    //sql
    QString sql = QStringLiteral("select sum(actowe) as sumowe from vi_%1_cash where %2;")
            .arg(tname)
            .arg(limExps.join(QStringLiteral(" and ")));

    //db execute
    respList << buildSqlData(sql);

    //日志
    serverLog(user->mName, 6, QStringLiteral("%1欠款打印: %2").arg(tname).arg(trader));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//BOTH desk and mobile
QString BsTerminator::reqMessage(const QString &packstr, const BsFronter *user, const QString &toName, qint64 *msgIdPtr)
{
    /*===========================================留言转发确认======================================
    【REQUEST】
        2: 发送人或群ID（冗余是为了方便接收方识别，且转发不需重新压缩加密，直接原包照转）
        3: 发送人或群名称（冗余是为了方便接收方识别，且转发不需重新压缩加密，直接原包照转）
        4：接受人HexID或群NumID
        5：内容

    【RESPONSE】
        2：reqId (即msgId，也即发信人前端微秒时间) */

    //拆解参数
    QStringList params = packstr.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 6 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }

    respList << params.at(0);
    respList << params.at(1);

    //参数解析预备
    qint64 msgId = QString(params.at(1)).toLongLong();  //microSeconds
    QString senderId = params.at(2);
    QString senderName = params.at(3);
    QString toHexId = params.at(4);     //meetid（真meetid或userHashHexId）
    QString content = params.at(5);

    //必要
    if ( toHexId.isEmpty() || content.isEmpty() ) {
        respList << QStringLiteral("非法");
        return respList.join(QChar('\f'));
    }

    //防止伪造发送人
    if ( senderId != user->mFrontId || senderName != user->mName ) {
        respList << QStringLiteral("非法");
        return respList.join(QChar('\f'));
    }

    //记录
    QSqlQuery query(QSqlDatabase::database(mDatabaseConnectionName));
    query.prepare(QStringLiteral("insert into msglog(msgid, senderid, sendername, receiverid, receivername, content) "
                                 "values(:msgid, :senderid, :sendername, :receiverid, :receivername, :content);"));
    query.bindValue(":msgid", msgId);
    query.bindValue(":senderid", senderId);
    query.bindValue(":sendername", user->mName);
    query.bindValue(":receiverid", toHexId);
    query.bindValue(":receivername", toName);
    query.bindValue(":content", content);
    query.exec();
    if ( query.lastError().isValid() ) {
        qDebug() << "msglog: " << query.lastError().text();
        *msgIdPtr = 0;
    } else {
        *msgIdPtr = msgId;
    }

    //output
    respList << QString::number(msgId);

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}


//only mobile client
QString BsTerminator::reqGrpCreate(const QString &packstr) {
    /*===========================================建群======================================
    【REQUEST】
        2：群id   因为转发成员使用的是请求原包，所以让请求端产生
        3：群名
        4：成员    TAB分隔

    【RESPONSE】
        2：群id
        3：检验过的群名
        4：成员（只为转发）    TAB分隔   */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 5 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }

    respList << params.at(0);
    respList << params.at(1);

    //参数
    qint64 meetId = QString(params.at(2)).trimmed().toLongLong();
    QString meetName = QString(params.at(3)).trimmed().left(30);
    QString members = params.at(4);

    //计算成员ID
    QStringList memberIds;
    QStringList newNames = members.split(QChar('\t'));
    foreach (QString newName, newNames) {
        memberIds << BsFronter::calcShortMd5(newName);
    }

    //sql
    QString sql = QStringLiteral("insert into meeting(meetid, meetname, members) values(%1, '%2', '%3');")
            .arg(meetId).arg(meetName).arg(members);

    //数据库执行
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        qDebug() << db.lastError().text() << sql;
        respList << QStringLiteral("建群失败");
        return respList.join(QChar('\f'));
    }

    //成功
    BsMeetingMap::loadUpdate(mDatabaseConnectionName);
    respList << QString::number(meetId);
    respList << meetName;
    respList << members;    

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}

//only mobile client
QString BsTerminator::reqGrpRename(const QString &packstr) {
    /*===========================================改群名======================================
    【REQUEST】
        2：群id
        3：新群名

    【RESPONSE】
        2：群id
        3：新群名（如太长，有裁剪） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 4 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数
    qint64 meetId = QString(params.at(2)).toLongLong();
    QString newName = QString(params.at(3)).trimmed().left(30);

    //sql
    QString sql = QStringLiteral("update meeting set meetname='%1' where meetid=%2;")
            .arg(newName).arg(meetId);

    //数据库执行
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        qDebug() << db.lastError().text() << sql;
        respList << QStringLiteral("改群名失败");
        return respList.join(QChar('\f'));
    }

    //成功
    BsMeetingMap::loadUpdate(mDatabaseConnectionName);
    respList << QString::number(meetId);
    respList << newName;

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}

//only mobile client
QString BsTerminator::reqGrpDismiss(const QString &packstr) {
    /*===========================================解散群======================================
    【REQUEST】
        2：群Id

    【RESPONSE】
        2：群Id */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 3 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数
    qint64 meetId = QString(params.at(2)).toLongLong();

    //sql
    QString sql = QStringLiteral("delete from meeting where meetid=%1;").arg(meetId);

    //数据库执行
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        qDebug() << db.lastError().text() << sql;
        respList << QStringLiteral("解散群失败");
        return respList.join(QChar('\f'));
    }

    //成功
    BsMeetingMap::loadUpdate(mDatabaseConnectionName);
    respList << QString::number(meetId);

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}

//only mobile client
QString BsTerminator::reqGrpInvite(const QString &packstr) {
    /*===========================================邀人======================================
    【REQUEST】
        2：群ID
        3: 群名（纯粹为了转发而设）
        4：原有成员表（名称TAB分隔）  //后端无用，但被邀请前端需要（原包照转）
        5：新增成员表（名称TAB分隔）

    【RESPONSE】
        2: 群ID
        3: 群名（转发新成员需要）
        4: 新成员名TAB分隔列表（不含总经理） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 6 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数
    qint64 meetId = QString(params.at(2)).toLongLong();
    QString meetName = params.at(3);
    QStringList newUserIds = QString(params.at(5)).split(QChar('\t'));

    //查原成员
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    QString sql = QStringLiteral("select members from meeting where meetid=%1;").arg(meetId);
    QSqlQuery qry(db);
    qry.exec(sql);
    QStringList members;
    if ( qry.next() )
        members = qry.value(0).toString().split(QChar('\t'));
    else {
        respList << QStringLiteral("群名错误");
        return respList.join(QChar('\f'));
    }

    //加群
    foreach (QString newUserId, newUserIds) {
        if ( members.indexOf(newUserId) < 0 ) members << newUserId;
    }

    //sql
    sql = QStringLiteral("update meeting set members='%1' where meetid=%2;")
            .arg(members.join(QChar('\t'))).arg(meetId);

    //数据库执行
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        qDebug() << db.lastError().text() << sql;
        respList << QStringLiteral("邀人失败");
        return respList.join(QChar('\f'));
    }

    //计算成员ID
    QStringList memberIds;
    foreach (QString member, members) {
        memberIds << BsFronter::calcShortMd5(member);
    }

    //成功
    BsMeetingMap::loadUpdate(mDatabaseConnectionName);
    respList << QString::number(meetId);
    respList << meetName;
    respList << members.join(QChar('\t'));

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}

//only mobile client
QString BsTerminator::reqGrpKickoff(const QString &packstr) {
    /*===========================================踢人======================================
    【REQUEST】
        2：群ID
        3：剔除成员表（名称TAB分隔）

    【RESPONSE】
        2：群ID
        3: 踢人后成员名TAB分隔列表（不含总经理） */

    //移除危险字符，并拆解参数
    QString spack = packstr;
    spack.replace(QChar(39), QString());  //禁止单引号
    QStringList params = spack.split(QChar('\f'));

    //前置检查
    QStringList respList;
    if ( params.length() != 4 ) {
        respList << QStringLiteral("参数数量错误");
        return respList.join(QChar('\f'));
    }
    respList << params.at(0);
    respList << params.at(1);

    //参数
    qint64 meetId = QString(params.at(2)).toLongLong();
    QStringList kickUserIds = QString(params.at(3)).split(QChar('\t'));

    //查原成员
    QSqlDatabase db = QSqlDatabase::database(mDatabaseConnectionName);
    QString sql = QStringLiteral("select members from meeting where meetid=%1;").arg(meetId);
    QSqlQuery qry(db);
    qry.exec(sql);
    QStringList members;
    if ( qry.next() )
        members = qry.value(0).toString().split(QChar('\t'));
    else {
        respList << QStringLiteral("群名错误");
        return respList.join(QChar('\f'));
    }

    //移除
    foreach (QString kickUserId, kickUserIds) {
        if ( members.indexOf(kickUserId) >= 0 ) members.removeAll(kickUserId);
    }

    //sql
    if ( members.length() > 1 )
        sql = QStringLiteral("update meeting set members='%1' where meetid=%2;")
                .arg(members.join(QChar('\t'))).arg(meetId);
    else
        sql = QStringLiteral("delete from meeting where meetid=%1;").arg(meetId);

    //数据库执行
    db.exec(sql);
    if ( db.lastError().isValid() ) {
        qDebug() << db.lastError().text() << sql;
        respList << QStringLiteral("踢人失败");
        return respList.join(QChar('\f'));
    }

    //成功
    BsMeetingMap::loadUpdate(mDatabaseConnectionName);
    if ( members.length() > 1 ) {
        //计算成员ID
        QStringList memberIds;
        foreach (QString member, members) {
            memberIds << BsFronter::calcShortMd5(member);
        }

        //还有多人
        respList << QString::number(meetId);
        respList << members.join(QChar('\t'));
    }
    else {
        //相当于解散群
        respList << QString::number(meetId);
        respList << QString();
    }

    //return
    respList << QStringLiteral("OK");
    return respList.join(QChar('\f'));
}

}

