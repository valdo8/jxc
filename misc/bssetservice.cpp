#include "bssetservice.h"
#include "main/bailicode.h"
#include "main/bailicustom.h"
#include "main/bailidata.h"
#include "main/bailifunc.h"
#include "main/bailishare.h"
#include "main/bailiserver.h"

#define RUNNING_HINT  "为保持服务不会中断，请将电脑省电模式设为“从不”。"
#define STOPPING_HINT "本网络服务安全性远高于传统ERP类系统架构，系统代码已经完全开源，请放心开启网络服务功能。"

namespace BailiSoft {

const QString user_info_pattern = QStringLiteral("用户名:<font color='red'>%1</font> "
                                                 "登录码:<font color='red'>%2</font>");

BsSetService::BsSetService(QWidget *parent, BsServer *netServer) : QTabWidget(parent)
{
    mppServer = netServer;
    mppServer->stopAutoKeeper();

    mTicker.setInterval(1000);
    mTicker.setSingleShot(false);
    connect(&mTicker, SIGNAL(timeout()), this, SLOT(waitRestartTick()));

    setProperty(BSWIN_TABLE, QStringLiteral("set_service"));
    setTabPosition(QTabWidget::South);
    setWindowTitle(QStringLiteral("网络后台"));
    if ( qApp->primaryScreen()->availableSize().height() < 1000)
        setMinimumSize(800, 500);
    else
        setMinimumSize(1000, 700);

    //开关页
    mpLblManulTitle = new QLabel(this);
    mpLblManulTitle->setText(QStringLiteral("<h2><font color='grey'>移动终端使用说明</font></h2>"));

    QString safekey = mapOption.value(QStringLiteral("app_encryption_key")).trimmed();
    if ( safekey.isEmpty() ) safekey = QStringLiteral("空白不填");
    mpLblManualContent = new QLabel(this);
    mpLblManualContent->setText(QStringLiteral(
                                    "<p>● 安卓系统（手机或平板）安装包下载地址："
                                    "<font color='green'><b>www.bailisoft.com/vip/app.apk</b></font>、"
                                    "苹果系统（手机或平板）地址："
                                    "<font color='green'><b>www.bailisoft.com/vip/app</b></font>。</p>"
                                    "<p>● 终端用户登录账号及权限分配，请到“用户与权限”窗口。</p>"
                                    "<p>● 终端登录所需后台名称为：<font color='red'>%1</font>。"
                                    "用户名与登录码可在本窗口底部“用户日志”页中选择具体用户查询。"
                                    "保密码统一为：<font color='red'>%2</font>。</p>"
                                    "<p>● 保密码可到“系统参数”窗口查看和自行设置。如有设置，则用户登录设置也必须一致。"
                                    "如未设置（空白），则登录设置处也相应留空不填。</p>"
                                    "<p>● 为保证终端始终能登录连接，请到系统设置中将电脑节电模式的闲置睡眠时间"
                                    "设为“从不”。屏幕关闭不影响服务。启动后关闭本窗口也不影响服务。</p>"
                                    "<p>● 用户日志中可查询所有用户所有时间的所有登录操作活动记录、以及查看密码。"
                                    "关闭本窗口进行所有其他窗口操作，均不影响服务。</p>"
                                    ).arg(dogNetName).arg(safekey));
    mpLblManualContent->setWordWrap(false);
    mpLblManualContent->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    QFont ftBig = font();
    ftBig.setPointSize(2 * ftBig.pointSize());

    mpBtnStart = new QPushButton(QIcon(":/icon/serveron.png"), mapMsg.value("word_start"), this);
    mpBtnStart->setFont(ftBig);
    mpBtnStart->setIconSize(QSize(32, 32));
    mpBtnStart->setFixedSize(150, 60);
    connect(mpBtnStart, SIGNAL(clicked(bool)), this, SLOT(clickServerStart()));

    mpBtnStop = new QPushButton(QIcon(":/icon/serveroff.png"), mapMsg.value("word_stop"), this);
    mpBtnStop->setFont(ftBig);
    mpBtnStop->setIconSize(QSize(32, 32));
    mpBtnStop->setFixedSize(150, 60);
    connect(mpBtnStop, SIGNAL(clicked(bool)), this, SLOT(clickServerStop()));

    QWidget *pnlSvrButtons = new QWidget(this);
    QHBoxLayout *laySvrButtons = new QHBoxLayout(pnlSvrButtons);
    laySvrButtons->addStretch();
    laySvrButtons->addWidget(mpBtnStart);
    laySvrButtons->addSpacing(32);
    laySvrButtons->addWidget(mpBtnStop);
    laySvrButtons->addStretch();

    mpLblPowerHint = new QLabel(this);
    mpLblPowerHint->setText(QStringLiteral(RUNNING_HINT));
    mpLblPowerHint->setAlignment(Qt::AlignCenter);
    mpLblPowerHint->setStyleSheet(QStringLiteral("color:red;"));

    mpPnlSvr = new QWidget(this);
    QVBoxLayout *laySvr = new QVBoxLayout(mpPnlSvr);
    laySvr->addStretch(1);
    laySvr->addWidget(mpLblManulTitle, 0, Qt::AlignCenter);
    laySvr->addSpacing(32);
    laySvr->addWidget(mpLblManualContent, 0, Qt::AlignCenter);
    laySvr->addSpacing(64);
    laySvr->addWidget(pnlSvrButtons, 0, Qt::AlignCenter);
    laySvr->addStretch(5);
    laySvr->addWidget(mpLblPowerHint, 0, Qt::AlignCenter);

    //服务日志页
    mpLogConDateB = new QDateEdit(QDate::currentDate(), this);
    mpLogConDateE = new QDateEdit(QDate::currentDate(), this);

    mpLogConUser = new QComboBox(this);
    mpLogConUser->setMinimumWidth(200);

    mpLogBtnQry = new QPushButton(QIcon(":/icon/query.png"), mapMsg.value("word_query"), this);
    connect(mpLogBtnQry, SIGNAL(clicked(bool)), this, SLOT(qryLog()));

    mpLblUserInfo = new QLabel(this);
    QFont ft(mpLblUserInfo->font());
    ft.setFamily(QStringLiteral("monospace"));
    mpLblUserInfo->setFont(ft);

    mpBtnBossword = new QToolButton(this);
    mpBtnBossword->setIcon(QIcon(":/icon/config.png"));
    mpBtnBossword->setText(QStringLiteral("特设"));
    mpBtnBossword->setIconSize(QSize(16, 16));
    mpBtnBossword->setToolTip(QStringLiteral("总经理前端登录码在此特别设置更改"));
    mpBtnBossword->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mpBtnBossword->hide();
    connect(mpBtnBossword, SIGNAL(clicked(bool)), this, SLOT(clickSetBossword()));

    mpPnlLogCon = new QWidget(this);
    QHBoxLayout *layLogCon = new QHBoxLayout(mpPnlLogCon);
    layLogCon->setContentsMargins(0, 0, 0, 0);
    layLogCon->setSpacing(0);
    layLogCon->addWidget(new QLabel(QStringLiteral("日期：")));
    layLogCon->addWidget(mpLogConDateB);
    layLogCon->addWidget(new QLabel(QStringLiteral("~")));
    layLogCon->addWidget(mpLogConDateE);
    layLogCon->addSpacing(30);
    layLogCon->addWidget(new QLabel(QStringLiteral("用户：")));
    layLogCon->addWidget(mpLogConUser);
    layLogCon->addSpacing(30);
    layLogCon->addWidget(mpLogBtnQry);
    layLogCon->addSpacing(30);
    layLogCon->addWidget(mpLblUserInfo);
    layLogCon->addWidget(mpBtnBossword);
    layLogCon->addStretch();

    QStringList logFlds;
    logFlds << QStringLiteral("时间") << QStringLiteral("用户") << QStringLiteral("类型") << QStringLiteral("细节");
    mpGrdLog = new QTableWidget(this);
    mpGrdLog->setColumnCount(4);
    mpGrdLog->setHorizontalHeaderLabels(logFlds);
    mpGrdLog->horizontalHeader()->setStretchLastSection(true);
    mpGrdLog->horizontalHeader()->setStyleSheet("QHeaderView{border-style:none; border-bottom:1px solid silver;} ");
    mpGrdLog->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mpGrdLog->verticalHeader()->setDefaultSectionSize(24);
    mpGrdLog->verticalHeader()->setStyleSheet("color:#999;");
    mpGrdLog->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mpGrdLog->setSelectionBehavior(QAbstractItemView::SelectRows);
    mpGrdLog->setSelectionMode(QAbstractItemView::SingleSelection);
    mpGrdLog->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mpGrdLog->setSortingEnabled(true);
    mpGrdLog->setAlternatingRowColors(true);
    mpGrdLog->setStyleSheet(mapMsg.value("css_grid_readonly"));

    mpPnlLog = new QWidget(this);
    QVBoxLayout *layLog = new QVBoxLayout(mpPnlLog);
    layLog->addWidget(mpPnlLogCon);
    layLog->addWidget(mpGrdLog, 1);

    //聊天记录页
    mpChatConDateB = new QDateEdit(QDate::currentDate(), this);
    mpChatConDateE = new QDateEdit(QDate::currentDate(), this);

    mpChatConSender = new QComboBox(this);
    mpChatConSender->setMinimumWidth(200);

    mpChatConReceiver = new QComboBox(this);
    mpChatConReceiver->setMinimumWidth(200);

    mpChatBtnQry = new QPushButton(QIcon(":/icon/query.png"), mapMsg.value("word_query"), this);
    connect(mpChatBtnQry, SIGNAL(clicked(bool)), this, SLOT(qryChat()));

    mpPnlChatCon = new QWidget(this);
    QHBoxLayout *layChatCon = new QHBoxLayout(mpPnlChatCon);
    layChatCon->setContentsMargins(0, 0, 0, 0);
    layChatCon->setSpacing(0);
    layChatCon->addWidget(new QLabel(QStringLiteral("日期：")));
    layChatCon->addWidget(mpChatConDateB);
    layChatCon->addWidget(new QLabel(QStringLiteral("~")));
    layChatCon->addWidget(mpChatConDateE);
    layChatCon->addSpacing(30);
    layChatCon->addWidget(new QLabel(QStringLiteral("发送人：")));
    layChatCon->addWidget(mpChatConSender);
    layChatCon->addSpacing(30);
    layChatCon->addWidget(new QLabel(QStringLiteral("接收人：")));
    layChatCon->addWidget(mpChatConReceiver);
    layChatCon->addSpacing(30);
    layChatCon->addWidget(mpChatBtnQry);
    layChatCon->addStretch();

    QStringList chatFlds;
    chatFlds << QStringLiteral("时间") << QStringLiteral("发送人") << QStringLiteral("接收人") << QStringLiteral("内容");
    mpGrdChat = new QTableWidget(this);
    mpGrdChat->setColumnCount(4);
    mpGrdChat->setHorizontalHeaderLabels(chatFlds);
    mpGrdChat->horizontalHeader()->setStretchLastSection(true);
    mpGrdChat->horizontalHeader()->setStyleSheet("QHeaderView{border-style:none; border-bottom:1px solid silver;} ");
    mpGrdChat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mpGrdChat->verticalHeader()->setDefaultSectionSize(24);
    mpGrdChat->verticalHeader()->setStyleSheet("color:#999;");
    mpGrdChat->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mpGrdChat->setSelectionBehavior(QAbstractItemView::SelectRows);
    mpGrdChat->setSelectionMode(QAbstractItemView::SingleSelection);
    mpGrdChat->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mpGrdChat->setSortingEnabled(true);
    mpGrdChat->setAlternatingRowColors(true);
    mpGrdChat->setStyleSheet(mapMsg.value("css_grid_readonly"));

    mpPnlChat = new QWidget(this);
    QVBoxLayout *layChat = new QVBoxLayout(mpPnlChat);
    layChat->addWidget(mpPnlChatCon);
    layChat->addWidget(mpGrdChat, 1);

    //布局
    addTab(mpPnlSvr,  QStringLiteral("服务开关"));
    addTab(mpPnlLog,  QStringLiteral("用户日志"));
    addTab(mpPnlChat, QStringLiteral("沟通记录"));

    //加载用户选择表
    mBossIndex = -1;
    loadFrontUsers();
    loadChaters();

    //服务状态
    connect(mppServer, SIGNAL(serverStarted(qint64)), this, SLOT(serviceStarted(qint64)));
    connect(mppServer, SIGNAL(serverStopped()), this, SLOT(serviceStopped()));
    connect(mppServer, SIGNAL(startFailed(QString)), this, SLOT(startFailed(QString)));
    if ( mppServer->isRunning() ) {
        mpBtnStart->setEnabled(false);
        mpBtnStop->setEnabled(true);
        mpLblPowerHint->setText(QStringLiteral(RUNNING_HINT));
        mpLblPowerHint->setStyleSheet(QStringLiteral("color:red;"));
    } else {        
        mpBtnStart->setEnabled(true);
        mpBtnStop->setEnabled(false);
        mpLblPowerHint->setText(QStringLiteral(STOPPING_HINT));
        mpLblPowerHint->setStyleSheet(QStringLiteral("color:#090;"));
    }
}

BsSetService::~BsSetService()
{
    disconnect(mppServer, nullptr, this, nullptr);
}

void BsSetService::closeEvent(QCloseEvent *event)
{
    if ( mRestartTicks > 0 ) {
        event->ignore();
        return;
    }
    QWidget::closeEvent(event);
}

void BsSetService::clickServerStart()
{
    if ( dogNetName.isEmpty() || dogNetPass.isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("没有发现有效的网络狗，不能启动后台服务！"));
        return;
    }

    //发出启动指令
    mUserClicking = true;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    mppServer->startServer(dogNetName, dogNetPass, mpLogConUser->count());
}

void BsSetService::clickServerStop()
{
    //检查停止
    if ( mppServer->isRunning() ) {
        if ( confirmDialog(this,
                           QStringLiteral("停止网络服务后所有手机平板电脑等终端将不能正常工作。"),
                           QStringLiteral("确定要停止服务吗？"),
                           mapMsg.value("btn_ok"),
                           mapMsg.value("btn_cancel"),
                           QMessageBox::Question) ) {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            mppServer->stopServer();
        }
    }
}

void BsSetService::clickSetBossword()
{
    //读取原密码
    QString sql = QStringLiteral("select passhash from baililoginer where loginer='%1';")
            .arg(bossAccount);
    QSqlQuery qry;
    qry.exec(sql);
    qry.next();
    QString oldPass = qry.value(0).toString();

    //对话框
    QInputDialog dlg(this);
    dlg.setLabelText(QStringLiteral("总经理前端登录码专设:"));
    dlg.setTextValue(oldPass);
    dlg.setTextEchoMode(QLineEdit::Password);
    dlg.setOkButtonText(mapMsg.value(QStringLiteral("btn_ok")));
    dlg.setCancelButtonText(mapMsg.value(QStringLiteral("btn_cancel")));
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.adjustSize();
    if ( dlg.exec() == QDialog::Accepted ) {
        QString newPass = dlg.textValue();
        if ( newPass.trimmed().isEmpty() ) {
            QMessageBox::information(this, QString(), QStringLiteral("密码太短！"));
            return;
        }
        sql = QStringLiteral("update baililoginer set passhash='%1' where loginer='%2';")
                .arg(newPass).arg(bossAccount);
        qry.exec(sql);
        if ( ! qry.lastError().isValid() ) {
            loadFrontUsers();
            if ( newPass != oldPass && mppServer->isRunning() ) {
                BsFronterMap::loadUpdate();
            }
        }
        else
            QMessageBox::information(this, QString(), QStringLiteral("设置不成功！"));
    }
}

void BsSetService::startFailed(const QString &errMsg)
{
    mpBtnStop->setEnabled(false);
    QApplication::restoreOverrideCursor();
    QMessageBox::information(this, QString(), errMsg);
    mUserClicking = false;

    mRestartTicks = 60;
    mTicker.start();
}

void BsSetService::qryLog()
{
    qint64 dateBegin = QDateTime(mpLogConDateB->date()).toMSecsSinceEpoch();
    qint64 dateEnd = QDateTime(mpLogConDateE->date().addDays(1)).toMSecsSinceEpoch() - 1;
    QString where = QStringLiteral("where (reqtime between %1 and %2)").arg(dateBegin).arg(dateEnd);
    if ( mpLogConUser->currentIndex() >= 0 )
        where += QStringLiteral(" and (reqman='%1')").arg(mpLogConUser->currentText());

    QString sql = QStringLiteral("select reqtime, reqman, "
                                 "(case reqtype "
                                 "  when 1 then '登录' "
                                 "  when 2 then '开货单' "
                                 "  when 3 then '填报销' "
                                 "  when 4 then '登记项' "
                                 "  when 5 then '查业务' "
                                 "  when 6 then '查欠款' "
                                 "  when 7 then '查欠货' "
                                 "  when 8 then '查库存' "
                                 "  when 9 then '查一览' "
                                 "  when 10 then '查登记' "
                                 "  when 11 then '查图' "
                                 "  else '未知' end) as rtype, "
                                 "reqinfo from serverlog %1;").arg(where);
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) { qDebug() << qry.lastError(); return; }
    mpGrdLog->clearContents();
    mpGrdLog->setRowCount(0);
    while ( qry.next() ) {
        qint64 reqTime = qry.value(0).toLongLong();
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(reqTime);
        QTableWidgetItem *itTime = new QTableWidgetItem(dt.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        QTableWidgetItem *itMan  = new QTableWidgetItem(qry.value(1).toString());
        QTableWidgetItem *itType = new QTableWidgetItem(qry.value(2).toString());
        QTableWidgetItem *itInfo = new QTableWidgetItem(qry.value(3).toString());
        int row = mpGrdLog->rowCount();
        mpGrdLog->setRowCount(row + 1);
        mpGrdLog->setItem(row, 0, itTime);
        mpGrdLog->setItem(row, 1, itMan);
        mpGrdLog->setItem(row, 2, itType);
        mpGrdLog->setItem(row, 3, itInfo);
    }
    qry.finish();
}

void BsSetService::qryChat()
{
    //chatFlds << QStringLiteral("时间") << QStringLiteral("发送人") << QStringLiteral("接收人") << QStringLiteral("内容");
    qint64 dateBegin = QDateTime(mpChatConDateB->date()).toMSecsSinceEpoch();
    qint64 dateEnd = QDateTime(mpChatConDateE->date().addDays(1)).toMSecsSinceEpoch() - 1;
    QString where = QStringLiteral("where (msgid between %1 and %2)").arg(dateBegin * 1000).arg(dateEnd * 1000);  //Flutter微秒
    if ( mpChatConSender->currentIndex() >= 0 )
        where += QStringLiteral(" and (senderName='%1')").arg(mpChatConSender->currentText());
    if ( mpChatConReceiver->currentIndex() >= 0 )
        where += QStringLiteral(" and (receiverName='%1')").arg(mpChatConReceiver->currentText());

    QString sql = QStringLiteral("select msgid, senderName, receiverName, content from msglog %1;").arg(where);
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) { qDebug() << qry.lastError(); return; }
    mpGrdChat->clearContents();
    mpGrdChat->setRowCount(0);
    while ( qry.next() ) {
        qint64 msgIdTime = qry.value(0).toLongLong();
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(msgIdTime / 1000);  //微秒转毫秒
        QTableWidgetItem *itTime = new QTableWidgetItem(dt.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        QTableWidgetItem *itSender  = new QTableWidgetItem(qry.value(1).toString());
        QTableWidgetItem *itReceiver = new QTableWidgetItem(qry.value(2).toString());
        QTableWidgetItem *itContent = new QTableWidgetItem(qry.value(3).toString());
        int row = mpGrdChat->rowCount();
        mpGrdChat->setRowCount(row + 1);
        mpGrdChat->setItem(row, 0, itTime);
        mpGrdChat->setItem(row, 1, itSender);
        mpGrdChat->setItem(row, 2, itReceiver);
        mpGrdChat->setItem(row, 3, itContent);
    }
    qry.finish();
}

void BsSetService::loadFrontUsers()
{
    disconnect(mpLogConUser, SIGNAL(currentIndexChanged(int)), nullptr, nullptr);
    mpLogConUser->clear();
    mpLblUserInfo->setText(QString());
    mpBtnBossword->hide();

    QString sql = QStringLiteral("select loginer, passhash "
                                 "from baililoginer "
                                 "where length(passhash)>0;");
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    int idx = -1;
    while ( qry.next() ) {
        idx++;
        QString loginer = qry.value(0).toString();
        QString netCode = qry.value(1).toString();
        mpLogConUser->addItem(loginer, user_info_pattern.arg(loginer).arg(netCode));
        if ( loginer == bossAccount ) mBossIndex = idx;
    }
    qry.finish();
    mpLogConUser->setCurrentIndex(-1);
    connect(mpLogConUser, SIGNAL(currentIndexChanged(int)), this, SLOT(userComboChanged(int)));
}

void BsSetService::loadChaters()
{
    mpChatConSender->clear();
    mpChatConReceiver->clear();

    QSqlQuery qry;
    QString sql = QStringLiteral("select loginer from baililoginer where length(passhash)>0;");
    qry.setForwardOnly(true);
    qry.exec(sql);
    while ( qry.next() ) {
        mpChatConSender->addItem(qry.value(0).toString());
        mpChatConReceiver->addItem(qry.value(0).toString());
    }

    sql = QStringLiteral("select meetname from meeting;");
    qry.exec(sql);
    while ( qry.next() ) {
        mpChatConSender->addItem(qry.value(0).toString());
        mpChatConReceiver->addItem(qry.value(0).toString());
    }

    qry.finish();
    mpChatConSender->setCurrentIndex(-1);
    mpChatConReceiver->setCurrentIndex(-1);
}

void BsSetService::userComboChanged(const int idx)
{
    mpLblUserInfo->setText(mpLogConUser->currentData().toString());
    mpBtnBossword->setVisible(mBossIndex == idx);
}

void BsSetService::waitRestartTick()
{
    mpBtnStart->setEnabled(false);
    mpBtnStart->setText(QStringLiteral("%1秒").arg(mRestartTicks));

    mRestartTicks--;
    if ( mRestartTicks < 0 ) {
        mTicker.stop();
        mpBtnStart->setEnabled(true);
        mpBtnStart->setText(mapMsg.value("word_start"));
    }
}

void BsSetService::serviceStarted(const qint64 licEpochDate)
{
    mpBtnStart->setEnabled(false);
    mpBtnStop->setEnabled(true);
    mpLblPowerHint->setText(QStringLiteral(RUNNING_HINT));
    mpLblPowerHint->setStyleSheet(QStringLiteral("color:red;"));
    QApplication::restoreOverrideCursor();

    if ( mUserClicking ) {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(licEpochDate);
        QString hint = QStringLiteral("网络服务启动成功！\n服务到期日：%1").arg(dt.toString("yyyy-MM-dd"));
        QMessageBox::information(this,  QString(), hint);
    }
    mUserClicking = false;
}

void BsSetService::serviceStopped()
{
    mpBtnStop->setEnabled(false);
    mpLblPowerHint->setText(QStringLiteral(STOPPING_HINT));
    mpLblPowerHint->setStyleSheet(QStringLiteral("color:#090;"));
    QApplication::restoreOverrideCursor();
    mUserClicking = false;

    mRestartTicks = 60;
    mTicker.start();
}

}
