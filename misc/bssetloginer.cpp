#include "bssetloginer.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailifunc.h"
#include "main/bailishare.h"
#include "dialog/bsloginerbaseinfodlg.h"

#define UDATA_DIRTY     0
#define UDATA_DESKPASS  1
#define UDATA_NETCODE   2
#define UDATA_SHOP      3
#define UDATA_CUSTOMER  4


//静态变量
namespace BailiSoft {

//权限名
extern QStringList              lstRegisRightFlagCNames;
extern QStringList              lstSheetRightFlagCNames;
extern QStringList              lstQueryRightFlagCNames;

QStringList              lstRegisRightFlagCNames;
QStringList              lstSheetRightFlagCNames;
QStringList              lstQueryRightFlagCNames;


//以下序号设计必须与bswins单元中bsRightXXX三枚举定义完全一致，并且因为数据库存储，序号不能再变了。
void initRightFlagCNames()
{
    lstRegisRightFlagCNames.clear();
    lstRegisRightFlagCNames << QStringLiteral("查看")
                            << QStringLiteral("新增")
                            << QStringLiteral("更改")
                            << QStringLiteral("删除")
                            << QStringLiteral("导出");

    lstSheetRightFlagCNames.clear();
    lstSheetRightFlagCNames << QStringLiteral("查看")
                            << QStringLiteral("新增")
                            << QStringLiteral("更改")
                            << QStringLiteral("删除")
                            << QStringLiteral("审核")
                            << QStringLiteral("打印")
                            << QStringLiteral("导出");

    lstQueryRightFlagCNames.clear();
    lstQueryRightFlagCNames << QStringLiteral("数量")
                            << QStringLiteral("金额")
                            << QStringLiteral("折扣")
                            << QStringLiteral("收付")
                            << QStringLiteral("欠款")
                            << QStringLiteral("打印")
                            << QStringLiteral("导出");
}

//二底幂函数
uint powtwo(const int n)
{
    uint rst = 1;
   for ( int i = 0; i < n; ++i ) {
       rst *= 2;
   }
   return rst;
}

}


// BsSetLoginer
namespace BailiSoft {

BsSetLoginer::BsSetLoginer(QWidget *parent) : QDialog(parent)
{
    //初始化静态变量
    initRightFlagCNames();

    //数据模型
    mpModelRegis = new BsRightModel(bsrmtRegis, this);
    mpModelSheet = new BsRightModel(bsrmtSheet, this);
    mpModelQuery = new BsRightModel(bsrmtQuery, this);

    //左功能
    mpBtnAddLoginer = new QPushButton(QStringLiteral("添加用户"), this);
    mpBtnAddLoginer->setFixedWidth(100);

    //左列表
    mpList = new BsUserList(this);
    mpList->setFixedWidth(200);
    mpList->setIconSize(QSize(32, 32));

    //左布局
    mpLeft = new QWidget(this);
    QVBoxLayout *layLeft = new QVBoxLayout(mpLeft);
    layLeft->addWidget(mpBtnAddLoginer, 0, Qt::AlignCenter);
    layLeft->addWidget(mpList, 1);

    //右头
    mpCurrentLoginer = new QLabel(this);
    mpCurrentLoginer->setAlignment(Qt::AlignCenter);
    mpCurrentLoginer->setText(QStringLiteral("请先用鼠标左键点击选择一个用户、或添加一个新用户…"));
    mpCurrentLoginer->setStyleSheet(QLatin1String("color:#999;"));

    //右页————角色
    QFont ft(font());
    ft.setPointSize(3 * ft.pointSize() / 2);
    mpLoginRole = new QLabel(this);
    mpLoginRole->setFont(ft);

    mpRoleInfo = new QLabel(this);     //绑定角色说明
    mpRoleInfo->setWordWrap(true);
    mpRoleInfo->setMinimumWidth(500);

    mpRetPrice = new QCheckBox(QStringLiteral("开放零售价"), this);
    mpLotPrice = new QCheckBox(QStringLiteral("开放批发价"), this);
    mpBuyPrice = new QCheckBox(QStringLiteral("开放进货价"), this);

    QWidget *pnlPrice = new QWidget(this);
    QHBoxLayout *layPrice = new QHBoxLayout(pnlPrice);
    layPrice->setSpacing(60);
    layPrice->addStretch();
    layPrice->addWidget(mpRetPrice);
    layPrice->addWidget(mpLotPrice);
    layPrice->addWidget(mpBuyPrice);
    layPrice->addStretch();

    mpLblCargoExp = new QLabel(QStringLiteral("限制货号范围表达式"), this);
    mpLblCargoExp->hide();
    mpLimCargoExp = new QLineEdit(this);
    mpLimCargoExp->setPlaceholderText(QStringLiteral("百分号通配多字符，下划线通配单字符；不填则无限制。"));
    mpLimCargoExp->setMinimumWidth(380);
    mpLimCargoExp->hide();

    mpRole = new QWidget(this);
    QVBoxLayout *layRole = new QVBoxLayout(mpRole);
    layRole->addStretch(1);
    layRole->addWidget(mpLoginRole, 0, Qt::AlignCenter);
    layRole->addWidget(mpRoleInfo, 1, Qt::AlignCenter);
    layRole->addStretch(1);
    layRole->addWidget(pnlPrice, 0, Qt::AlignCenter);
    layRole->addSpacing(30);
    layRole->addWidget(mpLblCargoExp, 0, Qt::AlignCenter);
    layRole->addWidget(mpLimCargoExp, 0, Qt::AlignCenter);
    layRole->addStretch(3);

    //右页————登记权限
    mpViewRegis = new BsRightView(this);
    mpViewRegis->setModel(mpModelRegis);
    mpViewRegis->setItemDelegateForColumn(0, new BsRightDelegate(mpViewRegis, lstRegisRightFlagCNames));

    //右页————单据权限
    mpViewSheet = new BsRightView(this);
    mpViewSheet->setModel(mpModelSheet);
    mpViewSheet->setItemDelegateForColumn(0, new BsRightDelegate(mpViewSheet, lstSheetRightFlagCNames));

    //右页————查询权限
    mpViewQuery = new BsRightView(this);
    mpViewQuery->setModel(mpModelQuery);
    mpViewQuery->setItemDelegateForColumn(0, new BsRightDelegate(mpViewQuery, lstQueryRightFlagCNames));
    for ( int i = 0, iLen = lstQueryWinTableNames.length(); i < iLen; ++i ) {
        QString wintname = lstQueryWinTableNames.at(i);
        if ( wintname == QStringLiteral("viall") )
            mpViewQuery->setItemDelegateForRow(i, new BsRightDelegate(mpViewQuery,
                                                                      lstQueryRightFlagCNames, true, true, true));
        else if ( wintname.endsWith(QStringLiteral("rest")) )
            mpViewQuery->setItemDelegateForRow(i, new BsRightDelegate(mpViewQuery,
                                                                      lstQueryRightFlagCNames, false, true, true));
        else if ( !wintname.endsWith(QStringLiteral("cash")) )
            mpViewQuery->setItemDelegateForRow(i, new BsRightDelegate(mpViewQuery,
                                                                      lstQueryRightFlagCNames, false, false, true));
    }

    //右页控件
    mpTab = new QTabWidget(this);
    mpTab->setMinimumSize(660, 500);
    mpTab->addTab(mpRole, QStringLiteral("角色前提"));
    mpTab->addTab(mpViewRegis, QStringLiteral("登记权限"));
    mpTab->addTab(mpViewSheet, QStringLiteral("单据权限"));
    mpTab->addTab(mpViewQuery, QStringLiteral("查询权限"));

    //右尾
    mpBtnSaveChange = new QPushButton(mapMsg.value("btn_apply"), this);
    mpBtnCancelChange = new QPushButton(mapMsg.value("btn_cancel"), this);

    mpSaveBox = new QWidget(this);
    QHBoxLayout *laySave = new QHBoxLayout(mpSaveBox);
    laySave->addStretch();
    laySave->addWidget(mpBtnSaveChange);
    laySave->addWidget(mpBtnCancelChange);
    laySave->addStretch();

    //右布局
    mpRight = new QWidget(this);
    QVBoxLayout *layRight = new QVBoxLayout(mpRight);
    layRight->addWidget(mpCurrentLoginer);
    layRight->addWidget(mpTab, 1);
    layRight->addWidget(mpSaveBox);

    //总布局
    QHBoxLayout *layWin = new QHBoxLayout(this);
    layWin->addWidget(mpLeft);
    layWin->addWidget(mpRight, 1);
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);

    //加载
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(QStringLiteral("select loginer, deskPassword, passHash, bindShop, bindCustomer "
                            "from baililoginer "
                            "where loginer<>'%1' and loginer<>'%2' "
                            "order by loginer;")
             .arg(bossAccount).arg(mapMsg.value("word_admin")));
    while ( qry.next() ) {
        QListWidgetItem* it = new QListWidgetItem(qry.value(0).toString());
        it->setData(Qt::UserRole + UDATA_DIRTY, false);
        it->setData(Qt::UserRole + UDATA_DESKPASS, qry.value(1).toString());
        it->setData(Qt::UserRole + UDATA_NETCODE, qry.value(2).toString());
        it->setData(Qt::UserRole + UDATA_SHOP, qry.value(3).toString());
        it->setData(Qt::UserRole + UDATA_CUSTOMER, qry.value(4).toString());

        bool deviceLocal = qry.value(2).toString().isEmpty();
        bool devicePc    = !deviceLocal && !qry.value(1).toString().isEmpty();
        bool typeBacker  = qry.value(3).toString().isEmpty() && qry.value(4).toString().isEmpty();
        bool typeShop    = !qry.value(3).toString().isEmpty();
        it->setData(Qt::DecorationRole, QIcon(iconFileOf(deviceLocal, devicePc, typeBacker, typeShop)));

        mpList->addItem(it);
    }
    qry.finish();

    //初始
    mpLoginRole->hide();
    mpRoleInfo->hide();
    mpSaveBox->hide();
    connect(mpBtnAddLoginer, SIGNAL(clicked(bool)), this, SLOT(addLoginer()));
    connect(mpBtnSaveChange, SIGNAL(clicked(bool)), this, SLOT(saveChange()));
    connect(mpBtnCancelChange, SIGNAL(clicked(bool)), this, SLOT(cancelChange()));
    connect(mpList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(pickLoginerChanged(QListWidgetItem*,QListWidgetItem*)));

    mpRetPrice->hide();
    mpLotPrice->hide();
    mpBuyPrice->hide();
    mpTab->tabBar()->hide();

    setMinimumSize(QSize(sizeHint().width(), sizeHint().height() + 100));
}

void BsSetLoginer::addLoginer()
{
    BsLoginerBaseInfoDlg dlg(this, false);
    if ( dlg.exec() != QDialog::Accepted )
        return;

    QString loginer = dlg.mpEdtName->text().trimmed().replace(QRegularExpression("\\s+"), QString());
    QString bindShop = ( dlg.mpTypeShop->isChecked() ) ? dlg.mpCmdBind->currentText() : QString();
    QString bindCustomer = ( dlg.mpTypeCustomer->isChecked() ) ? dlg.mpCmdBind->currentText() : QString();
    QString deskPass = dlg.getDeskPass();
    QString netCode = dlg.getNetCode();

    loginer.replace(QChar(39), QString());
    bindShop.replace(QChar(39), QString());
    bindCustomer.replace(QChar(39), QString());
    deskPass.replace(QChar(39), QString());
    netCode.replace(QChar(39), QString());

    QSqlQuery qry;
    QString sql = QStringLiteral("insert into baililoginer(loginer, bindShop, bindCustomer, deskPassword, passHash) "
                                 "values('%1', '%2', '%3', '%4', '%5');")
            .arg(loginer).arg(bindShop).arg(bindCustomer).arg(deskPass).arg(netCode);
    qry.exec(sql);
    if ( !qry.lastError().isValid() ) {
        QListWidgetItem* it = new QListWidgetItem(loginer);
        it->setData(Qt::UserRole + UDATA_DIRTY, false);
        it->setData(Qt::UserRole + UDATA_DESKPASS, deskPass);
        it->setData(Qt::UserRole + UDATA_NETCODE, netCode);
        it->setData(Qt::UserRole + UDATA_SHOP, bindShop);
        it->setData(Qt::UserRole + UDATA_CUSTOMER, bindCustomer);

        bool deviceLocal = netCode.isEmpty();
        bool devicePc    = !deviceLocal && !deskPass.isEmpty();
        bool typeBacker  = bindShop.isEmpty() && bindCustomer.isEmpty();
        bool typeShop    = !bindShop.isEmpty();
        it->setData(Qt::DecorationRole, QIcon(iconFileOf(deviceLocal, devicePc, typeBacker, typeShop)));

        mpList->addItem(it);
        mpList->setCurrentRow(mpList->count() - 1);
        loadLoginer(it);
        BsFronterMap::loadUpdate();
        QMessageBox::information(this, QString(), QStringLiteral("添加用户成功，请重启服务！"));
    }
    else
        QMessageBox::information(this, QString(), qry.lastError().text());
}

void BsSetLoginer::delLoginer()
{
    QListWidgetItem *it = mpList->currentItem();
    if ( !it )
        return;

    QSqlQuery qry;
    QString sql = QStringLiteral("delete from baililoginer where loginer='%1';").arg(it->text());
    qry.exec(sql);

    if ( !qry.lastError().isValid() ) {
        mpList->takeItem(mpList->currentRow());
        delete it;
        mpModelRegis->clearData();
        mpModelSheet->clearData();
        mpModelQuery->clearData();
        BsFronterMap::loadUpdate();
        QMessageBox::information(this, QString(), QStringLiteral("删除用户成功，请重启服务！"));
    }
    else {
        QMessageBox::information(this, QString(), qry.lastError().text());
    }
}

void BsSetLoginer::resetPassword()
{
    QListWidgetItem* it = mpList->currentItem();

    BsLoginerBaseInfoDlg dlg(this, true);
    dlg.mpEdtName->setText(it->text());
    dlg.setPasswordAndDeviceType(it->data(Qt::UserRole + UDATA_DESKPASS).toString(),
                                 it->data(Qt::UserRole + UDATA_NETCODE).toString(),
                                 it->data(Qt::UserRole + UDATA_SHOP).toString(),
                                 it->data(Qt::UserRole + UDATA_CUSTOMER).toString());
    dlg.setWindowTitle(QStringLiteral("重置密码"));
    if ( dlg.exec() == QDialog::Accepted ) {

        QString deskPass = dlg.getDeskPass();
        QString netCode = dlg.getNetCode();
        deskPass.replace(QChar(39), QString());
        netCode.replace(QChar(39), QString());

        QSqlQuery qry;
        QString sql = QStringLiteral("update baililoginer set deskPassword='%1', passhash='%2' where loginer='%3';")
                .arg(deskPass).arg(netCode).arg(it->text());
        qry.exec(sql);
        if ( qry.lastError().isValid() )
            QMessageBox::information(this, QString(), qry.lastError().text());
        else {
            it->setData(Qt::UserRole + UDATA_DESKPASS, deskPass);
            it->setData(Qt::UserRole + UDATA_NETCODE, netCode);
            loadLoginer(it);
            BsFronterMap::loadUpdate();
            QMessageBox::information(this, QString(), QStringLiteral("密码更新成功，请重启服务！"));
        }
    }
}

void BsSetLoginer::presetMinRight()
{
    presetSaveRight(false);
    mpList->currentItem()->setForeground(QColor(0, 0, 0));
    mpList->currentItem()->setData(Qt::UserRole + UDATA_DIRTY, false);
    loadLoginer(mpList->currentItem());
    BsFronterMap::loadUpdate();
}

void BsSetLoginer::presetMaxRight()
{
    presetSaveRight(true);
    mpList->currentItem()->setForeground(QColor(0, 0, 0));
    mpList->currentItem()->setData(Qt::UserRole + UDATA_DIRTY, false);
    loadLoginer(mpList->currentItem());
    BsFronterMap::loadUpdate();
}

void BsSetLoginer::saveChange()
{
    QStringList setExps;

    setExps << QStringLiteral("retprice=%1").arg((mpRetPrice->isChecked()) ? -1 : 0);
    setExps << QStringLiteral("lotprice=%1").arg((mpLotPrice->isChecked()) ? -1 : 0);
    setExps << QStringLiteral("buyprice=%1").arg((mpBuyPrice->isChecked()) ? -1 : 0);
    setExps << QStringLiteral("limcargoexp='%1'").arg(mpLimCargoExp->text().replace(QChar(39), QString()));

    for ( int i = 0, iLen = lstRegisWinTableNames.length(); i < iLen; ++i ) {
        QModelIndex dataIdx = mpModelRegis->index(i, 0);
        setExps << QStringLiteral("%1=%2").arg(lstRegisWinTableNames.at(i))
                   .arg(mpModelRegis->data(dataIdx, Qt::EditRole).toUInt());
    }

    for ( int i = 0, iLen = lstSheetWinTableNames.length(); i < iLen; ++i ) {
        QModelIndex dataIdx = mpModelSheet->index(i, 0);
        setExps << QStringLiteral("%1=%2").arg(lstSheetWinTableNames.at(i))
                   .arg(mpModelSheet->data(dataIdx, Qt::EditRole).toUInt());
    }

    for ( int i = 0, iLen = lstQueryWinTableNames.length(); i < iLen; ++i ) {
        QModelIndex dataIdx = mpModelQuery->index(i, 0);
        setExps << QStringLiteral("%1=%2").arg(lstQueryWinTableNames.at(i))
                   .arg(mpModelQuery->data(dataIdx, Qt::EditRole).toUInt());
    }

    QString sql = QStringLiteral("update baililoginer set %1 where loginer='%2';")
            .arg(setExps.join(QChar(44))).arg(mpList->currentItem()->text());
    QSqlQuery qry;
    qry.exec(sql);
    if ( qry.lastError().isValid() ) {
        qDebug() << qry.lastError().text() << "\n" << sql;
    }
    else {
        mpList->currentItem()->setForeground(QColor(0, 0, 0));
        mpList->currentItem()->setData(Qt::UserRole + UDATA_DIRTY, false);
        mpSaveBox->hide();
        BsFronterMap::loadUpdate();
    }
}

void BsSetLoginer::cancelChange()
{
    mpList->currentItem()->setForeground(QColor(0, 0, 0));
    mpList->currentItem()->setData(Qt::UserRole + UDATA_DIRTY, false);
    mpSaveBox->hide();

    loadLoginer(mpList->currentItem());
}

void BsSetLoginer::pickLoginerChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if ( current ) {
        loadLoginer(current);
    }
}

void BsSetLoginer::rightChanged()
{
    if ( mpList->currentItem() ) {
        mpList->currentItem()->setForeground(QColor(255, 0, 0));
        mpList->currentItem()->setData(Qt::UserRole + UDATA_DIRTY, true);
        mpSaveBox->show();
    }
}

void BsSetLoginer::loadLoginer(const QListWidgetItem *it)
{
    disconnect(mpRetPrice, nullptr, nullptr, nullptr);
    disconnect(mpLotPrice, nullptr, nullptr, nullptr);
    disconnect(mpBuyPrice, nullptr, nullptr, nullptr);
    disconnect(mpLimCargoExp, nullptr, nullptr, nullptr);
    disconnect(mpModelRegis, nullptr, nullptr, nullptr);
    disconnect(mpModelSheet, nullptr, nullptr, nullptr);
    disconnect(mpModelQuery, nullptr, nullptr, nullptr);

    QString bindShop = it->data(Qt::UserRole + UDATA_SHOP).toString();
    QString bindCustomer = it->data(Qt::UserRole + UDATA_CUSTOMER).toString();
    QString binding = bindShop.isEmpty() ? QString() : QStringLiteral("（%1）").arg(bindShop);
    if ( bindShop.isEmpty() && !bindCustomer.isEmpty() ) {
        binding = QStringLiteral("（%1）").arg(bindCustomer);
    }

    mpCurrentLoginer->setText(QStringLiteral("<b>%1</b><font color='#666'>%2 的权限：</font>")
                              .arg(it->text()).arg(binding));
    mpCurrentLoginer->setStyleSheet(QLatin1String("color:black;"));

    QSqlQuery qry;
    QString sql = QStringLiteral("select deskpassword, bindshop, retprice, lotprice, buyprice, limcargoexp, "
                                 "%1, %2, %3 from baililoginer where loginer='%4';")
            .arg(lstRegisWinTableNames.join(QChar(44)))
            .arg(lstSheetWinTableNames.join(QChar(44)))
            .arg(lstQueryWinTableNames.join(QChar(44)))
            .arg(it->text());
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << "\n" << sql;
    qry.next();

    mpRetPrice->setChecked(qry.value(2).toBool());
    mpLotPrice->setChecked(qry.value(3).toBool());
    mpBuyPrice->setChecked(qry.value(4).toBool());
    mpLimCargoExp->setText(qry.value(5).toString());

    if ( bindShop.isEmpty() && bindCustomer.isEmpty() ) {
        mpLoginRole->setText(QStringLiteral("<b>总部角色</b>"));
        mpRoleInfo->setText(QStringLiteral("前端安全可控用户<br/>"
                                           "无系统配置相关权限。"));
        mpLblCargoExp->hide();
        mpLimCargoExp->hide();
    }
    else if ( bindShop.isEmpty() ) {
        mpLoginRole->setText(QStringLiteral("<b>客户角色</b>&nbsp;绑定&nbsp;<b>%1</b>").arg(bindCustomer));
        mpRoleInfo->setText(QStringLiteral("前端安全可控用户<br/>"
                                           "只能查看及录入自己订货单和批发单<br/>"
                                           "无法审核<br/>"
                                           "只能查询批发类且仅限自己单据<br/>"
                                           "其他权限无效。"));
        mpLblCargoExp->show();
        mpLimCargoExp->show();
    }
    else {
        mpLoginRole->setText(QStringLiteral("<b>门店角色</b>&nbsp;绑定&nbsp;<b>%1</b>").arg(bindShop));
        mpRoleInfo->setText(QStringLiteral("前端安全可控用户<br/>"
                                           "只能查看及录入本店单据和本店统计<br/>"
                                           "查询库存分布只能一款款地查询，无法一下拉出全部货品<br/>"
                                           "其他权限无效。"));
        mpLblCargoExp->show();
        mpLimCargoExp->show();
    }

    mpLoginRole->show();
    mpRoleInfo->show();

    int idxBase = 6;
    QList<uint> regisDatas;
    for ( int i = idxBase; i < idxBase + lstRegisWinTableNames.length(); ++i )
        regisDatas << qry.value(i).toUInt();
    mpModelRegis->loadData(regisDatas);

    idxBase += lstRegisWinTableNames.length();
    QList<uint> sheetDatas;
    for ( int i = idxBase; i < idxBase + lstSheetWinTableNames.length(); ++i )
        sheetDatas << qry.value(i).toUInt();
    mpModelSheet->loadData(sheetDatas);

    idxBase += lstSheetWinTableNames.length();
    QList<uint> queryDatas;
    for ( int i = idxBase; i < idxBase + lstQueryWinTableNames.length(); ++i )
        queryDatas << qry.value(i).toUInt();
    mpModelQuery->loadData(queryDatas);

    mpTab->setCurrentIndex(0);
    mpSaveBox->setVisible(mpList->currentItem()->data(Qt::UserRole + UDATA_DIRTY).toBool());

    connect(mpRetPrice, SIGNAL(clicked(bool)), this, SLOT(rightChanged()));
    connect(mpLotPrice, SIGNAL(clicked(bool)), this, SLOT(rightChanged()));
    connect(mpBuyPrice, SIGNAL(clicked(bool)), this, SLOT(rightChanged()));
    connect(mpLimCargoExp, SIGNAL(textChanged(QString)), this, SLOT(rightChanged()));
    connect(mpModelRegis, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(rightChanged()));
    connect(mpModelSheet, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(rightChanged()));
    connect(mpModelQuery, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(rightChanged()));

    mpRetPrice->show();
    mpLotPrice->show();
    mpBuyPrice->show();
    mpTab->tabBar()->show();
}

void BsSetLoginer::presetSaveRight(const bool maxx)
{
    QStringList setExps;

    setExps << QStringLiteral("retprice=%1").arg((maxx) ? -1 : 0);
    setExps << QStringLiteral("lotprice=%1").arg((maxx) ? -1 : 0);
    setExps << QStringLiteral("buyprice=%1").arg((maxx) ? -1 : 0);

    for ( int i = 0, iLen = lstRegisWinTableNames.length(); i < iLen; ++i ) {
        setExps << QStringLiteral("%1=%2").arg(lstRegisWinTableNames.at(i))
                   .arg((maxx) ? 0x0fffffff : 0);
    }

    for ( int i = 0, iLen = lstSheetWinTableNames.length(); i < iLen; ++i ) {
        setExps << QStringLiteral("%1=%2").arg(lstSheetWinTableNames.at(i))
                   .arg((maxx) ? 0x0fffffff : 0);
    }

    for ( int i = 0, iLen = lstQueryWinTableNames.length(); i < iLen; ++i ) {
        setExps << QStringLiteral("%1=%2").arg(lstQueryWinTableNames.at(i))
                   .arg((maxx) ? 0x0fffffff : 0);
    }

    QString sql = QStringLiteral("update baililoginer set %1 where loginer='%2';")
            .arg(setExps.join(QChar(44))).arg(mpList->currentItem()->text());
    QSqlQuery qry;
    qry.exec(sql);
}

QString BsSetLoginer::iconFileOf(const bool locall, const bool pcc, const bool backerr, const bool shopp)
{
    if ( locall ) {
        return QStringLiteral(":/icon/share.png");
    }
    else if ( pcc ) {
        if ( backerr ) {
            return QStringLiteral(":/icon/pcoffice.png");
        }
        else if ( shopp ) {
            return QStringLiteral(":/icon/pcshop.png");
        }
        else {
            return QStringLiteral(":/icon/pcguest.png");
        }
    }
    else {
        if ( backerr ) {
            return QStringLiteral(":/icon/mobioffice.png");
        }
        else if ( shopp ) {
            return QStringLiteral(":/icon/mobishop.png");
        }
        else {
            return QStringLiteral(":/icon/mobiguest.png");
        }
    }
    return QString();
}

}


// BsUserList
namespace BailiSoft {

BsUserList::BsUserList(BsSetLoginer *parent) : QListWidget(parent), mppWin(parent)
{
    mpMenu = new QMenu(this);
    mpAcAddLoginer  = mpMenu->addAction(QStringLiteral("添加用户"), mppWin, SLOT(addLoginer()));
    mpAcDelLoginer  = mpMenu->addAction(QStringLiteral("删除用户"), mppWin, SLOT(delLoginer()));
    mpMenu->addSeparator();
    mpAcResetPwd    = mpMenu->addAction(QStringLiteral("设置查看密码"), mppWin, SLOT(resetPassword()));
    mpMenu->addSeparator();
    mpAcPresetMin   = mpMenu->addAction(QStringLiteral("预设最小权限"), mppWin, SLOT(presetMinRight()));
    mpAcPresetMax   = mpMenu->addAction(QStringLiteral("预设最大权限"), mppWin, SLOT(presetMaxRight()));
}

void BsUserList::mousePressEvent(QMouseEvent *e)
{
    QListWidget::mousePressEvent(e);

    if ( e->button() != Qt::RightButton )
        return;

    QPoint pt = viewport()->mapFromGlobal(e->globalPos());
    QListWidgetItem *it = itemAt(pt);
    if ( it == currentItem() ) {
        mpAcAddLoginer->setEnabled(true);
        mpAcDelLoginer->setEnabled(currentItem());
        mpAcResetPwd->setEnabled(currentItem());
        mpAcPresetMin->setEnabled(currentItem());
        mpAcPresetMax->setEnabled(currentItem());
        mpMenu->popup(e->globalPos());
    }
}

void BsUserList::paintEvent(QPaintEvent *e)
{
    QListWidget::paintEvent(e);
    if ( count() <= 10 ) {
        QPainter p(viewport());
        p.setPen(QColor(160, 160, 160));
        p.drawText(viewport()->rect(), Qt::AlignHCenter | Qt::AlignBottom,
                   QStringLiteral("更多操作请鼠标右击用户…"));
    }
}

}


// BsRightModel
namespace BailiSoft {

BsRightModel::BsRightModel(const BsRightModelType rmType, QObject *parent)
    : QAbstractTableModel(parent), mRmType(rmType)
{
    if ( mRmType == bsrmtRegis )
        mTvNames = lstRegisWinTableCNames;
    else if ( mRmType == bsrmtSheet )
        mTvNames = lstSheetWinTableCNames;
    else
        mTvNames = lstQueryWinTableCNames;

    if ( mRmType == bsrmtRegis )
        mFlagNames = lstRegisRightFlagCNames;
    else if ( mRmType == bsrmtSheet )
        mFlagNames = lstSheetRightFlagCNames;
    else
        mFlagNames = lstQueryRightFlagCNames;
}

void BsRightModel::clearData()
{
    beginResetModel();
    mDatas.clear();
    endResetModel();
}

void BsRightModel::loadData(const QList<uint> &datas)
{
    beginResetModel();
    mDatas.clear();
    mDatas << datas;
    endResetModel();
}

int BsRightModel::rowCount(const QModelIndex &) const
{
    return mTvNames.length();
}

int BsRightModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant BsRightModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::EditRole ) {
            return mDatas.at(index.row());
    }
    else if ( mDatas.length() > index.row() && role == Qt::DisplayRole ) {
        QStringList grants;
        uint flags = mDatas.at(index.row());
        for ( int i = 0, iLen = mFlagNames.length(); i < iLen; ++i )
        {
            uint f = powtwo(i);
            if ( (flags & f) == f ) {
                grants << mFlagNames.at(i);
            }
        }
        return grants.join(QStringLiteral("，"));
    }
    else if ( role == Qt::TextAlignmentRole )
        return Qt::AlignCenter;

    return QVariant();
}

QVariant BsRightModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role == Qt::DisplayRole )
    {
        if ( orientation == Qt::Horizontal )
            return QStringLiteral("权限");
        else
            return QStringLiteral("  %1  ").arg(mTvNames.at(section));
    }

    return QVariant();
}

Qt::ItemFlags BsRightModel::flags(const QModelIndex &index) const
{
    if ( mDatas.isEmpty() )
        return Qt::NoItemFlags;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool BsRightModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole && index.isValid()) {
        mDatas[index.row()] = value.toUInt();
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

}


// BsRightView
namespace BailiSoft {

BsRightView::BsRightView(QWidget *parent) : QTableView(parent)
{
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setStyleSheet("QHeaderView::section{border-style:none; border-bottom:1px solid #ccc;}");

    verticalHeader()->setDefaultSectionSize(24);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setStyleSheet("QHeaderView{background-color:#ccc;} "
                                    "QHeaderView::section{border-style:none; border-bottom:1px solid #ccc;}");

    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setSortingEnabled(false);
}

}


// BsRightDelegate
namespace BailiSoft {

BsRightDelegate::BsRightDelegate(QWidget *parent, const QStringList &options, const bool disableActMoney,
                                 const bool disableDisMoney, const bool disablePayOwe)
    : QStyledItemDelegate(parent), mOptions(options), mDisableActMoney(disableActMoney),
      mDisableDisMoney(disableDisMoney), mDisablePayOwe(disablePayOwe)
{
}

QWidget *BsRightDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
    QWidget *pnl = new QWidget(parent);
    QHBoxLayout *lay = new QHBoxLayout(pnl);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);
    lay->addStretch();
    for ( int i = 0, iLen = mOptions.length(); i < iLen; ++i ) {
        QCheckBox *chk = new QCheckBox(mOptions.at(i), pnl);
        if ( mDisableActMoney && i == 1 ) chk->setEnabled(false);
        if ( mDisableDisMoney && i == 2 ) chk->setEnabled(false);
        if ( mDisablePayOwe && (i == 3 || i ==4) ) chk->setEnabled(false);
        lay->addWidget(chk, 0, Qt::AlignVCenter);
    }
    lay->addStretch();
    pnl->setStyleSheet(".QWidget{background-color:#fff;}");  //默认为透明，故需如此
    return pnl;
}

void BsRightDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QWidget *pnl = qobject_cast<QWidget *>(editor);
    uint val = index.model()->data(index, Qt::EditRole).toUInt();
    pnl->setProperty("flags", val);
    for ( int i = 0, iLen = mOptions.length(); i < iLen; ++i ) {
        QCheckBox *chk = getCheckBoxByText(pnl, mOptions.at(i));
        if ( chk ) {
            bool picked = (val & powtwo(i));
            chk->setChecked(picked && chk->isEnabled());
        }
    }
}

void BsRightDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QWidget *pnl = qobject_cast<QWidget *>(editor);
    uint oldFlags = pnl->property("flags").toUInt();
    uint newFlags = 0;
    for ( int i = 0, iLen = mOptions.length(); i < iLen; ++i ) {
        QCheckBox *chk = getCheckBoxByText(pnl, mOptions.at(i));
        if ( chk ) {
            if ( chk->isChecked() ) {
                newFlags |= powtwo(i);
            }
        }
    }
    if ( newFlags != oldFlags ) {
        model->setData(index, newFlags, Qt::EditRole);
        emit model->dataChanged(index, index);
    }
}

QCheckBox *BsRightDelegate::getCheckBoxByText(QWidget *pnl, const QString &txt) const
{
    for ( int i = 0, iLen = pnl->children().count(); i < iLen; ++i ) {
        QCheckBox *chk = qobject_cast<QCheckBox *>(pnl->children().at(i));
        if ( chk ) {
            if ( chk->text() == txt ) {
                return chk;
            }
        }
    }
    return nullptr;
}

}

