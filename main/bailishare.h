#ifndef BSDBPOOL_H
#define BSDBPOOL_H

#include <QtSql>
#include <QQueue>
#include <QString>
#include <QMutex>
#include <QMutexLocker>

namespace BailiSoft {


// 后台信息单例 =======================================================================
class BsBackerInfo
{
public:
    static void loadUpdate(const QString &backerName, const QString &backerVcode);

    static QString &getBossId();
    static QString &getBossName();
    static QString &getBackerName();
    static QString &getBackerId();
    static QString &getNetCode();
    static bool usingCryption();
    static uint8_t* getCryptionKey();

private:
    static BsBackerInfo& getInstance();

    QString    mBossId;
    QString    mBossName;
    QString    dogNetName;
    QString    mBackerId;
    QString    mNetCode;
    bool       mUsingCryption;
    uint8_t    mCryptionKey[16];

    static QMutex                    mutex;
    static BsBackerInfo *            instance;
};


// 前端定义，及其查找单例类 ============================================================================
class BsFronter
{
public:
    QString         mName;
    QString         mFrontId;
    QString         mDeskPass;
    QString         mNetCode;
    bool            mOnlinee = false;
    bool                    mBosss = false;
    bool                    canRett = false;
    bool                    canLott = false;
    bool                    canBuyy = false;
    QString                 bindShop;
    QString                 bindTrader;         //绑定客户
    QString                 limCargoExp;
    QMap<QString, uint>     rightMap;
    int                     versionDate = 0;    //前端登录后根据登录请求参数标识协议版本，用于兼容区别升级。

    static QByteArray calcShortMd5(const QString &text) {
        QByteArray hashBytes = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5).mid(4, 8);
        return hashBytes.toHex();
    }
};
//单例类
class BsFronterMap
{
public:
    static void loadUpdate();
    static BsFronter* frontOfId(const QString &userId);
    static QByteArray getAllIdsBytes();
    static bool actionAllow(const BsFronter *fronter, const QString &obj, const QString &act);

private:
    static BsFronterMap& getInstance();
    uint rightFlagOf(const QString &actionKeyString);

    QMap<QString, BsFronter*>     map;       //key:frontHashHexId(mFrontId)

    static QMutex                    mutex;
    static BsFronterMap *            instance;
};


// 群组类，及其查找单例类 ============================================================================
class BsMeeting
{
public:
    qint64      mMeetId;
    QString     mMeetName;
    QStringList mMembers;
    QStringList mMemberIds;
};
//单例类
class BsMeetingMap
{
public:
    static void loadUpdate(const QString &dbConnName = QString());
    static BsMeeting* meetingOfId(const qint64 meetId);
    static QStringList memberIdsOfMeet(const qint64 meetId);

private:
    static BsMeetingMap& getInstance();

    QMap<qint64, BsMeeting*>     map;       //key:meetId

    static QMutex                    mutex;
    static BsMeetingMap *            instance;
};


}

#endif // BSDBPOOL_H

