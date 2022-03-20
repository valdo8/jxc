#include "bailicode.h"
#include "bailicustom.h"
#include "bailidata.h"
#include "bailiwins.h"
#include "bailigrid.h"
#include "bailifunc.h"
#include "bailishare.h"
#include "bailiserver.h"
#include "bailipublisher.h"
#include "bsmain.h"
#include "dialog/bsloginguide.h"
#include "dialog/bssetpassword.h"
#include "dialog/bslicwarning.h"
#include "misc/bsmdiarea.h"
#include "dialog/bsabout.h"
#include "misc/bsoption.h"
#include "misc/bsalarmreport.h"
#include "misc/bssetloginer.h"
#include "misc/bssetservice.h"
#include "misc/bsupg11.h"
#include "tools/bsbatchrename.h"
#include "tools/bsbatchrecheck.h"
#include "tools/bsbarcodemaker.h"
#include "tools/bslabeldesigner.h"
#include "tools/bstoolstockreset.h"
#ifdef Q_OS_WIN
#include "admin_sales/lxsalesmanage.h"
#endif

namespace BailiSoft {

BsMain::BsMain(QWidget *parent) : QMainWindow(parent)
{
    //self
    setMinimumSize(1000, 700);

    //UI
    QMenuBar *mnbar = menuBar();

    QMenu *mnFile = mnbar->addMenu(mapMsg.value("main_system"));
    mnFile->addAction(QStringLiteral("登录向导"), this, SLOT(openLoginGuide()));
    mnFile->addSeparator();
    mnFile->addAction(QStringLiteral("数据文件"), this, SLOT(openFileInfo()));
    mnFile->addSeparator();
    mnFile->addAction(QStringLiteral("退出"), qApp, SLOT(quit()));

    QMenu *mnSet = mnbar->addMenu(mapMsg.value("main_setting"));
    mnSet->addAction(mapMsg.value("win_setpassword").split(QChar(9)).at(0), this, SLOT(openSetPassword()));
    mnSet->addSeparator();
    mpMenuSetLoginer = mnSet->addAction(mapMsg.value("win_setloginer").split(QChar(9)).at(0), this, SLOT(openSetLoginer()));
    mnSet->addSeparator();
    mpMenuSetBarcodeRule = mnSet->addAction(mapMsg.value("win_setbarcode").split(QChar(9)).at(0), this, SLOT(openSetBarcodeRule()));
    mnSet->addSeparator();
    mpMenuSetSystemOption = mnSet->addAction(mapMsg.value("win_setsystem").split(QChar(9)).at(0), this, SLOT(openSetSystemOption()));
    mpMenuSetNetService = mnSet->addAction(mapMsg.value("win_setnet").split(QChar(9)).at(0), this, SLOT(openSetNetService()));

    QMenu *mnReg = mnbar->addMenu(mapMsg.value("main_register"));
    mpMenuSizerType = mnReg->addAction(mapMsg.value("win_sizertype").split(QChar(9)).at(0), this, SLOT(openRegSizerType()));
    mpMenuColorType = mnReg->addAction(mapMsg.value("win_colortype").split(QChar(9)).at(0), this, SLOT(openRegColorType()));
    mpMenuCargo = mnReg->addAction(mapMsg.value("win_cargo").split(QChar(9)).at(0), this, SLOT(openRegCargo()));
    mnReg->addSeparator();
    mpMenuStaff = mnReg->addAction(mapMsg.value("win_staff").split(QChar(9)).at(0), this, SLOT(openRegStaff()));
    mpMenuSubject = mnReg->addAction(mapMsg.value("win_subject").split(QChar(9)).at(0), this, SLOT(openRegSubject()));
    mnReg->addSeparator();
    mpMenuShop = mnReg->addAction(mapMsg.value("win_shop").split(QChar(9)).at(0), this, SLOT(openRegShop()));
    mpMenuSupplier = mnReg->addAction(mapMsg.value("win_supplier").split(QChar(9)).at(0), this, SLOT(openRegSupplier()));
    mpMenuCustomer = mnReg->addAction(mapMsg.value("win_customer").split(QChar(9)).at(0), this, SLOT(openRegCustomer()));
    mnReg->addSeparator();
    mpMenuCustomer = mnReg->addAction(mapMsg.value("win_lotpolicy").split(QChar(9)).at(0), this, SLOT(openRegPolicy()));  //包括ret

    QMenu *mnBuy = mnbar->addMenu(mapMsg.value("main_buy"));
    mpMenuSheetCGD  = mnBuy->addAction(mapMsg.value("win_cgd").split(QChar(9)).at(0), this, SLOT(openSheetCGD()));
    mpMenuQryVICGD  = mnBuy->addAction(mapMsg.value("win_vi_cgd").split(QChar(9)).at(0), this, SLOT(openQryVICGD()));
    mpMenuQryVIYCG  = mnBuy->addAction(mapMsg.value("win_vi_cg_rest").split(QChar(9)).at(0), this, SLOT(openQryViCgRest()));
    mnBuy->addSeparator();
    mpMenuSheetCGJ  = mnBuy->addAction(mapMsg.value("win_cgj").split(QChar(9)).at(0), this, SLOT(openSheetCGJ()));
    mpMenuQryVICGJ  = mnBuy->addAction(mapMsg.value("win_vi_cgj").split(QChar(9)).at(0), this, SLOT(openQryVICGJ()));
    mnBuy->addSeparator();
    mpMenuSheetCGT  = mnBuy->addAction(mapMsg.value("win_cgt").split(QChar(9)).at(0), this, SLOT(openSheetCGT()));
    mpMenuQryVICGT  = mnBuy->addAction(mapMsg.value("win_vi_cgt").split(QChar(9)).at(0), this, SLOT(openQryVICGT()));
    mnBuy->addSeparator();
    mpMenuQryVIJCG  = mnBuy->addAction(mapMsg.value("win_vi_cg").split(QChar(9)).at(0), this, SLOT(openQryViCgNeat()));
    mpMenuQryVIJCGM = mnBuy->addAction(mapMsg.value("win_vi_cg_cash").split(QChar(9)).at(0), this, SLOT(openQryViCgCash()));

    QMenu *mnSale = mnbar->addMenu(mapMsg.value("main_sale"));
    mpMenuSheetPFD  = mnSale->addAction(mapMsg.value("win_pfd").split(QChar(9)).at(0), this, SLOT(openSheetPFD()));
    mpMenuQryVIPFD  = mnSale->addAction(mapMsg.value("win_vi_pfd").split(QChar(9)).at(0), this, SLOT(openQryVIPFD()));
    mpMenuQryVIYPF  = mnSale->addAction(mapMsg.value("win_vi_pf_rest").split(QChar(9)).at(0), this, SLOT(openQryViPfRest()));
    mnSale->addSeparator();
    mpMenuSheetPFF  = mnSale->addAction(mapMsg.value("win_pff").split(QChar(9)).at(0), this, SLOT(openSheetPFF()));
    mpMenuQryVIPFF  = mnSale->addAction(mapMsg.value("win_vi_pff").split(QChar(9)).at(0), this, SLOT(openQryVIPFF()));
    mnSale->addSeparator();
    mpMenuSheetPFT  = mnSale->addAction(mapMsg.value("win_pft").split(QChar(9)).at(0), this, SLOT(openSheetPFT()));
    mpMenuQryVIPFT  = mnSale->addAction(mapMsg.value("win_vi_pft").split(QChar(9)).at(0), this, SLOT(openQryVIPFT()));
    mnSale->addSeparator();
    mpMenuSheetLSD  = mnSale->addAction(mapMsg.value("win_lsd").split(QChar(9)).at(0), this, SLOT(openSheetLSD()));
    mpMenuQryVILSD  = mnSale->addAction(mapMsg.value("win_vi_lsd").split(QChar(9)).at(0), this, SLOT(openQryVILSD()));
    mnSale->addSeparator();
    mpMenuQryVIJPF  = mnSale->addAction(mapMsg.value("win_vi_pf").split(QChar(9)).at(0), this, SLOT(openQryViPfNeat()));
    mpMenuQryVIJPFM = mnSale->addAction(mapMsg.value("win_vi_pf_cash").split(QChar(9)).at(0), this, SLOT(openQryViPfCash()));
    mpMenuQryVIJXS  = mnSale->addAction(mapMsg.value("win_vi_xs").split(QChar(9)).at(0), this, SLOT(openQryViXsNeat()));
    mpMenuQryVIJXSM = mnSale->addAction(mapMsg.value("win_vi_xs_cash").split(QChar(9)).at(0), this, SLOT(openQryViXsCash()));

    QMenu *mnStock = mnbar->addMenu(mapMsg.value("main_stock"));
    mpMenuSheetDBD  = mnStock->addAction(mapMsg.value("win_dbd").split(QChar(9)).at(0), this, SLOT(openSheetDBD()));
    mpMenuQryVIDBD  = mnStock->addAction(mapMsg.value("win_vi_dbd").split(QChar(9)).at(0), this, SLOT(openQryVIDBD()));
    mnStock->addSeparator();
    mpMenuSheetSYD  = mnStock->addAction(mapMsg.value("win_syd").split(QChar(9)).at(0), this, SLOT(openSheetSYD()));
    mpMenuQryVISYD  = mnStock->addAction(mapMsg.value("win_vi_syd").split(QChar(9)).at(0), this, SLOT(openQryVISYD()));
    mnStock->addSeparator();
    mpMenuSheetSZD  = mnStock->addAction(mapMsg.value("win_szd").split(QChar(9)).at(0), this, SLOT(openSheetSZD()));
    mpMenuQryVISZD  = mnStock->addAction(mapMsg.value("win_vi_szd").split(QChar(9)).at(0), this, SLOT(openQryVISZD()));
    mnStock->addSeparator();
    mpMenuQryVIYKC  = mnStock->addAction(mapMsg.value("win_vi_stock").split(QChar(9)).at(0), this, SLOT(openQryViStock()));
    mpMenuQryVIALL  = mnStock->addAction(mapMsg.value("win_vi_all").split(QChar(9)).at(0), this, SLOT(openQryViewAll()));
    mnStock->addSeparator();
    mpMenuQryMinAlarm  = mnStock->addAction(mapMsg.value("win_min_alarm").split(QChar(9)).at(0), this, SLOT(openQryMinAlarm()));
    mpMenuQryMaxAlarm  = mnStock->addAction(mapMsg.value("win_max_alarm").split(QChar(9)).at(0), this, SLOT(openQryMaxAlarm()));


    QMenu *mnTool = mnbar->addMenu(mapMsg.value("main_tool"));
    mpMenuToolStockReset = mnTool->addAction(mapMsg.value("menu_stock_reset"), this, SLOT(openToolStockReset()));
    mpMenuToolBatchCheck = mnTool->addAction(mapMsg.value("menu_batch_check"), this, SLOT(openToolBatchCheck()));
    mpMenuToolBatchEdit = mnTool->addAction(mapMsg.value("menu_batch_edit"), this, SLOT(openToolBatchEdit()));
    mpMenuToolBarcodeMaker = mnTool->addAction(mapMsg.value("menu_barcode_maker"), this, SLOT(openToolBarcodeMaker()));
    mpMenuToolLabelDesigner = mnTool->addAction(mapMsg.value("menu_label_designer"), this, SLOT(openToolLabelDesigner()));
    mnTool->addSeparator();
    mnTool->addAction(mapMsg.value("menu_custom"), this, SLOT(openHelpSite()));


    QMenu* mnHelp = mnbar->addMenu(mapMsg.value("main_help"));
    mnHelp->addAction(QStringLiteral("使用手册"), this, SLOT(openHelpManual()));
    mnHelp->addAction(QStringLiteral("官方网站"), this, SLOT(openHelpSite()));
    mnHelp->addAction(QStringLiteral("关于软件"), this, SLOT(openHelpAbout()));

    //状态栏
    QStatusBar *stbar = statusBar();

    stbar->insertWidget(0, new QLabel(QStringLiteral("当前账册")));
    mpLoginedFile = new QLabel(QStringLiteral("未打开或未连接"));
    mpLoginedFile->setStyleSheet(QLatin1String("color:#090;padding-right:20px;"));
    mpLoginedFile->setMinimumWidth(160);
    stbar->insertWidget(1, mpLoginedFile);

    stbar->insertWidget(2, new QLabel(QStringLiteral("当前用户")));
    mpLoginedUser = new QLabel(QStringLiteral("未登录"));
    mpLoginedUser->setStyleSheet(QLatin1String("color:#090;padding-right:20px;"));
    mpLoginedUser->setMinimumWidth(160);
    stbar->insertWidget(3, mpLoginedUser);

    stbar->insertWidget(4, new QLabel(QStringLiteral("权限角色")));
    mpLoginedRole = new QLabel(QStringLiteral("未知"));
    mpLoginedRole->setStyleSheet(QLatin1String("color:#090;padding-right:20px;"));
    stbar->insertWidget(5, mpLoginedRole);

    stbar->insertWidget(6, new QLabel(QStringLiteral("注册客户")));
    mpLoginedCompany = new QLabel(QString());
    mpLoginedCompany->setStyleSheet(QLatin1String("color:#090;padding-right:20px;"));
    stbar->insertWidget(7, mpLoginedCompany);

    mpBtnNet = new QToolButton(this);
    mpBtnNet->setText(QStringLiteral("单机"));
    mpBtnNet->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mpBtnNet->setIcon(QIcon(":/icon/desktop.png"));
    mpBtnNet->setStyleSheet(QLatin1String("border:none;background:none;"));
    stbar->insertPermanentWidget(8, mpBtnNet);

    //主容器
    QColor backColor = QColor(160, 160, 160);
    mpMdi = new BsMdiArea(this, backColor);
    mpMdi->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mpMdi->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mpMdi->show();
    setCentralWidget(mpMdi);

    //服务预备
    mpServer = new BsServer(this);
    connect(mpServer, SIGNAL(serverStarted(qint64)), this, SLOT(netServerStarted(qint64)));
    connect(mpServer, SIGNAL(serverStopped()), this, SLOT(netServerStopped()));

    //同步器
    mpSentinel = new BsPublisher(this);
    mpSentinel->start();

    //开启登录向导
    QTimer::singleShot(100, this, SLOT(openLoginGuide()));
}

bool BsMain::eventFilter(QObject *watched, QEvent *event)
{
    if ( event->type() == QEvent::StatusTip ) {
        BsWin *w = qobject_cast<BsWin*>(watched);
        QStatusTipEvent *e = static_cast<QStatusTipEvent*>(event);
        if ( w && e ) {
            w->displayGuideTip(e->tip());
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void BsMain::clickQuickButton(const QString &winBaseName, const bool statt)
{
    if ( winBaseName == QStringLiteral("cgd") ) {
        if ( statt )
            openQryVICGD();
        else
            openSheetCGD();
    }
    if ( winBaseName == QStringLiteral("cgj") ) {
        if ( statt )
            openQryVICGJ();
        else
            openSheetCGJ();
    }
    if ( winBaseName == QStringLiteral("cgt") ) {
        if ( statt )
            openQryVICGT();
        else
            openSheetCGT();
    }
    if ( winBaseName == QStringLiteral("dbd") ) {
        if ( statt )
            openQryVIDBD();
        else
            openSheetDBD();
    }
    if ( winBaseName == QStringLiteral("pfd") ) {
        if ( statt )
            openQryVIPFD();
        else
            openSheetPFD();
    }
    if ( winBaseName == QStringLiteral("pff") ) {
        if ( statt )
            openQryVIPFF();
        else
            openSheetPFF();
    }
    if ( winBaseName == QStringLiteral("pft") ) {
        if ( statt )
            openQryVIPFT();
        else
            openSheetPFT();
    }
    if ( winBaseName == QStringLiteral("lsd") ) {
        if ( statt )
            openQryVILSD();
        else
            openSheetLSD();
    }
    if ( winBaseName == QStringLiteral("syd") ) {
        if ( statt )
            openQryVISYD();
        else
            openSheetSYD();
    }
    if ( winBaseName == QStringLiteral("szd") ) {
        if ( statt )
            openQryVISZD();
        else
            openSheetSZD();
    }

    if ( winBaseName == QStringLiteral("jcg") )
        openQryViCgNeat();

    if ( winBaseName == QStringLiteral("jpf") )
        openQryViPfNeat();

    if ( winBaseName == QStringLiteral("jxs") )
        openQryViXsNeat();

    if ( winBaseName == QStringLiteral("cgrest") )
        openQryViCgRest();

    if ( winBaseName == QStringLiteral("cgcash") )
        openQryViCgCash();

    if ( winBaseName == QStringLiteral("pfrest") )
        openQryViPfRest();

    if ( winBaseName == QStringLiteral("pfcash") )
        openQryViPfCash();

    if ( winBaseName == QStringLiteral("xscash") )
        openQryViXsCash();

    if ( winBaseName == QStringLiteral("stock") )
        openQryViStock();

    if ( winBaseName == QStringLiteral("viall") )
        openQryViewAll();
}

void BsMain::openLoginGuide()
{
    if ( mpServer->isRunning() ) {
        QMessageBox::information(this, QString(), QStringLiteral("后台服务运行中，不能切换登录。"));
        return;
    }

    if ( ! questionCloseAllSubWin() )
        return;

    //窗口
    mpMdi->mpPnlGuide->hide();
    BsLoginGuide dlg(this);
    dlg.adjustSize();
    if ( QDialog::Accepted == dlg.exec() )
    {
        if ( mpSentinel->logined() ) {
            mpSentinel->bookLogout();
        }

        //状态栏
        QString loginedRole = (loginShop.isEmpty()) ? QStringLiteral("总部管理") : QStringLiteral("绑定%1").arg(loginShop);
        mpLoginedFile->setText(dlg.getCurrentBook());
        mpLoginedUser->setText(loginer);
        mpLoginedRole->setText(loginedRole);
        mpLoginedCompany->setText(dogUserName);

        //加载配置
        loginLoadOptions(); //必须最先

        //加载结构
        loginLoadRegis();

        //加载权限
        loginLoadRights();

        //设置菜单禁用
        setMenuAllowable();

        //大数据集重置更新时间
        dsCargo->switchBookLogin();
        dsSubject->switchBookLogin();
        dsShop->switchBookLogin();
        dsSupplier->switchBookLogin();
        dsCustomer->switchBookLogin();

        //加载数据集
        dsSizer->reload();
        dsColorType->reload();
        dsColorList->reload();
        dsCargo->reload();
        dsSubject->reload();
        dsShop->reload();
        dsSupplier->reload();
        dsCustomer->reload();

        //向导栏开启
        mpMdi->mpPnlGuide->updateButtonRights();
        mpMdi->expandGuide();

        //共享使用账册禁止后台窗口
        QString bf = dlg.getCurrentBook();
        mpMenuSetNetService->setVisible(!bf.contains(QStringLiteral("//")) && !bf.contains(QStringLiteral("\\\\")));

        //样例文件试用更新日期
        if ( !dogOk && loginBook == mapMsg.value("app_sample_book_name") ) {
            updateSheetDateForMemo();
        }

        //库存同步
        mpSentinel->bookLogin(loginFile);
    }
    else if ( loginer.isEmpty() ) {
        close();
    }
}

void BsMain::closeEvent(QCloseEvent *event)
{
    if ( mpServer->isRunning() ) {
        QMessageBox::information(this, QString(), QStringLiteral("后台服务运行中，不能退出。"));
        event->ignore();
        return;
    }

    QMainWindow::closeEvent(event);

    //等待线程
    mpSentinel->stopWait();
    QEventLoop loop;
    connect(mpSentinel, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    //自动备份
    if ( !loginBook.isEmpty() && !loginFile.contains("//") && !loginFile.contains("\\\\") ) {
        QDir bakDir(backupDir);
        QString bakFile = bakDir.absoluteFilePath(QStringLiteral("%1.bak").arg(loginBook));
        if ( QFile::exists(bakFile) ) QFile::remove(bakFile);
        QFile::copy(loginFile, bakFile);
    }
}

void BsMain::keyPressEvent(QKeyEvent *e)
{
    QMainWindow::keyPressEvent(e);

#ifdef Q_OS_WIN

    //自用
    if ( dogFoundMother ) {


        if ( e->key() == Qt::Key_L && e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier) ) {

            //判断文件是否存在
            QString secretFile = QStringLiteral("G:/BailiR17/设计文档锁密码等/Rockey/blue_admin.txt");
            if ( !QFile::exists(secretFile) ) {
                QMessageBox::information(this, QString(), QStringLiteral("None of yours!?"));
                return;
            }

            //读取文件数据
            QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
            QString fileData;
            QFile f(secretFile);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream strm(&f);
                fileData = strm.readAll();
                f.close();
            }
            QTextCodec::setCodecForLocale(QTextCodec::codecForName("System"));

            //解析密码
            QString password;
            QStringList lines = fileData.split(QChar(10));
            for ( int i = 0, iLen = lines.length(); i < iLen; ++i ) {
                QString line = QString(lines.at(i)).trimmed();
                if ( line.contains(QStringLiteral("管理")) && line.contains(QStringLiteral("密码"))
                     && line.length() > 20 ) {
                    password = line.mid(line.length() - 16);
                }
            }
            if ( password.length() != 16 ) {
                QMessageBox::information(this, QString(), QStringLiteral("锁管理密码文件格式解析错误。"));
                return;
            }

            //打开管理窗口
            if ( ! checkRaiseSubWin("lxsales") ) {
                addNewSubWin(new LxSalesManage(this, password));
            }

        }

        //自用
        if ( e->key() == Qt::Key_U && e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier) ) {
            BsUpg11 dlg(this);
            dlg.exec();
        }
    }
#endif
}

void BsMain::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    mpMdi->shrinkGuide();
}

void BsMain::netServerStarted(const qint64 licDateEpochSecs)
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(licDateEpochSecs);
    mpBtnNet->setIcon(QIcon(":/icon/serveron.png"));
    mpBtnNet->setText(QStringLiteral("后台服务中"));
    mpBtnNet->setToolTip(QStringLiteral("服务到期日：%1").arg(dt.toString("yyyy-MM-dd")));
}

void BsMain::netServerStopped()
{
    mpBtnNet->setIcon(QIcon(":/icon/serveroff.png"));
    mpBtnNet->setText(QStringLiteral("后台服务已停止"));
}

void BsMain::openFileInfo()
{
    if ( loginAsAdminOrBoss )
        QMessageBox::information(this, QString(), QStringLiteral("本账册数据文件为：\n%1\n请妥善保管备份。").arg(loginFile));
    else
        QMessageBox::information(this, QString(), loginBook);
}

void BsMain::openSetPassword()
{
    if ( !dogOk ) {
        QMessageBox::information(this, QString(), QStringLiteral("试用体验版没有密码设置功能。"));
        return;
    }
    BsSetPassword dlg(loginer, loginPassword, this);
    if ( dlg.exec() == QDialog::Accepted ) {
        QString sql = QStringLiteral("update baililoginer set deskpassword='%1' where loginer='%2';")
                .arg(dlg.mpNewWord->text()).arg(loginer);
        QSqlQuery qry;
        qry.exec(sql);
        if ( qry.lastError().isValid() ) {
            QMessageBox::information(this, QString(), QStringLiteral("更换密码不成功！"));
        }
        else {
            QMessageBox::information(this, QString(), QStringLiteral("更换密码成功，重新登录生效！"));
            loginPassword = dlg.mpNewWord->text();
        }
    }
}

void BsMain::openSetLoginer()
{
    BsSetLoginer dlg(this);
    dlg.exec();

    //太难准确判断是否有变化，故一律更新。
    if ( mpServer->isRunning() ) {
        BsFronterMap::loadUpdate();
    }
}

void BsMain::openSetBarcodeRule()
{
    if ( ! questionCloseAllSubWin() )
        return;

    if ( ! checkRaiseSubWin("barcoderule") ) {
        QStringList flds;
        flds << QStringLiteral("barcodexp")
             << QStringLiteral("sizermiddlee")
             << QStringLiteral("barcodemark")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("barcoderule"), flds));
    }
}

void BsMain::openSetSystemOption()
{
    if ( ! questionCloseAllSubWin() )
        return;

    BsOption dlg(this);
    dlg.setWindowTitle(mapMsg.value("win_setsystem").split(QChar(9)).at(0));
    if ( dlg.exec() != QDialog::Accepted ) {
        return;
    }

    //更新网络服务保密码
    if ( !dogNetName.isEmpty()
         && !dogNetPass.isEmpty()
         && dlg.netEncryptionKeyUpdated
         && mpServer->isRunning() ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_require_restart_net_server"));
    }

    if ( dlg.bossNameChanged ) {
        QMessageBox::information(this, QString(), QStringLiteral("您更换了全权账号，需要退出重启！"));
        close();
    }
}

void BsMain::openSetNetService()
{
    if ( dogOk ) {
        if ( ! checkRaiseSubWin("set_service") ) {
            addNewSubWin(new BsSetService(this, mpServer));
        }
    }
    else {
        QMessageBox::information(this, QString(), QStringLiteral("试用版不能运行此功能！"));
    }
}

void BsMain::openRegSizerType()
{
    if ( ! checkRaiseSubWin("sizertype") ) {
        QStringList flds;
        flds << QStringLiteral("tname")
             << QStringLiteral("namelist")
             << QStringLiteral("codelist")
             << QStringLiteral("beforecolor")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("sizertype"), flds));
    }
}

void BsMain::openRegColorType()
{
    if ( ! checkRaiseSubWin("colortype") ) {
        QStringList flds;
        flds << QStringLiteral("tname")
             << QStringLiteral("namelist")
             << QStringLiteral("codelist")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("colortype"), flds));
    }
}

void BsMain::openRegCargo()
{
    if ( ! checkRaiseSubWin("cargo") ) {
        QStringList flds;
        flds << QStringLiteral("hpcode")
             << QStringLiteral("hpname")
             << QStringLiteral("sizertype")
             << QStringLiteral("colortype")
             << QStringLiteral("unit")
             << QStringLiteral("setprice")
             << QStringLiteral("retprice")
             << QStringLiteral("lotprice")
             << QStringLiteral("buyprice")
             << QStringLiteral("amtag")
             << QStringLiteral("almin")
             << QStringLiteral("almax");
        for ( int i = 1; i <= 6; ++i ) {
            QString attrx = QStringLiteral("attr%1").arg(i);
            QString optkey = QStringLiteral("cargo_attr%1_name").arg(i);
            if ( !mapOption.value(optkey).trimmed().isEmpty() ) {
                flds << attrx;
            }
        }
        flds << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("cargo"), flds));
    }
}

void BsMain::openRegStaff()
{
    if ( ! questionCloseAllSubWin() )
        return;

    if ( ! checkRaiseSubWin("staff") ) {
        QStringList flds;
        flds << QStringLiteral("kname")
             << QStringLiteral("cancg")
             << QStringLiteral("canpf")
             << QStringLiteral("canls")
             << QStringLiteral("candb")
             << QStringLiteral("cansy")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("staff"), flds));
    }
}

void BsMain::openRegSubject()
{
    if ( ! checkRaiseSubWin("subject") ) {
        QStringList flds;
        flds << QStringLiteral("kname");
        for ( int i = 1; i <= 6; ++i ) {
            QString attrx = QStringLiteral("attr%1").arg(i);
            QString optkey = QStringLiteral("subject_attr%1_name").arg(i);
            if ( !mapOption.value(optkey).trimmed().isEmpty() ) {
                flds << attrx;
            }
        }
        flds << QStringLiteral("refsheetin")
             << QStringLiteral("refsheetex")
             << QStringLiteral("adminboss")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("subject"), flds));
    }
}

void BsMain::openRegShop()
{
    if ( ! checkRaiseSubWin("shop") ) {
        QStringList flds;
        flds << QStringLiteral("kname")
             << QStringLiteral("regdis")
             << QStringLiteral("regman")
             << QStringLiteral("regtele")
             << QStringLiteral("regaddr")
             << QStringLiteral("amgeo")
             << QStringLiteral("regmark")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("shop"), flds));
    }
}

void BsMain::openRegSupplier()
{
    if ( ! checkRaiseSubWin("supplier") ) {
        QStringList flds;
        flds << QStringLiteral("kname")
             << QStringLiteral("regdis")
             << QStringLiteral("regman")
             << QStringLiteral("regtele")
             << QStringLiteral("regaddr")
             << QStringLiteral("regmark")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("supplier"), flds));
    }
}

void BsMain::openRegCustomer()
{
    if ( ! checkRaiseSubWin("customer") ) {
        QStringList flds;
        flds << QStringLiteral("kname")
             << QStringLiteral("regdis")
             << QStringLiteral("regman")
             << QStringLiteral("regtele")
             << QStringLiteral("regaddr")
             << QStringLiteral("regmark")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("customer"), flds));
    }
}

void BsMain::openRegPolicy()
{
    if ( ! checkRaiseSubWin("lotpolicy") ) { //名为lot，但也包含ret。系统不设计进货于调拨价格政策。
        QStringList flds;
        flds << QStringLiteral("policyname")
             << QStringLiteral("traderexp")
             << QStringLiteral("cargoexp")
             << QStringLiteral("policydis")
             << QStringLiteral("uselevel")
             << QStringLiteral("startdate")
             << QStringLiteral("enddate")
             << QStringLiteral("upman")
             << QStringLiteral("uptime");
        addNewSubWin(new BsRegWin(this, QStringLiteral("lotpolicy"), flds));
    }
}

void BsMain::openSheetCGD()
{
    if ( ! checkRaiseSubWin("cgd") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("cgd"), cargoSheetCommonFields));
    }
}

void BsMain::openSheetCGJ()
{
    if ( ! checkRaiseSubWin("cgj") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("cgj"), cargoSheetCommonFields));
    }
}

void BsMain::openSheetCGT()
{
    if ( ! checkRaiseSubWin("cgt") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("cgt"), cargoSheetCommonFields));
    }
}

void BsMain::openQryVICGD()
{
    if ( ! checkRaiseSubWin("vi_cgd") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_cgd"), cargoQueryCommonFields, bsqtSumSheet | bsqtSumOrder));
    }
}

void BsMain::openQryVICGJ()
{
    if ( ! checkRaiseSubWin("vi_cgj") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_cgj"), cargoQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryVICGT()
{
    if ( ! checkRaiseSubWin("vi_cgt") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_cgt"), cargoQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryViCgRest()
{
    if ( ! checkRaiseSubWin("vi_cg_rest") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_cg_rest"), cargoQueryCommonFields, bsqtSumOrder | bsqtSumMinus | bsqtSumRest));
    }
}

void BsMain::openQryViCgNeat()
{
    if ( ! checkRaiseSubWin("vi_cg") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_cg"), cargoQueryCommonFields, bsqtSumMinus));
    }
}

void BsMain::openQryViCgCash()
{
    if ( ! checkRaiseSubWin("vi_cg_cash") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_cg_cash"), cargoQueryCommonFields, bsqtSumCash | bsqtSumMinus));
    }
}

void BsMain::openSheetPFD()
{
    if ( ! checkRaiseSubWin("pfd") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("pfd"), cargoSheetCommonFields));
    }
}

void BsMain::openSheetPFF()
{
    if ( ! checkRaiseSubWin("pff") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("pff"), cargoSheetCommonFields));
    }
}

void BsMain::openSheetPFT()
{
    if ( ! checkRaiseSubWin("pft") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("pft"), cargoSheetCommonFields));
    }
}

void BsMain::openSheetLSD()
{
    if ( ! checkRaiseSubWin("lsd") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("lsd"), cargoSheetCommonFields));
    }
}

void BsMain::openQryVIPFD()
{
    if ( ! checkRaiseSubWin("vi_pfd") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_pfd"), cargoQueryCommonFields, bsqtSumSheet | bsqtSumOrder));
    }
}

void BsMain::openQryVIPFF()
{
    if ( ! checkRaiseSubWin("vi_pff") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_pff"), cargoQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryVIPFT()
{
    if ( ! checkRaiseSubWin("vi_pft") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_pft"), cargoQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryVILSD()
{
    if ( ! checkRaiseSubWin("vi_lsd") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_lsd"), cargoQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryViPfRest()
{
    if ( ! checkRaiseSubWin("vi_pf_rest") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_pf_rest"), cargoQueryCommonFields, bsqtSumMinus | bsqtSumOrder | bsqtSumRest));
    }
}

void BsMain::openQryViPfNeat()
{
    if ( ! checkRaiseSubWin("vi_pf") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_pf"), cargoQueryCommonFields, bsqtSumMinus));
    }
}

void BsMain::openQryViPfCash()
{
    if ( ! checkRaiseSubWin("vi_pf_cash") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_pf_cash"), cargoQueryCommonFields, bsqtSumCash | bsqtSumMinus));
    }
}

void BsMain::openQryViXsNeat()
{
    if ( ! checkRaiseSubWin("vi_xs") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_xs"), cargoQueryCommonFields, bsqtSumMinus));
    }
}

void BsMain::openQryViXsCash()
{
    if ( ! checkRaiseSubWin("vi_xs_cash") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_xs_cash"), cargoQueryCommonFields, bsqtSumCash | bsqtSumMinus));
    }
}

void BsMain::openSheetDBD()
{
    if ( ! checkRaiseSubWin("dbd") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("dbd"), cargoSheetCommonFields));
    }
}

void BsMain::openSheetSYD()
{
    if ( ! checkRaiseSubWin("syd") ) {
        addNewSubWin(new BsSheetCargoWin(this, QStringLiteral("syd"), cargoSheetCommonFields));
    }
}

void BsMain::openSheetSZD()
{
    if ( ! checkRaiseSubWin("szd") ) {
        addNewSubWin(new BsSheetFinanceWin(this, financeSheetCommonFields));
    }
}

void BsMain::openQryVIDBD()
{
    if ( ! checkRaiseSubWin("vi_dbd") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_dbd"), cargoQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryVISYD()
{
    if ( ! checkRaiseSubWin("vi_syd") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_syd"), cargoQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryVISZD()
{
    if ( ! checkRaiseSubWin("vi_szd") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_szd"), financeQueryCommonFields, bsqtSumSheet));
    }
}

void BsMain::openQryViStock()
{
    if ( ! checkRaiseSubWin("vi_stock") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_stock"), cargoQueryCommonFields, bsqtSumMinus | bsqtSumRest | bsqtSumStock));
    }
}

void BsMain::openQryViewAll()
{
    if ( ! checkRaiseSubWin("vi_all") ) {
        addNewSubWin(new BsQryWin(this, QStringLiteral("vi_all"), cargoQueryCommonFields, bsqtSumSheet | bsqtSumStock));
    }
}

void BsMain::openQryMinAlarm()
{
    openQryAlarm(true);
}

void BsMain::openQryMaxAlarm()
{
    openQryAlarm(false);
}

void BsMain::openToolBatchEdit()
{
    QAction *act = qobject_cast<QAction*>(QObject::sender());
    Q_ASSERT(act);
    BsBatchRename dlg(this);
    dlg.setWindowTitle(act->text());
    dlg.setMinimumSize(dlg.sizeHint());
    dlg.exec();
}

void BsMain::openToolBatchCheck()
{
    if ( ! loginAsAdminOrBoss ) {
        QMessageBox::information(this, QString(), QStringLiteral("没有权限！"));
        return;
    }
    QAction *act = qobject_cast<QAction*>(QObject::sender());
    Q_ASSERT(act);
    BsBatchReCheck dlg(this);
    dlg.setWindowTitle(act->text());
    dlg.setMinimumSize(dlg.sizeHint());
    dlg.exec();
}

void BsMain::openToolStockReset()
{
    QAction *act = qobject_cast<QAction*>(QObject::sender());
    Q_ASSERT(act);
    BsToolStockReset dlg(this);
    dlg.setWindowTitle(act->text());
    dlg.setMinimumSize(dlg.sizeHint());
    dlg.exec();
}

void BsMain::openToolBarcodeMaker()
{
    if ( ! checkRaiseSubWin("tool_barcodemaker") ) {
        addNewSubWin(new BsBarcodeMaker(this, QStringLiteral("tool_barcodemaker")));
    }
}

void BsMain::openToolLabelDesigner()
{
    if ( ! checkRaiseSubWin("tool_labeldesigner") ) {
        addNewSubWin(new BsLabelDesigner(this, QStringLiteral("tool_labeldesigner")));
    }
}

void BsMain::openHelpManual()
{
    QDesktopServices::openUrl(QUrl("https://www.bailisoft.com/passage/jyb_index.html"));
}

void BsMain::openHelpSite()
{
    QDesktopServices::openUrl(QUrl("https://www.bailisoft.com/"));
}

void BsMain::openHelpAbout()
{
    BsAbout *w = new BsAbout(this);
    w->show();
    w->setGeometry((mpMdi->width() - w->width()) / 2, (mpMdi->height() - w->height()) / 2, w->width(), w->height());
}

void BsMain::responseOpenSheet(const QString &sheetName, const int sheetId)
{
    if ( ! checkRaiseSubWin(sheetName) ) {
        addNewSubWin(new BsSheetCargoWin(this, sheetName, cargoSheetCommonFields));
    }
    QWidget *pwin = getTableWin(sheetName);
    BsAbstractSheetWin *sheetWin = qobject_cast<BsAbstractSheetWin*>(pwin);
    if ( sheetWin ) {

        if ( sheetWin->isEditing() ) {
            QMessageBox::information(this, QString(), QStringLiteral("单据编辑中，尚未保存或取消，不能打开单据。"));
            return;
        }

        sheetWin->openSheet(sheetId);
    }
}

int BsMain::getTotalMenuWidth()
{
    int totalMenuWidth = 1;
    QRect actRect;
    for ( int i = 0, iLen = menuBar()->actions().length(); i < iLen; ++i )
    {
        QAction *act = menuBar()->actions().at(i);
        actRect = menuBar()->actionGeometry(act);
        if ( actRect.x() + actRect.width() > totalMenuWidth )
            totalMenuWidth = actRect.x() + actRect.width();
    }
    return totalMenuWidth;
}

bool BsMain::checkRaiseSubWin(const QString &winTable)
{
    for ( int i = 0, iLen = mpMdi->subWindowList().size(); i < iLen; ++i ) {
        QMdiSubWindow *sw = mpMdi->subWindowList().at(i);
        if ( mpMdi->subWindowList().at(i)->widget()->property(BSWIN_TABLE).toString() == winTable ) {
            sw->raise();
            sw->setFocus();
            return true;
        }
    }
    return false;
}

void BsMain::addNewSubWin(QWidget *win, const bool setCenterr)
{
    win->installEventFilter(this);
    QMdiSubWindow *subWin = mpMdi->addSubWindow(win);
    win->showNormal();

    if ( setCenterr ) {
        int w = subWin->width();
        int h = subWin->height();
        int ww = width();
        int wh = height();
        subWin->setGeometry((ww - w) / 2, (wh - h) / 2, w, h);
    }

    //库存同步
    BsAbstractSheetWin *sheet = qobject_cast<BsAbstractSheetWin*>(win);
    if ( sheet ) {
        QString table = sheet->mMainTable;
        if ( table == QStringLiteral("cgj") ||
             table == QStringLiteral("cgt") ||
             table == QStringLiteral("pff") ||
             table == QStringLiteral("pft") ||
             table == QStringLiteral("lsd") ||
             table == QStringLiteral("dbd") ||
             table == QStringLiteral("syd") ) {
            connect(sheet, &BsAbstractSheetWin::shopStockChanged, mpSentinel, &BsPublisher::addJob);
        }
    }
}

void BsMain::closeAllSubWin()
{
    for ( int i = mpMdi->subWindowList().size() - 1; i >= 0; --i )
    {
        QWidget *w = qobject_cast<QWidget *>(mpMdi->subWindowList().at(i)->widget());
        if ( w ) {
            w->close();
            mpMdi->subWindowList().at(i)->close();
        }
    }
}

QWidget *BsMain::getTableWin(const QString &sheetName)
{
    for ( int i = 0, iLen = mpMdi->subWindowList().size(); i < iLen; ++i ) {
        QMdiSubWindow *sw = mpMdi->subWindowList().at(i);
        if ( mpMdi->subWindowList().at(i)->widget()->property(BSWIN_TABLE).toString() == sheetName ) {
            sw->raise();
            return mpMdi->subWindowList().at(i)->widget();
        }
    }
    return nullptr;
}

bool BsMain::questionCloseAllSubWin()
{
    bool allClean = true;
    for ( int i = mpMdi->subWindowList().size() - 1; i >= 0; --i )
    {
        QWidget *w = qobject_cast<QWidget *>(mpMdi->subWindowList().at(i)->widget());
        if ( w ) {
            BsWin *bsWin = qobject_cast<BsWin*>(w);
            if ( bsWin ) {
                if ( bsWin->isEditing() ) {
                    allClean = false;
                    break;
                }
            }
        }
    }

    if ( ! allClean ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_found_editing_win"));
        return false;
    }

    if ( mpMdi->subWindowList().size() > 0 ) {
        if ( confirmDialog(this,
                             mapMsg.value("i_need_close_other_wins_first"),
                             mapMsg.value("i_close_all_other_win_now"),
                             mapMsg.value("btn_ok"),
                             mapMsg.value("btn_cancel"),
                             QMessageBox::Warning) )
            closeAllSubWin();
        else
            return false;
    }

    return true;
}

void BsMain::openQryAlarm(const bool isMinType)
{
    QString winClass = ( isMinType ) ? QStringLiteral("BsMinAlarmReport") : QStringLiteral("BsMaxAlarmReport");
    QString winTitle = ( isMinType ) ? mapMsg.value("win_min_alarm") : mapMsg.value("win_max_alarm");

    QDockWidget *dockWidget = nullptr;
    QList<QDockWidget *> docks = findChildren<QDockWidget *>();
    for ( int i = 0, iLen = docks.length(); i < iLen; ++i ) {
        QDockWidget *dock = docks.at(i);
        if ( QString(dock->widget()->metaObject()->className()).endsWith(winClass) ) {  //前有namespace
            dockWidget = dock;
            break;
        }
    }

    if ( !dockWidget ) {
        dockWidget = new QDockWidget(winTitle, this);
        dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        if ( isMinType )
            dockWidget->setWidget(new BsMinAlarmReport(this));
        else
            dockWidget->setWidget(new BsMaxAlarmReport(this));
        addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    }

    dockWidget->setFloating(true);
    int w = dockWidget->widget()->sizeHint().width() + 10;
    dockWidget->resize(w, mpMdi->height() / 2);
    QPoint pt = mpMdi->mapToGlobal(QPoint(0, 0));
    int delta = ( isMinType ) ? 50 : 0;
    dockWidget->setGeometry(pt.x() + (mpMdi->width() - w) / 2 - 2 * delta,
                            pt.y() + mpMdi->height() / 4 - delta,
                            w,
                            mpMdi->height() / 2);
    dockWidget->show();
}

void BsMain::setMenuAllowable()
{
    //以下须与lstRegisWinTableNames、lstSheetWinTableNames、lstQueryWinTableNames三变量一致

    mpMenuSetLoginer->setEnabled(loginAsAdminOrBoss);
    mpMenuSetBarcodeRule->setEnabled(loginAsAdminOrBoss);
    mpMenuSetSystemOption->setEnabled(loginAsAdminOrBoss);
    mpMenuSetNetService->setEnabled(loginAsAdminOrBoss &&
                                    !loginFile.contains("//") &&
                                    !loginFile.contains("\\\\"));

    mpMenuSizerType->setEnabled(canDo("sizertype"));
    mpMenuColorType->setEnabled(canDo("colortype"));
    mpMenuCargo->setEnabled(canDo("cargo"));
    mpMenuStaff->setEnabled(canDo("staff"));
    mpMenuShop->setEnabled(canDo("shop"));
    mpMenuSupplier->setEnabled(canDo("supplier"));
    mpMenuCustomer->setEnabled(canDo("customer"));

    mpMenuSheetCGD->setEnabled( canDo("cgd"));
    mpMenuSheetCGJ->setEnabled( canDo("cgj"));
    mpMenuSheetCGT->setEnabled( canDo("cgt"));
    mpMenuQryVICGD->setEnabled( canDo("vicgd"));
    mpMenuQryVICGJ->setEnabled( canDo("vicgj"));
    mpMenuQryVICGT->setEnabled( canDo("vicgt"));
    mpMenuQryVIYCG->setEnabled( canDo("vicgrest"));
    mpMenuQryVIJCG->setEnabled( canDo("vicg"));
    mpMenuQryVIJCGM->setEnabled(canDo("vicgcash"));

    mpMenuSheetPFD->setEnabled( canDo("pfd"));
    mpMenuSheetPFF->setEnabled( canDo("pff"));
    mpMenuSheetPFT->setEnabled( canDo("pft"));
    mpMenuSheetLSD->setEnabled( canDo("lsd"));
    mpMenuQryVIPFD->setEnabled( canDo("vipfd"));
    mpMenuQryVIPFF->setEnabled( canDo("vipff"));
    mpMenuQryVIPFT->setEnabled( canDo("vipft"));
    mpMenuQryVILSD->setEnabled( canDo("vilsd"));
    mpMenuQryVIYPF->setEnabled( canDo("vipfrest"));
    mpMenuQryVIJPF->setEnabled( canDo("vipf"));
    mpMenuQryVIJPFM->setEnabled(canDo("vipfcash"));
    mpMenuQryVIJXS->setEnabled( canDo("vixs"));
    mpMenuQryVIJXSM->setEnabled(canDo("vixscash"));

    mpMenuSheetDBD->setEnabled(canDo("dbd"));
    mpMenuSheetSYD->setEnabled(canDo("syd"));
    mpMenuQryVIDBD->setEnabled(canDo("vidbd"));
    mpMenuQryVISYD->setEnabled(canDo("visyd"));
    mpMenuQryVIYKC->setEnabled(canDo("vistock"));
    mpMenuQryVIALL->setEnabled(canDo("viall"));

    mpMenuSubject->setEnabled(loginAsBoss);
    mpMenuSheetSZD->setEnabled(loginAsBoss);
    mpMenuQryVISZD->setEnabled(loginAsBoss);
}

void BsMain::updateSheetDateForMemo()
{
    QString sql;
    QSqlQuery qry;

    sql = QString("select max(dated) from lsd;");
    qry.exec(sql);
    qry.next();
    qint64 maxLsdDate  = qry.value(0).toLongLong();

    sql = QString("select max(uptime) from lsd;");
    qry.exec(sql);
    qry.next();

    qint64 secsDiff = QDateTime(QDate::currentDate()).toMSecsSinceEpoch() / 1000 - maxLsdDate;

    QStringList sqls;
    sqls << QStringLiteral("update cgd set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update cgj set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update cgt set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update pfd set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update pff set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update pft set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update lsd set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update dbd set dated=dated+%1;").arg(secsDiff)
         << QStringLiteral("update syd set dated=dated+%1;").arg(secsDiff);

    sqls << QStringLiteral("update cgd set "
                           "sumqty=(select sum(qty) from cgddtl where parentid=cgd.sheetid), "
                           "summoney=(select sum(actmoney) from cgddtl where parentid=cgd.sheetid), "
                           "sumdis=(select sum(dismoney) from cgddtl where parentid=cgd.sheetid) "
                           "where sheetid=(select parentid from cgddtl where parentid=cgd.sheetid);")
         << QStringLiteral("update cgj set "
                           "sumqty=(select sum(qty) from cgjdtl where parentid=cgj.sheetid), "
                           "summoney=(select sum(actmoney) from cgjdtl where parentid=cgj.sheetid), "
                           "sumdis=(select sum(dismoney) from cgjdtl where parentid=cgj.sheetid) "
                           "where sheetid=(select parentid from cgjdtl where parentid=cgj.sheetid);")
         << QStringLiteral("update cgt set "
                           "sumqty=(select sum(qty) from cgtdtl where parentid=cgt.sheetid), "
                           "summoney=(select sum(actmoney) from cgtdtl where parentid=cgt.sheetid), "
                           "sumdis=(select sum(dismoney) from cgtdtl where parentid=cgt.sheetid) "
                           "where sheetid=(select parentid from cgtdtl where parentid=cgt.sheetid);")
         << QStringLiteral("update pfd set "
                           "sumqty=(select sum(qty) from pfddtl where parentid=pfd.sheetid), "
                           "summoney=(select sum(actmoney) from pfddtl where parentid=pfd.sheetid), "
                           "sumdis=(select sum(dismoney) from pfddtl where parentid=pfd.sheetid) "
                           "where sheetid=(select parentid from pfddtl where parentid=pfd.sheetid);")
         << QStringLiteral("update pff set "
                           "sumqty=(select sum(qty) from pffdtl where parentid=pff.sheetid), "
                           "summoney=(select sum(actmoney) from pffdtl where parentid=pff.sheetid), "
                           "sumdis=(select sum(dismoney) from pffdtl where parentid=pff.sheetid) "
                           "where sheetid=(select parentid from pffdtl where parentid=pff.sheetid);")
         << QStringLiteral("update pft set "
                           "sumqty=(select sum(qty) from pftdtl where parentid=pft.sheetid), "
                           "summoney=(select sum(actmoney) from pftdtl where parentid=pft.sheetid), "
                           "sumdis=(select sum(dismoney) from pftdtl where parentid=pft.sheetid) "
                           "where sheetid=(select parentid from pftdtl where parentid=pft.sheetid);")
         << QStringLiteral("update lsd set "
                           "sumqty=(select sum(qty) from lsddtl where parentid=lsd.sheetid), "
                           "summoney=(select sum(actmoney) from lsddtl where parentid=lsd.sheetid), "
                           "sumdis=(select sum(dismoney) from lsddtl where parentid=lsd.sheetid) "
                           "where sheetid=(select parentid from lsddtl where parentid=lsd.sheetid);")
         << QStringLiteral("update dbd set "
                           "sumqty=(select sum(qty) from dbddtl where parentid=dbd.sheetid), "
                           "summoney=(select sum(actmoney) from dbddtl where parentid=dbd.sheetid), "
                           "sumdis=(select sum(dismoney) from dbddtl where parentid=dbd.sheetid) "
                           "where sheetid=(select parentid from dbddtl where parentid=dbd.sheetid);")
         << QStringLiteral("update syd set "
                           "sumqty=(select sum(qty) from syddtl where parentid=syd.sheetid), "
                           "summoney=(select sum(actmoney) from syddtl where parentid=syd.sheetid), "
                           "sumdis=(select sum(dismoney) from syddtl where parentid=syd.sheetid) "
                           "where sheetid=(select parentid from syddtl where parentid=syd.sheetid);");

    sqls << QStringLiteral("update cgd set actowe=summoney-actpay;")
         << QStringLiteral("update cgj set actowe=summoney-actpay;")
         << QStringLiteral("update cgt set actowe=summoney-actpay;")
         << QStringLiteral("update pfd set actowe=summoney-actpay;")
         << QStringLiteral("update pff set actowe=summoney-actpay;")
         << QStringLiteral("update pft set actowe=summoney-actpay;")
         << QStringLiteral("update lsd set actowe=summoney-actpay;")
         << QStringLiteral("update dbd set actowe=summoney-actpay;")
         << QStringLiteral("update syd set actowe=summoney-actpay;");

    QString sqlErr = sqliteCommit(sqls);

    if ( sqlErr.isEmpty() ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_update_demo_book_date"));
    } else {
        qDebug() << sqlErr;
    }
}

}
