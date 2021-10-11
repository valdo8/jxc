#include "lxsalesmanage.h"
#include "main/bailicode.h"
#include "main/bailisql.h"
#include "main/bailicustom.h"
#include "main/bailifunc.h"
#include "main/bailiwins.h"

#ifdef Q_OS_WIN
#include "Dongle_API.h"     //省略路径，则pro文件中必须有INCLUDEPATH+=...
#endif

namespace BailiSoft {

//初始用户号。以后逐渐增1，比如第一家的用户号为2003319（广州wx蓝天）
const int user_birth_id = 2003318;

LxSalesManage::LxSalesManage(QWidget *parent, const QString &lockAdminPassword)
    : QWidget(parent), mLockAdminPassword(lockAdminPassword)
{
    setProperty(BSWIN_TABLE, "lxsales");

    //客户表自用特殊字段检查
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery qry(db);
    QStringList checkFields = getExistsFieldsOfTable(QStringLiteral("customer"), db);
    QStringList sqls;
    if ( checkFields.indexOf(QStringLiteral("duserid")) < 0 ) {
        //用户锁值ID（整数值，从2003318基值开始递增）
        sqls << QStringLiteral("alter table customer add column duserid integer default 0;");
    }
    if ( checkFields.indexOf(QStringLiteral("dusername")) < 0 ) {
        //网络后台名与后台验证码
        sqls << QStringLiteral("alter table customer add column dusername text default '';");
        sqls << QStringLiteral("alter table customer add column duserpass text default '';");
    }
    QString strErr;
    db.transaction();
    foreach (QString sql, sqls) {
        db.exec(sql);
        if ( db.lastError().isValid() ) {
            strErr = db.lastError().text();
            db.rollback();
            break;
        }
    }
    if ( strErr.isEmpty() ) db.commit(); else qDebug() << strErr;

    //Form
    QFont font = this->font();
    font.setPointSize(font.pointSize() + 3);

    QLabel *lblUserStart = new QLabel(QStringLiteral("选择客户："));
    lblUserStart->setFixedWidth(300);
    lblUserStart->setFont(font);

    mpUsers = new QComboBox(this);
    mpUsers->setFixedWidth(300);
    mpUsers->setFont(font);
    mpUsers->addItem(QStringLiteral("★★★新客户★★★"), 0);
    QString sql = QStringLiteral("select kname, duserid from customer order by kname;");
    qry.setForwardOnly(true);
    qry.exec(sql);
    mMaxUserId = user_birth_id;
    while ( qry.next() ) {
        uint userid = qry.value(1).toUInt();
        mpUsers->addItem(qry.value(0).toString(), userid);
        if ( userid > mMaxUserId )
            mMaxUserId = userid;
    }
    qry.finish();
    connect(mpUsers, SIGNAL(currentIndexChanged(int)), this, SLOT(userPicked(int)));

    mpUserName = new QLineEdit(this);
    mpUserName->setMaxLength(20);

    mpBuyQty = new QLineEdit(this);
    mpBuyQty->setText("1");

    mpBuyMan = new QLineEdit(this);
    mpBuyMan->setMaxLength(20);

    mpBuyTele = new QLineEdit(this);
    mpBuyTele->setMaxLength(50);

    mpBuyAddr = new QLineEdit(this);
    mpBuyAddr->setMaxLength(100);
    mpBuyAddr->setMinimumWidth(600);

    mpNetName = new QLineEdit(this);
    mpNetName->setMaxLength(10);
    mpNetName->setPlaceholderText(QStringLiteral("纯单机授权不填"));
    mpNetName->setAttribute(Qt::WA_InputMethodEnabled, false);

    QFormLayout *layForm = new QFormLayout;
    layForm->addRow(QStringLiteral("客户名称："), mpUserName);
    layForm->addRow(QStringLiteral("购买数量："), mpBuyQty);
    layForm->addRow(QStringLiteral("收 件 人："), mpBuyMan);
    layForm->addRow(QStringLiteral("联系电话："), mpBuyTele);
    layForm->addRow(QStringLiteral("收件地址："), mpBuyAddr);
    layForm->addRow(QStringLiteral("后台登记名："), mpNetName);

    mpBtnMakeHard = new QPushButton(QStringLiteral("烧录硬锁"));
    mpBtnMakeHard->setFixedWidth(300);
    mpBtnMakeHard->setFont(font);
    connect(mpBtnMakeHard, SIGNAL(clicked(bool)), this, SLOT(makeHard()));
    mpBtnMakeHard->setEnabled(false);

    mpBtnMakeSoft = new QPushButton(QStringLiteral("创建软锁"));
    mpBtnMakeSoft->setFixedWidth(300);
    mpBtnMakeSoft->setFont(font);
    connect(mpBtnMakeSoft, SIGNAL(clicked(bool)), this, SLOT(makeSoft()));
    mpBtnMakeSoft->setEnabled(false);

    QWidget *pnlButtons = new QWidget(this);
    QHBoxLayout *layButtons = new QHBoxLayout(pnlButtons);
    layButtons->addStretch();
    layButtons->addWidget(mpBtnMakeHard);
    layButtons->addWidget(mpBtnMakeSoft);
    layButtons->addStretch();

    mpReport = new QTextEdit(this);
    mpReport->setReadOnly(true);
    mpReport->setMinimumHeight(120);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(lblUserStart, 0, Qt::AlignCenter);
    lay->addWidget(mpUsers, 0, Qt::AlignCenter);
    lay->addSpacing(20);
    lay->addLayout(layForm);
    lay->addSpacing(20);
    lay->addWidget(pnlButtons, 0, Qt::AlignCenter);
    lay->addSpacing(20);
    lay->addWidget(mpReport, 1);
    setWindowTitle(QStringLiteral("敬业版授权管理"));

    connect(mpUserName, SIGNAL(editingFinished()), this, SLOT(checkEnableButton()));
    connect(mpBuyQty, SIGNAL(editingFinished()), this, SLOT(checkEnableButton()));
    connect(mpBuyMan, SIGNAL(editingFinished()), this, SLOT(checkEnableButton()));
    connect(mpBuyTele, SIGNAL(editingFinished()), this, SLOT(checkEnableButton()));
    connect(mpBuyAddr, SIGNAL(editingFinished()), this, SLOT(checkEnableButton()));
    connect(mpNetName, SIGNAL(editingFinished()), this, SLOT(checkEnableButton()));
}

void LxSalesManage::userPicked(int index)
{
    if ( index > 0 ) {
        QSqlQuery qry;
        qry.setForwardOnly(true);
        qry.exec(QStringLiteral("select regman, regtele, regaddr from customer where kname='%1';")
                 .arg(mpUsers->currentText()));
        if ( qry.next() ) {
            mpUserName->setText(mpUsers->currentText());
            mpUserName->setEnabled(false);
            mpBuyMan->setText(qry.value(0).toString());
            mpBuyTele->setText(qry.value(1).toString());
            mpBuyAddr->setText(qry.value(2).toString());
        }
        qry.finish();
    }
    else {
        mpUserName->setEnabled(true);
        mpUserName->clear();
        mpBuyMan->clear();
        mpBuyTele->clear();
        mpBuyAddr->clear();
    }

    //只有单机授权才会增加锁，因此选客户必然是纯单机授权
    mpNetName->clear();

    //改动禁用
    checkEnableButton();
}

void LxSalesManage::checkEnableButton()
{
    mpBtnMakeHard->setEnabled(checkInputValid());
    mpBtnMakeSoft->setEnabled(checkInputValid() && !mpNetName->text().trimmed().isEmpty());
}

void LxSalesManage::makeHard()
{
    if ( ! checkInputValid() )
        return;

    mpBtnMakeHard->setEnabled(false);

#ifdef Q_OS_WIN

    //锁操作返回值
    DWORD dwRet = 0;

    //枚举锁数量
    int nCount = 0;
    int motherDogIdx = -1;
    int userDogIdx = -1;
    QByteArray userHardId;
    dwRet = Dongle_Enum(NULL, &nCount);
    if ( nCount < 2 ) {
        mpReport->setText(QStringLiteral("请插入一只母狗，一只空狗，一共两只狗！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //枚举锁信息
    if ( DONGLE_SUCCESS == dwRet ) {

        //获取信息
        DONGLE_INFO *pDongleInfo = (DONGLE_INFO *)malloc(nCount * sizeof(DONGLE_INFO));
        dwRet = Dongle_Enum(pDongleInfo, &nCount);

        if ( DONGLE_SUCCESS == dwRet ) {

            //取得母锁索引
            for ( int i = 0; i < nCount; ++i ) {
                if ( pDongleInfo[i].m_IsMother ) {
                    motherDogIdx = i;
                    break;
                }
            }

            //取得用户锁索引
            for ( int i = 0; i < nCount; ++i ) {
                if ( ! pDongleInfo[i].m_IsMother ) {
                    userDogIdx = i;
                    for (int j = 0; j < 8; j++) {
                        userHardId += (char)(pDongleInfo[i].m_HID[j]);  //8Bytes硬件ID
                    }
                    break;
                }
            }
        }

        //清除信息缓存
        if (NULL != pDongleInfo) {
            free(pDongleInfo);
            pDongleInfo = NULL;
        }
    }

    //母锁提示
    if ( motherDogIdx < 0 ) {
        mpReport->setText(QStringLiteral("两只都不是母狗，请拔掉一只子狗，插入一只母狗！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //子锁提示
    if ( userDogIdx < 0 ) {
        mpReport->setText(QStringLiteral("必须插入有一只子狗！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //母锁句柄
    DONGLE_HANDLE mMotherDogHandle;
    dwRet = Dongle_Open(&mMotherDogHandle, motherDogIdx);
    if ( DONGLE_SUCCESS != dwRet ) {
        mpReport->setText(QStringLiteral("打开母狗失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //子锁句柄
    DONGLE_HANDLE mUserDogHandle;
    dwRet = Dongle_Open(&mUserDogHandle, userDogIdx);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        mpReport->setText(QStringLiteral("打开子狗失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //数据缓存
    BYTE      buff[1024];
    BYTE      request[16];
    memset(request, 0, sizeof(request));
    memset(buff, 0, sizeof(buff));

    //空锁请求初始化
    dwRet = Dongle_RequestInit(mUserDogHandle, request);
    if( DONGLE_SUCCESS != dwRet) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        qDebug() << "Dongle_RequestInit: " << dwRet;
        mpReport->setText(QStringLiteral("取空锁请求初始化数据不成功！"));
        mpBtnMakeHard->setEnabled(true);
        return ;
    }

    //从母锁获取生产数据
    int len = sizeof(buff);
    dwRet = Dongle_GetInitDataFromMother(mMotherDogHandle, request, buff, &len);
    if( DONGLE_SUCCESS != dwRet) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        qDebug() << "Dongle_GetInitDataFromMother: " << dwRet;
        mpReport->setText(QStringLiteral("母锁产生初始化数据不成功！"));
        mpBtnMakeHard->setEnabled(true);
        return ;
    }

    //生产
    dwRet = Dongle_InitSon(mUserDogHandle, buff, len);
    if( DONGLE_SUCCESS != dwRet) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        qDebug() << "Dongle_InitSon: " << dwRet;
        mpReport->setText(QStringLiteral("生产子锁不成功！"));
        mpBtnMakeHard->setEnabled(true);
        return ;
    }

    //验证身份
    char adminPassPin[17];
    memcpy(adminPassPin, mLockAdminPassword.toUtf8().constData(), 16);
    adminPassPin[16] = 0;
    int nRemainTries = 0;
    dwRet = Dongle_VerifyPIN(mUserDogHandle, FLAG_ADMINPIN, adminPassPin, &nRemainTries);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        qDebug() << "Dongle_VerifyPIN: " << dwRet << "  remainTries: " << nRemainTries;
        mpReport->setText(QStringLiteral("授权管理密码验证失败，请重新换新狗或将此狗恢复出厂设置！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //烧录用户号
    uint userId = ( mpUsers->currentIndex() > 0 ) ? mpUsers->currentData().toUInt() : mMaxUserId + 1;
    dwRet = Dongle_SetUserID(mUserDogHandle, (DWORD)userId);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        qDebug() << "Dongle_SetUserID: " << dwRet;
        mpReport->setText(QStringLiteral("设置用户ID失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }


    //创建三数据文件。1号文件存客户公司名、2号存购买信息、3后台授权登记信息。
    QString strName = mpUserName->text();
    qint64 buyTime = QDateTime(QDate::currentDate()).toMSecsSinceEpoch();
    QString netPass = (strName.contains(QStringLiteral("百利")) && strName.contains(QStringLiteral("测试")))
            ? QStringLiteral("12345678")
            : generateReadableRandomString(8);
    QString strBuy = QStringLiteral("%1\t%2\t%3\t%4\t%5")
            .arg(buyTime).arg(mpBuyQty->text())
            .arg(mpBuyMan->text()).arg(mpBuyTele->text()).arg(mpBuyAddr->text());
    QString strNet = (mpNetName->text().isEmpty())
            ? QString()
            : QStringLiteral("%1\t%2").arg(mpNetName->text()).arg(netPass);

    QByteArray baData1 = strName.toUtf8();
    QByteArray baData2 = strBuy.toUtf8();
    QByteArray baData3 = strNet.toUtf8();

    while ( baData1.length() < 128 )
        baData1 += '\0';

    while ( baData2.length() < 1024 )
        baData2 += '\0';

    while ( baData3.length() < 64 )
        baData3 += '\0';

    DATA_LIC dataLic;
    dataLic.m_Read_Priv = 1;
    dataLic.m_Write_Priv = 2;
    DATA_FILE_ATTR dataFileAttr;
    dataFileAttr.m_Size = 128;
    dataFileAttr.m_Lic = dataLic;
    dwRet = Dongle_CreateFile(mUserDogHandle, FILE_DATA, 1, &dataFileAttr);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("创建文件1（客户公司名）失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }
    dataFileAttr.m_Size = 1024;
    dwRet = Dongle_CreateFile(mUserDogHandle, FILE_DATA, 2, &dataFileAttr);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("创建文件2（详细购买信息）失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }
    dataFileAttr.m_Size = 64;
    dwRet = Dongle_CreateFile(mUserDogHandle, FILE_DATA, 3, &dataFileAttr);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("创建文件3（网络后台登记信息）失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    dwRet = Dongle_WriteFile(mUserDogHandle, FILE_DATA, 1, 0, (BYTE*)baData1.constData(), 128);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("写入文件1（客户公司名）失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    dwRet = Dongle_WriteFile(mUserDogHandle, FILE_DATA, 2, 0, (BYTE*)baData2.constData(), 1024);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("写入文件2（详细购买信息）失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    dwRet = Dongle_WriteFile(mUserDogHandle, FILE_DATA, 3, 0, (BYTE*)baData3.constData(), 64);
    if ( DONGLE_SUCCESS != dwRet ) {
        qDebug() << "dwRet when write file 3: " << dwRet;
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("写入文件3（网络后台登记信息）失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //清空全部数据区
    char initData[8192] = {0};
    dwRet = Dongle_WriteData(mUserDogHandle, 0, (BYTE*)initData, 8192);
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("清空8K数据区失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //初始数据（由于不需要防破解，因此数据区不设计无意义）
    QByteArray baHeader = generateRandomBytes(2048);
    dwRet = Dongle_WriteData(mUserDogHandle, 0, (BYTE*)baHeader.constData(), baHeader.length());
    if ( DONGLE_SUCCESS != dwRet ) {
        Dongle_Close(mMotherDogHandle);
        Dongle_Close(mUserDogHandle);
        mpReport->setText(QStringLiteral("初始化原始2K数据失败！"));
        mpBtnMakeHard->setEnabled(true);
        return;
    }

    //释放锁句柄
    Dongle_Close(mMotherDogHandle);
    Dongle_Close(mUserDogHandle);

    //重复烧录多锁，必须禁止乱动————不同客户，需要关闭窗口重新打开。
    mpUsers->setEnabled(false);
    mpUserName->setEnabled(false);
    mpBuyQty->setEnabled(false);
    mpBuyMan->setEnabled(false);
    mpBuyTele->setEnabled(false);
    mpBuyAddr->setEnabled(false);
    mpNetName->setEnabled(false);

    //记录硬件ID
    mMadeHardIds.insert(QString(userHardId.toHex()));

    //数据库保存
    if ( mMadeHardIds.count() == mpBuyQty->text().toInt() ) {
        QString sql = QStringLiteral("select max(sheetid) from lsd;");
        QSqlQuery qry;
        qry.exec(sql);
        if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << "\n" << sql;
        int newSheetId = (qry.next()) ? (qry.value(0).toInt() + 1) : 1;

        QString trader = mpUserName->text().replace(QChar(39), QString());

        QStringList hards;
        QSetIterator<QString> sit(mMadeHardIds);
        while ( sit.hasNext() )
            hards << sit.next();

        qint64 qty = mpBuyQty->text().toInt() * 10000;
        qint64 mny = qty * 980;

        QString remark = QStringLiteral("硬件ID：%1").arg(hards.join(QChar(44)));
        if ( ! mpNetName->text().trimmed().isEmpty() ) {
            remark += QStringLiteral("。 网络后台账号：%1， 网络后台验证码：%2")
                    .arg(mpNetName->text()).arg(netPass);
        }

        QStringList sqls;
        if ( mpUsers->currentIndex() == 0 )
            sqls << QStringLiteral("insert into customer(kname, duserid, regman, regtele, regaddr, dusername, duserpass) "
                                   "values('%1', %2, '%3', '%4', '%5', '%6', '%7');").arg(trader).arg(userId)
                    .arg(mpBuyMan->text()).arg(mpBuyTele->text()).arg(mpBuyAddr->text())
                    .arg(mpNetName->text()).arg(netPass);
        sqls << QStringLiteral("insert into lsd(sheetid, dated, shop, trader, stype, remark, sumqty, summoney, actpay) "
                               "values(%1, %2, '公司', '%3', '网络硬锁', '硬件ID：%4', %5, %6, %6);")
                .arg(newSheetId).arg(buyTime/1000).arg(trader).arg(remark).arg(qty).arg(mny);
        sqls << QStringLiteral("insert into lsddtl(parentid, rowtime, cargo, color, qty, actmoney) "
                               "values(%1, %2, '敬业网络硬锁', '无色', %3, %4);")
                .arg(newSheetId).arg(QDateTime::currentMSecsSinceEpoch() / 1000).arg(qty).arg(mny);
        sqliteCommit(sqls);

        mpBtnMakeHard->setEnabled(false);
    }
    else {
        mpBtnMakeHard->setEnabled(true);
    }

    //报告
    if ( mpNetName->text().isEmpty() ) {
        QString report = QStringLiteral("该客户第%1只狗烧录成功！").arg(mMadeHardIds.count());
        if ( mMadeHardIds.count() < mpBuyQty->text().toInt() )
            report += QStringLiteral("\n请拔下烧录好的新狗，换插其他空狗继续烧录……");
        else
            report += QStringLiteral("\n%1只狗全部烧录完成，可以交付客户了。").arg(mpBuyQty->text());
        mpReport->setPlainText(report);
    }
    else {
        mpReport->setPlainText(QStringLiteral("烧制成功，网络后台验证码为：%1").arg(netPass));
    }
#endif
}

void LxSalesManage::makeSoft()
{
    if ( ! checkInputValid() )
        return;

    mpBtnMakeSoft->setEnabled(false);

#ifdef Q_OS_WIN

    //锁操作返回值
    DWORD dwRet = 0;

    //枚举锁数量
    int nCount = 0;
    int motherDogIdx = -1;
    dwRet = Dongle_Enum(NULL, &nCount);
    if ( nCount < 1 ) {
        mpReport->setText(QStringLiteral("请插入母狗！"));
        mpBtnMakeSoft->setEnabled(true);
        return;
    }

    //枚举锁信息
    if ( DONGLE_SUCCESS == dwRet ) {

        //获取信息
        DONGLE_INFO *pDongleInfo = (DONGLE_INFO *)malloc(nCount * sizeof(DONGLE_INFO));
        dwRet = Dongle_Enum(pDongleInfo, &nCount);

        if ( DONGLE_SUCCESS == dwRet ) {

            //取得母锁索引
            for ( int i = 0; i < nCount; ++i ) {
                if ( pDongleInfo[i].m_IsMother ) {
                    motherDogIdx = i;
                    break;
                }
            }
        }

        //清除信息缓存
        if (NULL != pDongleInfo) {
            free(pDongleInfo);
            pDongleInfo = NULL;
        }
    }

    //母锁提示
    if ( motherDogIdx < 0 ) {
        mpReport->setText(QStringLiteral("所插狗不是母狗！"));
        mpBtnMakeSoft->setEnabled(true);
        return;
    }

    //母锁句柄
    DONGLE_HANDLE mMotherDogHandle;
    dwRet = Dongle_Open(&mMotherDogHandle, motherDogIdx);
    if ( DONGLE_SUCCESS != dwRet ) {
        mpReport->setText(QStringLiteral("打开母狗失败！"));
        mpBtnMakeSoft->setEnabled(true);
        return;
    }

    //释放锁句柄
    Dongle_Close(mMotherDogHandle);

    //界面
    mpUsers->setEnabled(false);
    mpUserName->setEnabled(false);
    mpBuyQty->setEnabled(false);
    mpBuyMan->setEnabled(false);
    mpBuyTele->setEnabled(false);
    mpBuyAddr->setEnabled(false);
    mpNetName->setEnabled(false);

    //用户号
    uint userId = ( mpUsers->currentIndex() > 0 ) ? mpUsers->currentData().toUInt() : mMaxUserId + 1;

    //创建数据文件信息。
    QString strName = mpUserName->text();
    QString netPass = (strName.contains(QStringLiteral("百利")) && strName.contains(QStringLiteral("测试")))
            ? QStringLiteral("12345678")
            : generateReadableRandomString(8);

    //约定 checkLicenseSoftKey ：userId, userName, netName, netPass, buyTime, buyMan, buyTele, buyAddr
    QStringList keys;
    keys << QString::number(userId)
         << mpUserName->text()
         << mpNetName->text()
         << netPass
         << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"))
         << mpBuyMan->text()
         << mpBuyTele->text()
         << mpBuyAddr->text();
    QString keyText = keys.join(QChar('\t'));

    //加密
    QString saveText = QString::fromLatin1(keyText.toUtf8().toBase64());

    //保存
    QFile f(QStringLiteral("D:/%1.key").arg(mpNetName->text()));
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream strm(&f);
        strm << saveText;
        f.close();
    }

    //数据库保存
    QString sql = QStringLiteral("select max(sheetid) from lsd;");
    QSqlQuery qry;
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << "\n" << sql;
    int newSheetId = (qry.next()) ? (qry.value(0).toInt() + 1) : 1;

    QString trader = mpUserName->text().replace(QChar(39), QString());

    qint64 qty = mpBuyQty->text().toInt() * 10000;
    qint64 mny = qty * 980;

    qint64 buyTime = QDateTime(QDate::currentDate()).toMSecsSinceEpoch();
    QString remark = QStringLiteral("网络软锁。后台账号为：%1，验证码为：%2")
            .arg(mpNetName->text()).arg(netPass);

    QStringList sqls;
    if ( mpUsers->currentIndex() == 0 )
        sqls << QStringLiteral("insert into customer(kname, duserid, regman, regtele, regaddr, dusername, duserpass) "
                               "values('%1', %2, '%3', '%4', '%5', '%6', '%7');").arg(trader).arg(userId)
                .arg(mpBuyMan->text()).arg(mpBuyTele->text()).arg(mpBuyAddr->text())
                .arg(mpNetName->text()).arg(netPass);
    sqls << QStringLiteral("insert into lsd(sheetid, dated, shop, trader, remark, sumqty, summoney, actpay) "
                           "values(%1, %2, '公司', '%3', '%4', %5, %6, %6);")
            .arg(newSheetId).arg(buyTime/1000).arg(trader).arg(remark).arg(qty).arg(mny);
    sqls << QStringLiteral("insert into lsddtl(parentid, rowtime, cargo, color, qty, actmoney) "
                           "values(%1, %2, '敬业软锁', 'mini', %3, %4);")
            .arg(newSheetId).arg(QDateTime::currentMSecsSinceEpoch() / 1000).arg(qty).arg(mny);
    sqliteCommit(sqls);

    mpBtnMakeSoft->setEnabled(false);

    //报告
    mpReport->setPlainText(QStringLiteral("软锁制作成功，网络后台账号为：%1，验证码为：%2，软锁保存为：D:/%1.key")
                           .arg(mpNetName->text()).arg(netPass));
#endif
}

bool LxSalesManage::checkInputValid()
{
    if ( mpNetName->text().isEmpty() ) {
        mpBuyQty->setEnabled(true);
    }
    else {
        mpBuyQty->setText(QStringLiteral("1"));
        mpBuyQty->setEnabled(false);
    }

    if ( mpUserName->text().length() < 3 ) {
        mpReport->setPlainText(QStringLiteral("客户名称太短！"));
        return false;
    }

    if ( mpUsers->currentIndex() == 0 && mpUsers->findText(mpUserName->text()) > 0 ) {
        mpReport->setPlainText(QStringLiteral("该客户已有登记，必须点选后操作！"));
        return false;
    }

    uint qty = mpBuyQty->text().toUInt();
    if ( mpBuyQty->text() != QString::number(qty) ) {
        mpReport->setPlainText(QStringLiteral("数量填写不合法！"));
        return false;
    }

    mpReport->clear();

    return true;
}

}
