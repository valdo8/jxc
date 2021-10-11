#include "bailicustom.h"
#include "bailicode.h"
#include "bailifunc.h"


namespace BailiSoft {

//公共版本号
QString                 versionLicenseName  = QString();

//授权狗信息
bool                    dogOk           = false;    //飞天狗OK
bool                    dogFoundMother  = false;    //开发母狗
quint32                 dogUserId       = 0;
quint32                 dogProductId    = 0;        //实测值
QByteArray              dogHardId       = QByteArray(8, '\0');
QString                 dogUserName     = QString();
qint64                  dogBuyTime      = 0;
QString                 dogNetName      = QString();
QString                 dogNetPass      = QString();
QString                 httpUserName    = QString();
QString                 httpPassHash    = QString();

#ifdef Q_OS_WIN
DONGLE_HANDLE           hDongle         = nullptr;
static const char *dogUserPin  = "j$aEj=6uT$qd9TM3";  //R17永久固定值，已分发狗已经使用
#endif

//检查读锁（硬锁）
void checkLicenseDog()
{
#ifdef Q_OS_WIN
    //名称先缺省为试用版，后面有从狗中读取用户公司名称。
    dogUserName = mapMsg.value("app_dog_user_try");

    //===================================以下为飞天新狗操作====================================//
    /*
    狗文件1：用户公司或个人名（程序运行使用）
    狗文件2：购买日期、收件人、收件电话、收件地址（维护鉴定使用）
    狗文件3：网络账号、密码（网络后台登记————最早纯单机版无）
    */

    DWORD dwRet = 0;

    //枚举锁数量
    int nCount = 0;
    int userKeyIdx = -1;
    dwRet = Dongle_Enum(nullptr, &nCount);
    dogOk = ( DONGLE_SUCCESS == dwRet );

    //枚举锁信息
    if ( dogOk ) {

        //获取信息
        DONGLE_INFO *pDongleInfo = static_cast<DONGLE_INFO *>(malloc(size_t(nCount) * sizeof(DONGLE_INFO)));
        dwRet = Dongle_Enum(pDongleInfo, &nCount);
        dogOk = ( DONGLE_SUCCESS == dwRet );

        if ( dogOk ) {

            //取得母锁索引
            for ( int i = 0; i < nCount; ++i ) {
                if ( pDongleInfo[i].m_IsMother ) {
                    dogFoundMother = true;
                    break;
                }
            }

            //取得用户锁索引
            for ( int i = 0; i < nCount; ++i ) {
                if ( ! pDongleInfo[i].m_IsMother ) {
                    userKeyIdx = i;
                    dogProductId = pDongleInfo[i].m_PID;
                    break;
                }
            }

            //用户变量
            dogUserId = pDongleInfo[userKeyIdx].m_UserID;
            for (int i = 0; i < 8; i++) {
                dogHardId[i] = char(pDongleInfo[userKeyIdx].m_HID[i]);
            }
        }

        //清除信息缓存
        if (nullptr != pDongleInfo) {
            free(pDongleInfo);
            pDongleInfo = nullptr;
        }
    }

    //打开用户锁，取得锁句柄供全局使用
    if ( dogOk ) {
        dwRet = Dongle_Open(&hDongle, userKeyIdx);
        dogOk = ( DONGLE_SUCCESS == dwRet );
    }

    //验证用户PIN码
    if ( dogOk ) {
        char passPin[17] = {0};  //必须是0终止的字符串（见Dongle_VerifyPIN说明）
        int nRemainTries = 0;
        memcpy(passPin, dogUserPin, 17);
        dwRet = Dongle_VerifyPIN(hDongle, FLAG_USERPIN, passPin, &nRemainTries);
        dogOk = ( DONGLE_SUCCESS == dwRet );
    }

    //读锁数据
    if ( dogOk ) {

        //文件1（客户名）
        char buffComName[128] = {0};
        dwRet = Dongle_ReadFile(hDongle, 1, 0, reinterpret_cast<BYTE *>(buffComName), 128);
        if ( DONGLE_SUCCESS == dwRet )
            dogUserName = QString(buffComName);

        //文件2（购买信息）
        char buffBuyInfo[1024] = {0};
        dwRet = Dongle_ReadFile(hDongle, 2, 0, reinterpret_cast<BYTE *>(buffBuyInfo), 1024);
        if ( DONGLE_SUCCESS == dwRet ) {
            QStringList buyInfo = QString(buffBuyInfo).split(QChar(9));
            dogBuyTime = QString(buyInfo.at(0)).toLongLong();
//            qDebug() << "dogOk: " << dogOk
//                     << "  dogUserId: " << dogUserId
//                     << "  dogHardId: " << dogHardId.toHex()
//                     << "  foundMother: " << dogFoundMother
//                     << "  dogUserName: " << dogUserName
//                     << "  dogBuyTime: " << QDateTime::fromMSecsSinceEpoch(dogBuyTime).toString("yyyy-MM-dd hh:mm:ss")
//                     << "  buyInfo: " << buyInfo;
        }

        //文件3（网络后台登记————纯单机版无）
        char buffNetInfo[64] = {0};
        dwRet = Dongle_ReadFile(hDongle, 3, 0, reinterpret_cast<BYTE *>(buffNetInfo), 64);
        if ( DONGLE_SUCCESS == dwRet ) {
            QStringList ls = QString(buffNetInfo).split(QChar('\t'));
            if ( ls.length() == 2 ) {
                dogNetName = ls.at(0);
                dogNetPass = ls.at(1);
                httpUserName = dogNetName;
                QString httpPassSecret = httpUserName + dogNetPass + QStringLiteral("aimeiwujia.com");
                httpPassHash = QCryptographicHash::hash(httpPassSecret.toLatin1(), QCryptographicHash::Sha256).toHex();
                //qDebug() << httpUserName << httpPassHash;
            }
//            qDebug() << "dogNetName: " << dogNetName
//                     << "  dogNetPass: " << dogNetPass;
        }
    }

    //生成service.key文件，内容约定：第一行dogUserName, 第二行dogNetName, 第三行dogNetPass
    QDir dir( qApp->applicationDirPath() );
    QFile file(dir.absoluteFilePath(QStringLiteral("service.key")));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream strm(&file);
        strm << QStringLiteral("%1\n%2\n%3").arg(dogUserName).arg(dogNetName).arg(dogNetPass);
        file.close();
    }
#endif
}

//检查读锁（软锁）
void checkLicenseSoftKey()
{
    QDir dir( qApp->applicationDirPath() );
    QStringList fileNames = dir.entryList((QStringList() << "*.key"), QDir::Files|QDir::Readable);
    if ( fileNames.length() != 1 )
        return;

    QFile file( dir.absoluteFilePath(fileNames.at(0)) );
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    QString bytes = in.readAll();
    file.close();

    QString text = QString::fromUtf8(QByteArray::fromBase64(bytes.toLatin1()));
    QStringList flds = text.split(QChar('\n'));
    if ( flds.length() < 3 )
        return;

    //service.key文件，内容约定：第一行dogUserName, 第二行dogNetName, 第三行dogNetPass
    dogOk = true;
    dogUserName = flds.at(0);
    dogNetName = flds.at(1);
    dogNetPass = flds.at(2);
    httpUserName = dogNetName;
    QString httpPassSecret = httpUserName + dogNetPass + QStringLiteral("aimeiwujia.com");
    httpPassHash = QCryptographicHash::hash(httpPassSecret.toLatin1(), QCryptographicHash::Sha256).toHex();
}

/***********************************华丽的定制分界线*******************************************/

//特殊定制授权登记，根据定制业务，按用户购买添加。
QSet<QString> _setCustomLicense;      //键名为组合字符串，组合格式：CustomOption-DogUserID
void initCustomLicenses()
{
    _setCustomLicense.insert("staffClassBySheet-2003319");
    _setCustomLicense.insert("shorcutColHideShow-2003319");
}

//用于所有非公共功能代码处，调用判断。
bool getCustomLicenseOf(const QString &customOptionName)
{
    return _setCustomLicense.contains(QStringLiteral("%1-%2").arg(customOptionName).arg(dogUserId));
}


}


