#include "bsloginguide.h"
#include "bslinkbookdlg.h"
#include "main/bailicode.h"
#include "main/bailicustom.h"
#include "main/bailidata.h"
#include "main/bailifunc.h"
#include "main/bailisql.h"
#include "misc/bsimportr15dlg.h"
#include "misc/bsimportr16dlg.h"

#include <QtSql/QSqlDatabase>

#define MN_NEW_BOOK     "新建账册"
#define MN_OPEN_BOOK    "打开账册"
#define MN_DROP_BOOK    "删除账册"
#define MN_DROP_LINK    "删除连接"
#define MN_LINK_BOOK    "连接网络后台"
#define MN_MANAGE       "配置管理"

//【账册信息QSettings记录约定】
//总部后端账册————文件全路径名，必然含有斜杠字符
//前端网络连接————前端登录名逗号列表

namespace BailiSoft {

BsLoginGuide::BsLoginGuide(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
    mpImageSide = new QWidget(this);
    mpImageSide->setFixedSize(160, 280);
    mpImageSide->setStyleSheet(QStringLiteral("background:url(:/image/login.png);"));

    QLabel *lblSelect = new QLabel(QStringLiteral("选择账册："));

    QMenu *mnManage = new QMenu(this);

    mnManage->addAction(QStringLiteral(MN_NEW_BOOK), this, SLOT(doCreateBook()));
    mnManage->addAction(QStringLiteral(MN_OPEN_BOOK), this, SLOT(doOpenBook()));
    mpAcDelBook = mnManage->addAction(QStringLiteral(MN_DROP_BOOK), this, SLOT(doDelBook()));

    if ( dogOk ) {
        mnManage->addSeparator();
        mnManage->addAction(QStringLiteral("导入R15账册"), this, SLOT(importR15Book()));
        mnManage->addAction(QStringLiteral("导入R16账册"), this, SLOT(importR16Book()));
    }
    mnManage->addSeparator();
    mnManage->addAction(QStringLiteral("帮助"), this, SLOT(doHelp()));

    QToolButton *btnManage = new QToolButton(this);
    btnManage->setText(QStringLiteral(MN_MANAGE));
    btnManage->setIcon(QIcon(QLatin1String(":/icon/config.png")));
    btnManage->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnManage->setMenu(mnManage);
    btnManage->setPopupMode(QToolButton::InstantPopup);
    btnManage->setAutoRaise(true);

    QHBoxLayout *layMenu = new QHBoxLayout;
    layMenu->addWidget(lblSelect, 0, Qt::AlignBottom);
    layMenu->addStretch();
    layMenu->addWidget(btnManage);

    mpBooks = new QListWidget(this);
    mpBooks->setStyleSheet("QAbstractItemView::item {min-height: 24px;}");

    QWidget *tabBook = new QWidget(this);
    QVBoxLayout *layBook = new QVBoxLayout;
    layBook->addLayout(layMenu);
    layBook->addWidget(mpBooks);
    layBook->setContentsMargins(0, 0, 0, 0);
    tabBook->setLayout(layBook);

    QLabel *lblUsers = new QLabel(QStringLiteral("账号："));
    mpUsers = new QComboBox(this);
    mpUsers->setItemDelegate(new QStyledItemDelegate()); //为使下面item的样式生效，必须如此
    mpUsers->setStyleSheet("QComboBox { min-height:20px; }"
                           "QComboBox QAbstractItemView::item { min-height: 20px; }");

    QLabel *lblPassword = new QLabel(QStringLiteral("密码："));
    mpPassword = new QLineEdit(this);
    disableEditInputMethod(mpPassword);  //密码不能有输入法，输入法太不安全。
    mpPassword->setEchoMode(QLineEdit::Password);
    mpPassword->setMinimumHeight(24);

    QWidget *tabLogin = new QWidget(this);
    QVBoxLayout *layLogin = new QVBoxLayout;
    layLogin->addStretch(1);
    layLogin->addWidget(lblUsers);
    layLogin->addWidget(mpUsers);
    layLogin->addSpacing(20);
    layLogin->addWidget(lblPassword);
    layLogin->addWidget(mpPassword);
    layLogin->addStretch(1);
    layLogin->setSpacing(5);
    layLogin->setContentsMargins(40, 0, 40, 0);
    tabLogin->setLayout(layLogin);

    mpStack = new QStackedLayout;
    mpStack->setContentsMargins(0, 0, 0, 0);
    mpStack->addWidget(tabBook);
    mpStack->addWidget(tabLogin);

    mpPrev = new QPushButton(QStringLiteral("<"));
    mpPrev->setFixedSize(32, 32);

    mpNext = new QPushButton(QStringLiteral("下一步"));
    mpNext->setFixedSize(120, 32);
    mpNext->setDefault(true);    

    mpCancel = new QPushButton(QStringLiteral("取消"));
    mpCancel->setFixedSize(120, 32);

    mpLayDlgBtns = new QHBoxLayout;
    mpLayDlgBtns->addStretch(1);
    mpLayDlgBtns->addWidget(mpPrev);
    mpLayDlgBtns->addWidget(mpNext);
    mpLayDlgBtns->addWidget(mpCancel);
    mpLayDlgBtns->addStretch(1);
    mpLayDlgBtns->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layWork = new QVBoxLayout;
    layWork->addLayout(mpStack, 1);
    layWork->addLayout(mpLayDlgBtns);
    layWork->setContentsMargins(0, 3, 10, 10);

    QHBoxLayout *layMain = new QHBoxLayout(this);
    layMain->addWidget(mpImageSide);
    layMain->addLayout(layWork, 1);
    layMain->setContentsMargins(0, 0, 0, 0);

    setFixedSize(sizeHint());
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(QStringLiteral("登录向导"));

#ifdef Q_OS_MAC
    QString btnStyle = "QPushButton{border:1px solid #999; border:6px; background-color:"
            "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f8f8f8, stop:1 #dddddd);} "
            "QPushButton:pressed{background-color:qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "stop:0 #dddddd, stop:1 #f8f8f8);}";
    mpPrev->setStyleSheet(btnStyle);
    mpNext->setStyleSheet(btnStyle);
    mpCancel->setStyleSheet(btnStyle);
#endif

    //版本名称
    mpVerName = new QLabel(this);
    mpVerName->setText(versionLicenseName);
    mpVerName->setAlignment(Qt::AlignCenter);
    mpVerName->setStyleSheet("color:white; font-weight:900;");

    if ( ! dogOk ) {
        mpPassword->setPlaceholderText(QStringLiteral("试用体验版无需密码"));
        mpPassword->setEnabled(false);
    }

    //加载账册
    reloadBookList();

    //信号
    connect(mpPrev, SIGNAL(clicked(bool)), this, SLOT(doPrev()));
    connect(mpNext, SIGNAL(clicked(bool)), this, SLOT(doNext()));
    connect(mpCancel, SIGNAL(clicked(bool)), this, SLOT(reject()));
    connect(mpBooks, SIGNAL(currentRowChanged(int)), this, SLOT(bookPicked(int)));
    connect(mpBooks, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(bookDoubleClicked(QListWidgetItem*)));
}

void BsLoginGuide::setVisible(bool visible)
{
    QDialog::setVisible(visible);
    if ( visible ) {
        mpBooks->clearSelection();
        mpAcDelBook->setEnabled(false);
        mpStack->setCurrentIndex(0);
        mpPrev->hide();
        mpNext->setEnabled(false);
        mpLayDlgBtns->setContentsMargins(0, 0, 0, 0);
    }
}

QString BsLoginGuide::getCurrentBook()
{
    return mpBooks->currentItem()->text();
}

void BsLoginGuide::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);
    mpVerName->setGeometry(0, 3 * height() / 5, mpImageSide->width(), 80);
}

void BsLoginGuide::bookPicked(int)
{
    mpNext->setEnabled( mpBooks->currentItem() );
    mpAcDelBook->setEnabled( mpBooks->currentItem() );
}

void BsLoginGuide::bookDoubleClicked(QListWidgetItem *item)
{
    if ( item ) {
        doNext();
    }
}

void BsLoginGuide::doPrev()
{
    mpStack->setCurrentIndex(0);
    mpPrev->hide();
    mpUsers->setEnabled(true);
    mpUsers->clear();
    mpNext->setText(QStringLiteral("下一步"));
    mpLayDlgBtns->setContentsMargins(0, 0, 0, 0);

    mpNext->setFixedSize(120, 32);
    mpCancel->setFixedSize(120, 32);
}

void BsLoginGuide::doNext()
{
    //加载用户
    if ( mpStack->currentIndex() == 0 ) {
        if ( mpBooks->currentItem() ) {

            //检查加载用户及关联信息
            QString strErr = loadUserInfo();

            //进入密码登录页面
            if ( strErr.isEmpty() ) {
                mpStack->setCurrentIndex(1);
                mpPrev->show();
                mpNext->setText(QStringLiteral("登录"));
                mpLayDlgBtns->setContentsMargins(0, 0, 0, 50);
                mpPassword->clear();
                mpPassword->setFocus();
                mpNext->setFixedSize(75, 32);
                mpCancel->setFixedSize(75, 32);
            }
            //报告错误
            else
                QMessageBox::information(this, QString(), strErr);
        }
    }
    //登录
    else {
        QStringList userInfoFields = mpUsers->currentData().toString().split("\n");
        if ( ( userInfoFields.at(0) == mpPassword->text() || QString(userInfoFields.at(0)).isEmpty() ) ) {
            loginer = mpUsers->currentText();
            loginShop = userInfoFields.at(1);
            loginPassword = mpPassword->text();
            loginAsBoss = (bossAccount == loginer);
            loginAsAdmin = (mapMsg.value("word_admin") == loginer);
            loginAsAdminOrBoss = (loginAsBoss || loginAsAdmin);
            accept();
        }
        else
            QMessageBox::information(this, QString(), QStringLiteral("密码验证未通过！"));
    }
}

void BsLoginGuide::doCreateBook()
{
    //让用户给账册取名
    QInputDialog dlg(this);
    dlg.setOkButtonText(mapMsg.value("btn_ok"));
    dlg.setCancelButtonText(mapMsg.value("btn_cancel"));
    dlg.setWindowTitle(QStringLiteral(MN_NEW_BOOK));
    dlg.setLabelText(QStringLiteral("账册名:"));
    dlg.setTextValue(QStringLiteral("某某账"));
    dlg.setWindowFlags(dlg.windowFlags() &~ Qt::WindowContextHelpButtonHint);
    dlg.adjustSize();
    if ( dlg.exec() != QDialog::Accepted ) return;

    //根据用户所取账册名，获取完整路径名，并判断该相同目录中是否已经有同名文件
    QDir dir(dataDir);
    QString fileName = dlg.textValue() + QStringLiteral(".jyb");
    QString bookFile = dir.absoluteFilePath(fileName);
    if ( dir.exists(fileName) ) {
        QMessageBox::information(this, QString(), QStringLiteral("该名称的文件已经存在！请重新取名、或移除原文件。"));
        return;
    }

    //调用创建过程
    QString strErr = createNewBook(dlg.textValue(), bookFile);
    QString strReport;

    //重载账册列表
    if ( strErr.isEmpty() ) {
        //记录
        QSettings settings;
        settings.beginGroup(BSR17BookList);
        settings.setValue(dlg.textValue(), bookFile);
        settings.endGroup();

        //刷新
        reloadBookList();

        //报告文字
        strReport = QStringLiteral("新账册创建成功！账册文件保存在文件“%1”中。").arg(bookFile);
    }
    else
        strReport = QStringLiteral("创建失败！错误原因：%1").arg(strErr);

    //报告
    QMessageBox::information(this, QString(), strReport);
}

void BsLoginGuide::doOpenBook()
{
    //打开文件
    QString openFile = QFileDialog::getOpenFileName(this, "百利R17账册数据文件", dataDir);
    if (openFile.isEmpty()) return;

    if ( !dogOk ) {
        if ( openFile.contains(QStringLiteral("//")) || openFile.contains(QStringLiteral("\\\\")) ) {
            QMessageBox::information(this, QString(), QStringLiteral("局域网共享方式登录账册需要每台电脑安装软件狗授权。"));
            return;
        }
    }

    QString sql = QStringLiteral("select vsetting from bailioption where optcode='app_book_name';");
    QString bookName = readValueFromSqliteFile(sql, openFile).toString();  //openFile.mid(openFile.lastIndexOf(QChar('/')) + 1);

    //检查重名
    for ( int i = 0, iLen = mpBooks->count(); i < iLen; ++i ) {
        if ( mpBooks->item(i)->text() == bookName ) {
            QMessageBox::information(this, QString(), QStringLiteral("该名称账册已经存在！"));
            return;
        }
    }

    //记录
    QSettings settings;
    settings.beginGroup(BSR17BookList);
    settings.setValue(bookName, openFile);
    settings.endGroup();

    //添加
    QListWidgetItem *itemNew = new QListWidgetItem(bookName, mpBooks);
    itemNew->setData(Qt::UserRole, openFile);
}

void BsLoginGuide::doDelBook()
{
    //账册信息
    QString bookName = mpBooks->currentItem()->text();
    QString tipMsg = QStringLiteral("此删除仅仅移除此处的账册名称。真正删除文件请在电脑文件夹中自行操作。");
    QString askMsg = QStringLiteral("确定要删除这个账册名称吗？");

    //提示
    if ( ! confirmDialog(this, tipMsg, askMsg,
                         mapMsg.value("btn_ok"),
                         mapMsg.value("btn_cancel"),
                         QMessageBox::Warning) )
        return;

    //记录
    QSettings settings;
    settings.beginGroup(BSR17BookList);
    settings.remove(bookName);
    settings.endGroup();
    settings.sync();

    //刷新
    reloadBookList();
}

void BsLoginGuide::importR15Book()
{
    //读取老账册文件
    QString mdbFile = QFileDialog::getOpenFileName(this, "打开百利R15账册文件", "D:/");
    if (mdbFile.isEmpty()) return;

    if ( ! BsImportR15Dlg::testR15Book(mdbFile) ) {
        QMessageBox::information(this, QString(), QStringLiteral("数据打开不成功，请确定是有效的R15账册文件！\n"));
        return;
    }

    BsImportR15Dlg dlg(this, mdbFile);
    if ( dlg.exec() == QDialog::Accepted ) {
        //记录
        QSettings settings;
        settings.beginGroup(BSR17BookList);
        settings.setValue(dlg.mBookName, dlg.mSqliteFile);
        settings.endGroup();

        //添加
        QListWidgetItem *itemNew = new QListWidgetItem(dlg.mBookName, mpBooks);
        itemNew->setData(Qt::UserRole, dlg.mSqliteFile);
    }
}

void BsLoginGuide::importR16Book()
{
    //读取老账册文件
    QString mdbFile = QFileDialog::getOpenFileName(this, "打开百利R16账册文件", "D:/");
    if (mdbFile.isEmpty()) return;

    BsImportR16Dlg dlg(this, mdbFile);
    if ( dlg.exec() == QDialog::Accepted ) {
        //记录
        QSettings settings;
        settings.beginGroup(BSR17BookList);
        settings.setValue(dlg.mBookName, dlg.mSqliteFile);
        settings.endGroup();

        //添加
        QListWidgetItem *itemNew = new QListWidgetItem(dlg.mBookName, mpBooks);
        itemNew->setData(Qt::UserRole, dlg.mSqliteFile);
    }
}

void BsLoginGuide::doHelp()
{
    QDesktopServices::openUrl(QUrl("https://www.bailisoft.com/passage/jyb_login_guide.html"));
}

QString BsLoginGuide::sqlInitBook(const QString &bookName, QSqlDatabase &sdb)
{
    QString strErr;

    //初始化新数据库表结构完整SQL
    QStringList sqls = sqliteInitSqls(bookName, false);

    //批处理
    sdb.transaction();
    foreach (QString sql, sqls) {
        if ( sql.trimmed().length() > 10 ) {
            sdb.exec(sql);
            if ( sdb.lastError().isValid() ) {
                strErr = QStringLiteral("%1\n%2").arg(sdb.lastError().text()).arg(sql);
                qDebug() << strErr;
                break;
            }
        }
    }

    //提交
    if ( strErr.isEmpty() )
        sdb.commit();
    else
        sdb.rollback();

    //返回
    return strErr;
}

QString BsLoginGuide::createNewBook(const QString &bookName, const QString &bookFile)
{
    //创建库文件并初始化为R18数据结构
    const QString tmp_sqlite_conn = QStringLiteral("tmp_sqlite_conn");
    QString strErr;
    {
        QSqlDatabase sdb = QSqlDatabase::addDatabase("QSQLITE", tmp_sqlite_conn);
        sdb.setDatabaseName(bookFile);
        if ( sdb.open() )
            strErr = sqlInitBook(bookName, sdb);
        else
            strErr = QStringLiteral("系统不支持Sqlite，%1创建不成功。错误信息：%2")
                    .arg(bookFile).arg(sdb.lastError().text());
    }

    //移除连接必须在addDatabase()作用域花括号外
    QSqlDatabase::removeDatabase(tmp_sqlite_conn);

    return strErr;
}

QString BsLoginGuide::calclatePassHash(const QString &pwd, const QString &salt)
{
    QString ptext = pwd + salt;
    return QCryptographicHash::hash(ptext.toLatin1(), QCryptographicHash::Sha256).toHex();
}

void BsLoginGuide::reloadBookList()
{
    mpBooks->clear();
    QSettings settings;
    settings.beginGroup(BSR17BookList);
    QStringList books = settings.childKeys();
    foreach (QString book, books) {
        //参见【账册信息QSettings记录约定】注释
        QString bookInfo = settings.value(book).toString();
        QListWidgetItem *item = new QListWidgetItem(book, mpBooks);
        item->setData(Qt::UserRole, bookInfo);
        item->setData(Qt::DecorationRole, QIcon(":/icon/book.png"));
    }
    settings.endGroup();
}

QString BsLoginGuide::loadUserInfo()
{
    //账册信息
    QString bookName = mpBooks->currentItem()->text();
    //QString bookInfo = mpBooks->currentItem()->data(Qt::UserRole).toString();

    //账册文件
    loginFile = mpBooks->currentItem()->data(Qt::UserRole).toString();
    if ( ! QFile::exists(loginFile) )
        return QStringLiteral("找不到账册文件%1").arg(loginFile);

    //更换默认主工作库
    QSqlDatabase defaultdb = QSqlDatabase::database();
    if ( defaultdb.isOpen() )
        defaultdb.close();
    defaultdb.setDatabaseName(loginFile);

    if ( ! defaultdb.open() )
        return QStringLiteral("无效或非法的数据库文件%1").arg(loginFile);

    //账册名可先设
    loginBook = bookName;

    //登录人先清除，密码验证通过后才设
    loginer.clear();
    loginShop.clear();

    //检查升级数据结构
    upgradeSqlCheck();

    //SQL ready
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);

    //总经理账号
    qry.exec(QStringLiteral("select vsetting from bailioption where optcode='app_boss_name';"));
    if ( qry.next() ) {
        bossAccount = qry.value(0).toString();
    } else {
        return QStringLiteral("账册文件无效！");
    }
    qry.exec(QStringLiteral("select loginer from baililoginer where loginer='%1';").arg(bossAccount));
    if ( !qry.next() ) {
        return QStringLiteral("账册文件数据无效！");
    }
    qry.exec(QStringLiteral("update baililoginer set passhash='boss' where loginer='%1' and passhash='';")
             .arg(bossAccount));

    //加载用户
    mpUsers->setToolTip(QString());
    mpUsers->clear();
    qry.exec(QStringLiteral("select loginer, deskpassword, bindshop from baililoginer "
                            "where loginer<>'%1' and loginer<>'%2' and length(passhash)=0 "
                            "order by loginer;")
             .arg(mapMsg.value("word_admin")).arg(bossAccount));
    if ( ! qry.lastError().isValid() ) {
        while ( qry.next() ) {
            QString loginerTxt = qry.value(0).toString();
            loginerTxt.replace(QChar(39), QChar(8217));
            QString shopTxt = qry.value(2).toString();
            shopTxt.replace(QChar(39), QChar(8217));
            //data存储“password \n bindshop”
            mpUsers->addItem(loginerTxt, QString("%1\n%2").arg(qry.value(1).toString()).arg(shopTxt));
        }
    }
    else
        QMessageBox::information(this, QString(), QStringLiteral("账册文件无效！"));

    qry.finish();
    qry.exec(QStringLiteral("select loginer, deskpassword, bindshop from baililoginer "
                            "where loginer='%1' or loginer='%2' order by loginer desc;")
             .arg(mapMsg.value("word_admin")).arg(bossAccount));
    while ( qry.next() ) {
        //data存储“password \n bindshop”
        mpUsers->addItem(qry.value(0).toString(),
                         QString("%1\n%2").arg(qry.value(1).toString()).arg(qry.value(2).toString()));
    }

    return QString();
}

void BsLoginGuide::upgradeSqlCheck()
{
    QStringList sqls;

    //检查添加表
    if ( !dogNetName.isEmpty() && !dogNetPass.isEmpty() ) {

        sqls << QStringLiteral("create table if not exists serverlog("
                               "reqtime     integer primary key,"   //毫秒精度
                               "reqman      text not null,"
                               "reqtype     integer default 0,"     //0解密不成功  1查统计  2查单据  3存单据  4登记
                               "reqinfo     text not null"
                               ");");

        sqls << QStringLiteral("create table if not exists serverfail ("
                               "timeid      integer primary key,"
                               "faildata    text not null);");

        sqls << QStringLiteral("create table if not exists meeting ("
                               "meetid      integer primary key,"
                               "meetname	text unique not null,"
                               "members		text default ''"
                               ");");

        sqls << QStringLiteral("create table if not exists msglog ("
                               "msgid         integer primary key, "  //milliSecondsSinceEpoch，几乎不可能重复冲突。
                               "senderId      text not null,"
                               "senderName    text not null,"
                               "receiverId    text not null,"
                               "receiverName  text not null,"
                               "content       text default '');");

        sqls << QStringLiteral("create table if not exists msgfail ("
                               "msgid       integer not null,"
                               "toFrontId   text not null,"
                               "primary key(msgid, toFrontId));");

        sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                               "'app_encryption_key', '网络保密码', '', '', "
                               "'用于数据加密传输，防止网络偷窥。设置后需要重启服务、终端重新设置登录。');");

    }

    sqls << QStringLiteral("create table if not exists colorbase(codename text primary key);");

    sqls << QStringLiteral("create table if not exists lxlabel("
                           "    labelid         integer primary key autoincrement, "
                           "    labelname       text unique not null, "
                           "    printername     text default '', "
                           "    fromx           integer default 0, "
                           "    fromy           integer default 0, "
                           "    unitxcount      integer default 1, "
                           "    unitwidth       integer default 40, "
                           "    unitheight      integer default 30, "
                           "    spacex          integer default 3, "
                           "    spacey          integer default 3, "
                           "    flownum         integer default 0, "    //存储流水号
                           "    cargoexp        text default ''"
                           ");");
    sqls << QStringLiteral("create table if not exists lxlabelobj( "
                           "    nId             integer primary key autoincrement, "
                           "    nParentid		integer not null, "
                           "    nObjType        integer, "  //0固定文字         1动态字段       2条形码        3二维码
                           "    sValue          text, "     //固定文字TEXT      =字段中文       [条形码]       [字段]文字...
                           "    sExp			text, "     //NULL              fieldName      NULL           NULL
                           "    nPosX			integer, "
                           "    nPosY			integer, "
                           "    nWidth          integer, "
                           "    nHeight         integer, "
                           "    sFontName		text, "
                           "    nFontPoint      integer, "
                           "    nFontAlign      integer "
                           "); ");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'app_boss_name', '总经理账号', '总经理', '总经理', '前后端通用全权总经理账号名称。');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'app_company_pcolor', '公司主题色', '119900', '119900', '请选择公司主题色');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'app_image_path', '货品图片根目录', '', '', '货品图片集中放置文件夹。注意每个图片文件名必须与货号一致，且需为jpg格式。');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'dots_of_qty', '数量小数位数', '0', '0', '0~4位');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'show_hpname_in_sheet_grid', '单据表格显示品名', '否', '否', '请填“是”或“否”');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'show_hpunit_in_sheet_grid', '单据表格显示单位', '否', '否', '请填“是”或“否”');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'show_hpprice_in_sheet_grid', '单据表格显示标牌价', '否', '否', '请填“是”或“否”');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'set_sheet_banlance_szd', '收支单保存是否强制检查借贷平衡', '否', '否', '请填“是”或“否”');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'set_sheet_link_finance', '单据审核时自动进行收支记账', '否', '否', '请填“是”或“否”');");

    sqls << QStringLiteral("insert or ignore into bailiOption(optcode, optname, vsetting, vdefault, vformat) values("
                           "'set_sheet_subject_divchar', '账目名称科目分隔符', '-', '-', '请使用半角字符');");

    sqls << QStringLiteral("update bailiOption set vsetting='%1', vdefault='%1' "
                           "where optcode='app_image_path' and vdefault='';").arg(imageDir);

    //数据库连接，先取用于getExistsFieldsOfTable中PROGMA取得已有字段信息
    QSqlDatabase defaultdb = QSqlDatabase::database();
    QStringList checkFields;

    //检查添加列（以下一直保留，这样不用担心初始SQL字段不齐全）
    checkFields = getExistsFieldsOfTable(QStringLiteral("baililoginer"), defaultdb);
    if ( checkFields.indexOf(QStringLiteral("limcargoexp")) < 0 )
        sqls << QStringLiteral("alter table baililoginer add column limcargoexp text default '';");

    checkFields = getExistsFieldsOfTable(QStringLiteral("customer"), defaultdb);
    if ( checkFields.indexOf(QStringLiteral("regmark")) < 0 )
        sqls << QStringLiteral("alter table customer add column regmark text default '';");

    checkFields = getExistsFieldsOfTable(QStringLiteral("supplier"), defaultdb);
    if ( checkFields.indexOf(QStringLiteral("regmark")) < 0 )
        sqls << QStringLiteral("alter table supplier add column regmark text default '';");

    checkFields = getExistsFieldsOfTable(QStringLiteral("shop"), defaultdb);
    if ( checkFields.indexOf(QStringLiteral("regmark")) < 0 )
        sqls << QStringLiteral("alter table shop add column regmark text default '';");
    if ( checkFields.indexOf(QStringLiteral("amgeo")) < 0 )
        sqls << QStringLiteral("alter table shop add column amgeo text default '';");

    checkFields = getExistsFieldsOfTable(QStringLiteral("subject"), defaultdb);
    if ( checkFields.indexOf(QStringLiteral("adminboss")) < 0 )
        sqls << QStringLiteral("alter table subject add column adminboss integer default 0;");
    if ( checkFields.indexOf(QStringLiteral("refsheetin")) < 0 )
        sqls << QStringLiteral("alter table subject add column refsheetin text default '';");
    if ( checkFields.indexOf(QStringLiteral("refsheetex")) < 0 )
        sqls << QStringLiteral("alter table subject add column refsheetex text default '';");

    checkFields = getExistsFieldsOfTable(QStringLiteral("sizertype"), defaultdb);
    if ( checkFields.indexOf(QStringLiteral("beforecolor")) < 0 )
        sqls << QStringLiteral("alter table sizertype add column beforecolor integer default 0;");

    checkFields = getExistsFieldsOfTable(QStringLiteral("cargo"), defaultdb);
    if ( checkFields.indexOf(QStringLiteral("amtag")) < 0 )
        sqls << QStringLiteral("alter table cargo add column amtag text default '';");

    //1.50升级索引（后期差不多时可仅保留bailisql.cpp中即可）
    sqls << QStringLiteral("CREATE INDEX IF NOT EXISTS idxsydshop ON syd(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxcgdshop ON cgd(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxcgjshop ON cgj(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxcgtshop ON cgt(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxdbdshop ON dbd(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxlsdshop ON lsd(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxpfdshop ON pfd(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxpffshop ON pff(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxpftshop ON pft(shop);")
         << QStringLiteral("CREATE INDEX IF NOT EXISTS idxszdshop ON szd(shop);");

    //最终批处理执行
    defaultdb.transaction();
    foreach (QString sql, sqls) {
        defaultdb.exec(sql);
        if ( defaultdb.lastError().isValid() ) {
            qDebug() << defaultdb.lastError() << sql;
            defaultdb.rollback();
            return;
        }
    }
    defaultdb.commit();
}


}

