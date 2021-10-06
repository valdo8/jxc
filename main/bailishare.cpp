#include "bailishare.h"
#include "bailicode.h"
#include "bailidata.h"
#include <QDebug>

namespace BailiSoft {

// 后台信息单例 =======================================================================
QMutex BsBackerInfo::mutex;
BsBackerInfo* BsBackerInfo::instance = nullptr;

void BsBackerInfo::loadUpdate(const QString &backerName, const QString &backerVcode)
{
    BsBackerInfo& info = BsBackerInfo::getInstance();
    QMutexLocker locker(&mutex);

    info.mBossId = BsFronter::calcShortMd5(bossAccount);
    info.mBossName = bossAccount;
    info.dogNetName = backerName;
    info.mNetCode = backerVcode;
    info.mBackerId = BsFronter::calcShortMd5(backerName);

    QString safekey = mapOption.value(QStringLiteral("app_encryption_key")).trimmed();
    info.mUsingCryption = !safekey.isEmpty();
    if ( info.mUsingCryption ) {
        QByteArray md5 = QCryptographicHash::hash(safekey.toLatin1(), QCryptographicHash::Md5);
        memcpy(info.mCryptionKey, md5.constData(), 16);
    }
}

QString &BsBackerInfo::getBossId()
{
    return BsBackerInfo::getInstance().mBossId;
}

QString &BsBackerInfo::getBossName()
{
    return BsBackerInfo::getInstance().mBossName;
}

QString &BsBackerInfo::getBackerName()
{
    return BsBackerInfo::getInstance().dogNetName;
}

QString &BsBackerInfo::getBackerId()
{
    return BsBackerInfo::getInstance().mBackerId;
}

QString &BsBackerInfo::getNetCode()
{
    return BsBackerInfo::getInstance().mNetCode;
}

bool BsBackerInfo::usingCryption()
{
    return BsBackerInfo::getInstance().mUsingCryption;
}

uint8_t *BsBackerInfo::getCryptionKey()
{
    return BsBackerInfo::getInstance().mCryptionKey;
}

BsBackerInfo &BsBackerInfo::getInstance()
{
    if (nullptr == instance) {
        QMutexLocker locker(&mutex);
        if (nullptr == instance) {
            instance = new BsBackerInfo();
        }
    }
    return *instance;
}


// 前端查找单例类 ============================================================================
QMutex BsFronterMap::mutex;
BsFronterMap* BsFronterMap::instance = nullptr;

void BsFronterMap::loadUpdate()
{
    BsFronterMap& inst = BsFronterMap::getInstance();
    QMutexLocker locker(&mutex);

    //sql
    QStringList baseFields;
    baseFields << QStringLiteral("loginer")
               << QStringLiteral("deskPassword")
               << QStringLiteral("passhash")
               << QStringLiteral("bindshop")
               << QStringLiteral("bindcustomer")
               << QStringLiteral("bindsupplier")
               << QStringLiteral("retprice")
               << QStringLiteral("lotprice")
               << QStringLiteral("buyprice")
               << QStringLiteral("limcargoexp");
    QString sql = QStringLiteral("select %1, %2, %3, %4 from baililoginer "
                                 "where loginer<>'%5';")
            .arg(baseFields.join(QChar(44)))
            .arg(lstRegisWinTableNames.join(QChar(44)))
            .arg(lstSheetWinTableNames.join(QChar(44)))
            .arg(lstQueryWinTableNames.join(QChar(44)))
            .arg(mapMsg.value(QStringLiteral("word_admin")));

    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) {
        qDebug() << qry.lastError().text() << "\n" << sql;
        return;
    }

    inst.map.clear();

    //全部用户（包括总经理）
    while ( qry.next() ) {

        BsFronter *pUser = new BsFronter();

        QString loginer = qry.value(0).toString();
        bool userAsBoss = (loginer == bossAccount);

        pUser->mBosss = userAsBoss;
        pUser->mOnlinee = false;

        pUser->mName = loginer;
        pUser->mFrontId = BsFronter::calcShortMd5(loginer);
        pUser->mDeskPass = qry.value(1).toString();
        pUser->mNetCode = qry.value(2).toString();

        QString bindCustomer = qry.value(4).toString().trimmed();
        QString bindSupplier = qry.value(5).toString().trimmed();
        QString bindTrader = (bindCustomer.isEmpty()) ? bindSupplier : bindCustomer;
        pUser->bindShop = (userAsBoss) ? QString() : qry.value(3).toString().trimmed();
        pUser->bindTrader = (userAsBoss) ? QString() : bindTrader;
        pUser->canRett = (userAsBoss) ? true : qry.value(6).toBool();
        pUser->canLott = (userAsBoss) ? true : qry.value(7).toBool();
        pUser->canBuyy = (userAsBoss) ? true : qry.value(8).toBool();
        pUser->limCargoExp = (userAsBoss) ? QString() : qry.value(9).toString();

        int idxBase = baseFields.length();
        for ( int i = idxBase; i < idxBase + lstRegisWinTableNames.length(); ++i ) {
            uint rvalue = (userAsBoss) ? 0xffffffff : qry.value(i).toUInt();
            pUser->rightMap.insert(lstRegisWinTableNames.at(i - idxBase), rvalue);
        }

        idxBase += lstRegisWinTableNames.length();
        for ( int i = idxBase; i < idxBase + lstSheetWinTableNames.length(); ++i ) {
            uint rvalue = (userAsBoss) ? 0xffffffff : qry.value(i).toUInt();
            pUser->rightMap.insert(lstSheetWinTableNames.at(i - idxBase), rvalue);
        }

        idxBase += lstSheetWinTableNames.length();
        for ( int i = idxBase; i < idxBase + lstQueryWinTableNames.length(); ++i ) {
            uint rvalue = (userAsBoss) ? 0xffffffff : qry.value(i).toUInt();
            pUser->rightMap.insert(lstQueryWinTableNames.at(i - idxBase), rvalue);
        }

        inst.map.insert(pUser->mFrontId, pUser);

        //qDebug() << loginer << pUser->mFrontId;
    }

    qry.finish();
}

BsFronter* BsFronterMap::frontOfId(const QString &userId) {
    BsFronterMap& inst = BsFronterMap::getInstance();
    QMutexLocker locker(&mutex);
    return inst.map.contains(userId) ? inst.map.value(userId) : nullptr;
}

QByteArray BsFronterMap::getAllIdsBytes()
{
    BsFronterMap& inst = BsFronterMap::getInstance();
    QMutexLocker locker(&mutex);

    QStringList ls;
    QMapIterator<QString, BsFronter*> it(inst.map);
    while ( it.hasNext() ) {
        it.next();
        BsFronter* front = it.value();
        ls << QStringLiteral("%1\t%2").arg(front->mFrontId).arg(front->mNetCode);
    }

    return QString(ls.join(QChar('\n'))).toLatin1();
}

//obj为bailicode.cpp中各lsXxxxWinTableNames变量
//act为rightFlagOf函数中各小写权限动作名，另参考bailiwins.h中enum bsRightXxxx {...}
bool BsFronterMap::actionAllow(const BsFronter *fronter, const QString &obj, const QString &act)
{
    BsFronterMap& inst = BsFronterMap::getInstance();
    QMutexLocker locker(&mutex);

    //其实是异常！
    if (obj.indexOf(QChar('_')) >= 0)
        return false;
    if (!fronter->rightMap.contains(obj))
        return false;

    //老板
    if ( fronter->mBosss )
        return true;

    //权限bits组合值
    uint rightFlag = inst.rightFlagOf(act);
    if ( rightFlag == 0 )
        return false;

    //bits求值
    uint v = fronter->rightMap.value(obj);
    return (v & rightFlag) != 0;
}

BsFronterMap& BsFronterMap::getInstance()
{
    if (nullptr == instance) {
        QMutexLocker locker(&mutex);
        if (nullptr == instance) {
            instance = new BsFronterMap();
        }
    }
    return *instance;
}

uint BsFronterMap::rightFlagOf(const QString &actionKeyString)
{
    //权限名约定见 :
    //bailiwins.h 中 enum bsRightXxxx {...}
    //bailicode.cpp 中各 lsXxxxWinTableNames 变量
    QString str = actionKeyString.toLower();
    if ( str.contains(QStringLiteral("open")) )
        return 1;
    if ( str.contains(QStringLiteral("new")) )
        return 2;
    if ( str.contains(QStringLiteral("upd")) )
        return 4;
    if ( str.contains(QStringLiteral("del")) )
        return 8;
    if ( str.contains(QStringLiteral("check")) )
        return 16;
    if ( str.contains(QStringLiteral("qty")) )
        return 1;
    if ( str.contains(QStringLiteral("mny")) )
        return 2;
    if ( str.contains(QStringLiteral("dis")) )
        return 4;
    if ( str.contains(QStringLiteral("pay")) )
        return 8;
    if ( str.contains(QStringLiteral("owe")) )
        return 16;
    return 0;
}


// 群组查找单例类 ============================================================================
QMutex BsMeetingMap::mutex;
BsMeetingMap* BsMeetingMap::instance = nullptr;

void BsMeetingMap::loadUpdate(const QString &dbConnName)
{
    BsMeetingMap& inst = BsMeetingMap::getInstance();
    QMutexLocker locker(&mutex);

    //sql
    QString sql = QStringLiteral("select meetid, meetname, members from meeting");

    QSqlDatabase db = QSqlDatabase::database(dbConnName);
    QSqlQuery qry(db);
    qry.setForwardOnly(true);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) {
        qDebug() << qry.lastError().text() << "\n" << dbConnName << "  sql: " << sql;
        return;
    }

    //map map
    inst.map.clear();
    while ( qry.next() ) {
        BsMeeting *pMeeting = new BsMeeting();
        pMeeting->mMeetId = qry.value(0).toLongLong();
        pMeeting->mMeetName = qry.value(1).toString();

        QStringList members = qry.value(2).toString().split(QChar('\t'));
        members << BsBackerInfo::getBossName();  //总经理已加入

        QStringList memberIds;
        foreach (QString member, members) {
            memberIds << QString(BsFronter::calcShortMd5(member));
        }

        pMeeting->mMembers = members;
        pMeeting->mMemberIds = memberIds;

        inst.map.insert(pMeeting->mMeetId, pMeeting);
    }

    qry.finish();
}

BsMeeting *BsMeetingMap::meetingOfId(const qint64 meetId)
{
    BsMeetingMap& inst = BsMeetingMap::getInstance();
    QMutexLocker locker(&mutex);
    return (inst.map.contains(meetId)) ? inst.map.value(meetId) : nullptr;
}

QStringList BsMeetingMap::memberIdsOfMeet(const qint64 meetId)
{
    BsMeetingMap& inst = BsMeetingMap::getInstance();
    QMutexLocker locker(&mutex);
    return ( inst.map.contains(meetId) ) ? inst.map.value(meetId)->mMemberIds : QStringList();
}

BsMeetingMap& BsMeetingMap::getInstance()
{
    if (nullptr == instance) {
        QMutexLocker locker(&mutex);
        if (nullptr == instance) {
            instance = new BsMeetingMap();
        }
    }
    return *instance;
}


}
