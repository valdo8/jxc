#include "bsimportr15dlg.h"
#include "quuid.h"
#include "comm/pinyincode.h"
#include "main/bailisql.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailifunc.h"

#define IMPORT_HELP     "升迁建账过程为——创建新的库文件，并从老的数据文件中导入数据，同时改变为新的数据格式，\
以适应新的程序。因此，升迁建账完全不影响原数据文件。如果不满意，可以仍然继续使用原R15系统。因此，请放心操作甚至\
多次练习。"

#define REGEXP_HELP     "<font color='#080'><b>正则表达式</b>简单语法：\\d通配数字，\\D通配字母，\\w通配字母或数字，\
前面不加反斜线为原字符不通配；几种可能用方括号列举，比如[ABC]表示A或B或C。<br/>\
字符种类识别后，还要识别位数。位数表示法是{x,y}的形式。\
x为至少位数，y为最多位数；固定位数用{x}表示；不限最多位数用{x,}表示。<br/>\
例如：<b>\\D{2}\\d{8}</b>表示两位字母后跟8位数字；\
<b>\\d{6,}-\\D\\d{2,4}-\\D{1,8}</b>表示最少6位数字后带减号再后面是一位字母跟2到4位数字，\
再又是减号，后面是1到8位数字。以此类推。</font>"

#define COLOR_DRAG_HELP  "原R15系统色号每货号重复登记，没有分组，而事实上，多个不同货号完全应当对应相同的颜色系列。\
左边的列表将原账册中所有出现的色号全部列出，请将其逐一拖动到上表格——注意：同系列的色号应当放到同一系列里。\
对那些不再使用以及错误的色号，也要拖上去，单独放到一行中，随便取个系列名。"


namespace BailiSoft {

BsImportR15Dlg::BsImportR15Dlg(QWidget *parent, const QString &accessFile)
    : QDialog(parent), mAccessFile(accessFile), mAccessConn("MSAccessImportR15Conn"),
      mSqliteConn("SqliteImportR15Conn")
{
    //全部待处理货号
    mpGrdCargo = new QTableWidget(this);
    mpGrdCargo->setColumnCount(3);
    mpGrdCargo->setSelectionBehavior(QAbstractItemView::SelectItems);
    mpGrdCargo->setSelectionMode(QAbstractItemView::SingleSelection);
    mpGrdCargo->setHorizontalHeaderLabels(QStringList()
                                          << QStringLiteral("货号")
                                          << QStringLiteral("    品类    ")
                                          << QStringLiteral("    色系    "));
    mpGrdCargo->verticalHeader()->setDefaultSectionSize(18);
    mpGrdCargo->horizontalHeader()->setStyleSheet("QHeaderView {border-style:none; border-bottom:1px solid #ccc;}");
    mpGrdCargo->setStatusTip(QStringLiteral("需要完成<b>码类</b>与<b>色系</b>设置。提示：利用左边色码两表的正则表达式测试功能可加速完成。"));
    mpGrdCargo->installEventFilter(this);

    mpChkOnlyStock = new QCheckBox(QStringLiteral("无库存不导入"), this);

    mpBtnExpApply = new QPushButton(QStringLiteral("填入测试品类"), this);
    mpBtnExpApply->setFixedWidth(120);
    mpBtnExpApply->setEnabled(false);
    connect(mpBtnExpApply, SIGNAL(clicked(bool)), this, SLOT(applyRegExp()));

    QVBoxLayout *layPnlCargo = new QVBoxLayout;
    layPnlCargo->setContentsMargins(10, 10, 10, 10);
    layPnlCargo->addWidget(mpChkOnlyStock, 0, Qt::AlignCenter);
    layPnlCargo->addWidget(mpGrdCargo, 1);
    layPnlCargo->addWidget(mpBtnExpApply, 0, Qt::AlignCenter);

    //尺码设置
    mpGrdSizer = new BsR15UpgSetGrid(this);
    mpGrdSizer->setColumnCount(3);
    mpGrdSizer->setStatusTip(QStringLiteral("需要完成<b>品类名</b>设置。<br/><br/>") + QStringLiteral(REGEXP_HELP));
    mpGrdSizer->installEventFilter(this);
    connect(mpGrdSizer, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(regExpChanged(int,int,int,int)));
    mpGrdSizer->setHorizontalHeaderLabels(QStringList()
                                          << QStringLiteral("尺码表")
                                          << QStringLiteral("品类名")
                                          << QStringLiteral("货号特征（正则表达式）"));

    QPushButton *btnLoadSizeType = new QPushButton(QStringLiteral("导入品类匹配"), this);
    btnLoadSizeType->setFixedWidth(120);
    connect(btnLoadSizeType, &QPushButton::clicked, this, &BsImportR15Dlg::loadSizeTypeCordFile);

    //色号设置
    mpGrdColor = new BsR15UpgColorGrid(this);
    mpGrdColor->setColumnCount(3);
    mpGrdColor->setStatusTip(QStringLiteral("需要完成<b>色系名</b>设置。但先要将下面色号拖上来，分好系列。"
                                            "<br/><br/>") + QStringLiteral(REGEXP_HELP));
    mpGrdColor->installEventFilter(this);
    connect(mpGrdColor, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(regExpChanged(int,int,int,int)));
    mpGrdColor->setHorizontalHeaderLabels(QStringList()
                                          << QStringLiteral("色号表")
                                          << QStringLiteral("色系名")
                                          << QStringLiteral("货号特征（正则表达式）"));

    mpChkColorFormat = new QCheckBox(QStringLiteral("不合并色系（通常应该选此，除非色号很少）"), this);

    mpLstColor = new BsR15UpgColorList(this);
    mpLstColor->setStatusTip(QStringLiteral(COLOR_DRAG_HELP));
    mpLstColor->installEventFilter(this);

    mpLblHelp = new QLabel(this);
    mpLblHelp->setText(QStringLiteral(IMPORT_HELP));
    mpLblHelp->setWordWrap(true);

    QHBoxLayout *layColor = new QHBoxLayout;
    layColor->addWidget(mpLstColor, 1);
    layColor->addWidget(mpLblHelp, 2);

    //确定取消
    QDialogButtonBox *pBox = new QDialogButtonBox(this);
    pBox->setOrientation(Qt::Horizontal);
    pBox->setCenterButtons(true);

    QPushButton *pBtnOk = pBox->addButton(QStringLiteral("执行导入"), QDialogButtonBox::AcceptRole);
    pBtnOk->setIcon(QIcon(":/icon/ok.png"));
    pBtnOk->setFixedSize(120, 32);
    pBtnOk->setIconSize(QSize(24, 24));
    connect(pBtnOk, SIGNAL(clicked(bool)), this, SLOT(doImport()));

    QPushButton *pBtnCancel = pBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setFixedSize(70, 32);
    pBtnCancel->setIconSize(QSize(24, 24));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    //左布局
    QVBoxLayout *layPnlCords = new QVBoxLayout;
    layPnlCords->setContentsMargins(0, 10, 0, 10);
    layPnlCords->addWidget(mpGrdSizer, 1);
    layPnlCords->addWidget(btnLoadSizeType, 0, Qt::AlignCenter);
    layPnlCords->addSpacing(12);
    layPnlCords->addWidget(mpGrdColor, 2);
    layPnlCords->addWidget(mpChkColorFormat, 0, Qt::AlignCenter);
    layPnlCords->addSpacing(12);
    layPnlCords->addLayout(layColor, 2);
    layPnlCords->addSpacing(12);
    layPnlCords->addWidget(pBox, 0, Qt::AlignCenter);

    //布局
    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addLayout(layPnlCargo, 1);
    lay->addLayout(layPnlCords, 2);
    lay->addWidget(new QSizeGrip(this), 0, Qt::AlignBottom | Qt::AlignRight);

    //正则表达式测试按钮（浮动显示）
    mpBtnExpTest = new QPushButton(QStringLiteral("测试"), this);
    connect(mpBtnExpTest, SIGNAL(clicked(bool)), this, SLOT(testRegExp()));

    //窗口
    setMinimumSize(1000, 600);
    setWindowTitle(QStringLiteral("R15数据升迁工具"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);

    //信号
    connect(this, SIGNAL(destroyed(QObject*)), this, SLOT(overDestroy()));
    connect(mpGrdSizer, SIGNAL(viewportEntered()), this, SLOT(showHelpHint()));
    connect(mpGrdColor, SIGNAL(viewportEntered()), this, SLOT(showHelpHint()));
    connect(mpGrdCargo, SIGNAL(viewportEntered()), this, SLOT(showHelpHint()));
    connect(mpLstColor, SIGNAL(viewportEntered()), this, SLOT(showHelpHint()));
    connect(mpChkOnlyStock, &QCheckBox::clicked, this, &BsImportR15Dlg::reloadOldBookData);
    connect(mpChkColorFormat, &QCheckBox::clicked, this, &BsImportR15Dlg::switchColorFormat);

    //打开老数据
    reloadOldBookData();
}

bool BsImportR15Dlg::eventFilter(QObject *watched, QEvent *event)
{
    if ( event->type() == QEvent::StatusTip ) {
        QWidget *w = qobject_cast<QWidget*>(watched);
        QStatusTipEvent *e = static_cast<QStatusTipEvent*>(event);
        if ( w && e ) {
            mpLblHelp->setText(e->tip());
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

bool BsImportR15Dlg::testR15Book(const QString &fileName)
{
    const QString testConnName = QStringLiteral("R15BookOpenTestConn");
    bool openValid = true;

    {
        QSqlDatabase dbConn = QSqlDatabase::addDatabase("QODBC", testConnName);
        dbConn.setDatabaseName(QStringLiteral("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};FIL={MS Access};DBQ=%1")
                               .arg(QDir::toNativeSeparators(fileName)));
        if ( dbConn.open() ) {
            QString sql = QStringLiteral("select setValue from sysRunValues where vlName='azcdbAlias';");
            QSqlQuery mqry(dbConn);
            mqry.exec(sql);
            openValid = mqry.next();
            mqry.finish();
        }
        else {
            openValid = false;
        }
    }

    QSqlDatabase::removeDatabase(testConnName);
    return openValid;
}

void BsImportR15Dlg::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    //判断文件是否存在
    QDir dir(backupDir);
    QString fileName = dir.absoluteFilePath(QStringLiteral("%1.txt").arg(mBookName));
    if ( ! QFile::exists(fileName) )
        return;

    //判断文件是否为unicode编码
    if ( LxSoft::isTextUnicode(fileName) ) {
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    }

    //读取文件数据
    QString fileData;
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream strm(&f);
        fileData = strm.readAll();
        f.close();
    }
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("System"));

    //解入表格
    QStringList parts = fileData.split(QChar(12));
    if ( parts.length() != 2 )
        return;

    QStringList ssets = QString(parts.at(0)).split(QChar(10));
    for ( int i = 0, iLen = ssets.length(); i < iLen; ++i ) {
        QStringList cols = QString(ssets.at(i)).split(QChar(9));
        if ( cols.length() == 3 ) {
            if ( mpGrdSizer->rowCount() < i + 1 ) {
                mpGrdSizer->setRowCount(i + 1);
                mpGrdSizer->setItem(i, 0, new QTableWidgetItem());
                mpGrdSizer->setItem(i, 1, new QTableWidgetItem());
                mpGrdSizer->setItem(i, 2, new QTableWidgetItem());
            }
            mpGrdSizer->item(i, 0)->setText(cols.at(0));
            mpGrdSizer->item(i, 1)->setText(cols.at(1));
            mpGrdSizer->item(i, 2)->setText(cols.at(2));
        }
    }

    QStringList csets = QString(parts.at(1)).split(QChar(10));
    QStringList pickedColors;
    for ( int i = 0, iLen = csets.length(); i < iLen; ++i ) {
        QStringList cols = QString(csets.at(i)).split(QChar(9));
        if ( cols.length() == 3 ) {
            if ( mpGrdColor->rowCount() < i + 1 ) {
                mpGrdColor->setRowCount(i + 1);
                mpGrdColor->setItem(i, 0, new QTableWidgetItem());
                mpGrdColor->setItem(i, 1, new QTableWidgetItem());
                mpGrdColor->setItem(i, 2, new QTableWidgetItem());
            }
            mpGrdColor->item(i, 0)->setText(cols.at(0));
            mpGrdColor->item(i, 1)->setText(cols.at(1));
            mpGrdColor->item(i, 2)->setText(cols.at(2));
            pickedColors << QString(cols.at(0)).split(QChar(44));
        }
    }

    for ( int i = 0, iLen = mpLstColor->count(); i < iLen; ++i ) {
        QListWidgetItem *it = mpLstColor->item(i);
        if ( pickedColors.indexOf(it->text()) >= 0 ) {
            it->setData(Qt::UserRole, true);
            it->setForeground(Qt::darkGreen);
        }
    }
}

void BsImportR15Dlg::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() != Qt::Key_Enter && e->key() != Qt::Key_Return && e->key() != Qt::Key_Escape )
        QDialog::keyPressEvent(e);
}

void BsImportR15Dlg::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);

    mpBtnExpTest->hide();

    mpGrdSizer->setColumnWidth(0, mpGrdSizer->width() / 2);
    mpGrdSizer->setColumnWidth(1, mpGrdSizer->width() / 8);

    mpGrdColor->setColumnWidth(0, mpGrdColor->width() / 2);
    mpGrdColor->setColumnWidth(1, mpGrdColor->width() / 8);
}

void BsImportR15Dlg::reloadOldBookData()
{
    //由于中间可能要换文件打开，所以如果isValid()，则仅仅close()等待重新setDatabaseName()
    QSqlDatabase mdb = QSqlDatabase::database(mAccessConn);
    if ( mdb.isValid() )
        mdb.close();
    else
        mdb = QSqlDatabase::addDatabase("QODBC", mAccessConn);

    //具体mdb文件
    mdb.setDatabaseName(QStringLiteral("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};FIL={MS Access};DBQ=%1")
                        .arg(QDir::toNativeSeparators(mAccessFile)));
    if ( mdb.open() ) {

        //账册名
        mBookName = QString();
        QString sql = QStringLiteral("select setValue from sysRunValues where vlName='azcdbAlias';");
        QSqlQuery mqry(mdb);
        mqry.exec(sql);
        if ( mqry.next() )
            mBookName = mqry.value(0).toString();
        mqry.finish();

        //文件名
        QString fileBase = QFileInfo(mAccessFile).baseName() + QStringLiteral(".jyb");
        QDir dir(dataDir);
        mSqliteFile = dir.absoluteFilePath(fileBase);
    }
    else {
        //因为BsImportR15Dlg对话框打开前已经test过，不存在。
    }

    //开始读取表格老数据
    QSqlQuery qry(mdb);
    qry.setForwardOnly(true);
    QString sql = QStringLiteral("select * from sysSizeCols order by id;");
    qry.exec(sql);
    if ( qry.lastError().isValid() ) {
        mpLblHelp->setText(QStringLiteral("错误的R15数据，无法导入！"));
        mpLblHelp->setStyleSheet("color:red;");
        return;
    }

    //尺码
    mpGrdSizer->clearContents();
    int flds = qry.record().count();
    int rows = 0;
    while ( qry.next() ) {
        mpGrdSizer->setRowCount(++rows);

        QStringList showSizers;
        QStringList dataSizers;
        for ( int i = 1; i < flds;  ++i ) {
            QString sizer = qry.value(i).toString();
            dataSizers << sizer;
            if ( ! sizer.isEmpty() )
                showSizers << sizer;
        }

        QTableWidgetItem *itSizers = new QTableWidgetItem(showSizers.join(QChar(44)));
        itSizers->setData(Qt::UserRole, dataSizers);
        itSizers->setFlags(Qt::NoItemFlags);
        itSizers->setBackground(QColor(240, 240, 240));
        mpGrdSizer->setItem(rows - 1, 0, itSizers);
        mpGrdSizer->setItem(rows - 1, 1, new QTableWidgetItem());
        mpGrdSizer->setItem(rows - 1, 2, new QTableWidgetItem());
    }

    if ( rows == 1 )
        mpGrdSizer->item(0, 2)->setText(QStringLiteral(".*"));

    //色号备选
    sql = ( mpChkOnlyStock->isChecked() )
            ? QStringLiteral("select color from qUKC group by color having sum(qty)<>0;")
            : QStringLiteral("select distinct color from qUKC order by color;");
    qry.exec(sql);
    QStringList colors;
    while ( qry.next() ) {
        QString clr = qry.value(0).toString().trimmed();
        if ( ! clr.isEmpty() )
            colors << QStringLiteral("    %1").arg(clr);    //留出Drag mouse hotpot空间
    }
    mpLstColor->clear();
    mpLstColor->addItems(colors);

    //货号
    QSet<QString> setCargos;
    mpGrdCargo->clearContents();
    mpGrdCargo->setRowCount(0);
    mpGrdCargo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    sql = ( mpChkOnlyStock->isChecked() )
            ? QStringLiteral("select a.cargo, b.cgname, b.attr1, b.attr2, b.attr3, b.attr4, b.attr5, b.attr6 "
                             "from qUKC a left join tCargo b on a.cargo=b.cargo "
                             "group by a.cargo, b.cgname, b.attr1, b.attr2, b.attr3, b.attr4, b.attr5, b.attr6 "
                             "having sum(a.qty)<>0;")
            : QStringLiteral("select cargo, cgname, attr1, attr2, attr3, attr4, attr5, attr6 "
                             "from tCargo order by cargo;");
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << sql;
    rows = 0;
    while ( qry.next() ) {
        QString cargo = qry.value(0).toString().trimmed();
        if ( !cargo.isEmpty() ) {
            mpGrdCargo->setRowCount(++rows);
            QTableWidgetItem *itCargo = new QTableWidgetItem(cargo);
            itCargo->setData(Qt::UserRole, qry.value(1).toString());
            itCargo->setData(Qt::UserRole + 1, qry.value(2).toString());
            itCargo->setData(Qt::UserRole + 2, qry.value(3).toString());
            itCargo->setData(Qt::UserRole + 3, qry.value(4).toString());
            itCargo->setData(Qt::UserRole + 4, qry.value(5).toString());
            itCargo->setData(Qt::UserRole + 5, qry.value(6).toString());
            itCargo->setData(Qt::UserRole + 6, qry.value(7).toString());
            itCargo->setBackground(QColor(240, 240, 240));
            itCargo->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            mpGrdCargo->setItem(rows - 1, 0, itCargo);
            mpGrdCargo->setItem(rows - 1, 1, new QTableWidgetItem());
            mpGrdCargo->setItem(rows - 1, 2, new QTableWidgetItem());
            setCargos.insert(cargo);
        }
    }

    //补充总库存为零但各店或各色仍然有的库存（因为正负相抵）
    sql = QStringLiteral("select a.stock, a.cargo, a.color, b.cgname, b.attr1, b.attr2, b.attr3, b.attr4, b.attr5, b.attr6 "
                         "from qUKC a left join tCargo b on a.cargo=b.cargo "
                         "group by a.stock, a.cargo, a.color, b.cgname, b.attr1, b.attr2, b.attr3, b.attr4, b.attr5, b.attr6 "
                         "having sum(a.qty)<>0;");
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << sql;
    while ( qry.next() ) {
        QString cargo = qry.value(1).toString().trimmed();
        if ( !setCargos.contains(cargo) ) {
            mpGrdCargo->setRowCount(++rows);
            QTableWidgetItem *itCargo = new QTableWidgetItem(cargo);
            itCargo->setData(Qt::UserRole, qry.value(3).toString());
            itCargo->setData(Qt::UserRole + 1, qry.value(4).toString());
            itCargo->setData(Qt::UserRole + 2, qry.value(5).toString());
            itCargo->setData(Qt::UserRole + 3, qry.value(6).toString());
            itCargo->setData(Qt::UserRole + 4, qry.value(7).toString());
            itCargo->setData(Qt::UserRole + 5, qry.value(8).toString());
            itCargo->setData(Qt::UserRole + 6, qry.value(9).toString());
            itCargo->setBackground(QColor(240, 240, 240));
            itCargo->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            mpGrdCargo->setItem(rows - 1, 0, itCargo);
            mpGrdCargo->setItem(rows - 1, 1, new QTableWidgetItem());
            mpGrdCargo->setItem(rows - 1, 2, new QTableWidgetItem());
            setCargos.insert(cargo);
        }
    }

    //登记货号已经删除但仍有库存的部分
    sql = QStringLiteral("select stock, cargo, color from qUKC group by stock, cargo, color having sum(qty)<>0;");
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << sql;
    while ( qry.next() ) {
        QString cargo = qry.value(1).toString().trimmed();
        if ( !setCargos.contains(cargo) ) {
            mpGrdCargo->setRowCount(++rows);
            QTableWidgetItem *itCargo = new QTableWidgetItem(cargo);
            itCargo->setBackground(QColor(240, 240, 240));
            itCargo->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            mpGrdCargo->setItem(rows - 1, 0, itCargo);
            mpGrdCargo->setItem(rows - 1, 1, new QTableWidgetItem());
            mpGrdCargo->setItem(rows - 1, 2, new QTableWidgetItem());
            setCargos.insert(cargo);
        }
    }

    mpGrdCargo->resizeColumnsToContents();
    mpGrdCargo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mpGrdCargo->verticalHeader()->hide();
    mpBtnExpTest->hide();
}

void BsImportR15Dlg::loadSizeTypeCordFile()
{
    //备用
    QStringList tnames;
    for ( int i = 0, iLen = mpGrdSizer->rowCount(); i < iLen; ++i ) {
        tnames << mpGrdSizer->item(i, 1)->text();
    }

    //读取
    QString dir = QStandardPaths::locate(QStandardPaths::DesktopLocation, QString(), QStandardPaths::LocateDirectory);
    QString openData = openLoadTextFile(mapMsg.value("tool_import_data"), dir, mapMsg.value("i_formatted_csv_file"), this);
    if ( openData.isEmpty() )
        return;

    //匹配
    QStringList rows = openData.split(QChar(10));
    foreach (QString row, rows) {
        QStringList cols = row.split(QChar(','));
        if ( cols.length() >= 2 ) {
            QString cargo = cols.at(0);
            QString sizet = cols.at(1);
            int tIndex = tnames.indexOf(sizet);
            if ( tIndex < 0 ) {
                QMessageBox::information(this, QString(), QStringLiteral("匹配表文件中码类名发现与现左表格有不一致名称。"));
                return;
            }
            for ( int i = 0, iLen = mpGrdCargo->rowCount(); i < iLen; ++i ) {
                QTableWidgetItem* itCargo = mpGrdCargo->item(i, 0);
                QTableWidgetItem* itSizet = mpGrdCargo->item(i, 1);
                if ( itCargo->text() == cargo ) {
                    itCargo->setForeground(Qt::black);
                    itSizet->setText(sizet);
                    itSizet->setData(Qt::UserRole, tIndex);
                }
            }
        }
    }
}

//doImport
void BsImportR15Dlg::doImport()
{
    //检查
    QString checkResult = checkReady();
    if ( !checkResult.isEmpty() ) {
        QMessageBox::information(this, QString(), checkResult);
        return;
    }

    //保存设置（以防失败，又要重新设）
    QStringList ssets;
    for ( int i = 0, iLen = mpGrdSizer->rowCount(); i < iLen; ++i ) {
        QStringList cols;
        cols << mpGrdSizer->item(i, 0)->text();
        cols << mpGrdSizer->item(i, 1)->text();
        cols << mpGrdSizer->item(i, 2)->text();
        ssets << cols.join(QChar(9));
    }

    QStringList csets;
    for ( int i = 0, iLen = mpGrdColor->rowCount(); i < iLen; ++i ) {
        QStringList cols;
        if ( mpGrdColor->item(i, 0) && mpGrdColor->item(i, 1) && mpGrdColor->item(i, 2) ) {
            cols << mpGrdColor->item(i, 0)->text();
            cols << mpGrdColor->item(i, 1)->text();
            cols << mpGrdColor->item(i, 2)->text();
            csets << cols.join(QChar(9));
        }
    }

    QDir dir(backupDir);
    QString fileName = dir.absoluteFilePath(QStringLiteral("%1.txt").arg(mBookName));
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream strm(&f);
        strm.setGenerateByteOrderMark(true);
        strm << (ssets.join(QChar(10)) + QChar(12) + csets.join(QChar(10)));    //不要用QChar(13)，QTextStream读取时会换为\n
        f.close();
    }

    //先检查删除库文件
    if ( QFile::exists(mSqliteFile) ) {
        if ( ! QFile::remove(mSqliteFile) ) {
            QMessageBox::information(this, QString(),
                                     QStringLiteral("%1文件被占用，无法删除，请关闭Sqlite工具！").arg(mSqliteFile));
            return;
        }
    }

    //创建库文件并初始化为R17数据结构
    QString strErr;
    {
        //初始化新数据库表结构完整SQL
        QStringList sqls = BailiSoft::sqliteInitSqls(mBookName, true);

        //启动数据库连接
        QSqlDatabase sdb = QSqlDatabase::addDatabase("QSQLITE", mSqliteConn);
        sdb.setDatabaseName(mSqliteFile);
        if ( sdb.open() ) {
            strErr = initNewSchema(sqls, sdb);
        } else {
            strErr = QStringLiteral("系统不支持Sqlite，%1创建不成功。错误信息：%2").arg(mSqliteFile).arg(sdb.lastError().text());
        }

        //执行导入过程
        if ( strErr.isEmpty() ) {
            QSqlDatabase mdb = QSqlDatabase::database(mAccessConn);
            qApp->setOverrideCursor(Qt::WaitCursor);
            strErr = importTableByTable(mdb, sdb);
            qApp->restoreOverrideCursor();
        }
    }

    //移除连接必须在addDatabase()作用域花括号外
    QSqlDatabase::removeDatabase(mSqliteConn);

    //报告
    if ( strErr.isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("导入成功！"));
        accept();   //调用外部使用mSqliteFile放到账册列表
    } else {
        mpLblHelp->setText(QStringLiteral("导入失败！\n%1").arg(strErr));
    }
}

void BsImportR15Dlg::overDestroy()
{
    QSqlDatabase::removeDatabase(mAccessConn);
}

void BsImportR15Dlg::showHelpHint()
{
    QWidget *w = qobject_cast<QWidget*>(QObject::sender());
    if ( w ) {
        if ( w->statusTip().isEmpty() ) {
            QWidget* grid = qobject_cast<QWidget*>(w->parent());
            if ( grid )
                mpLblHelp->setText(grid->statusTip());
        }
        else
            mpLblHelp->setText(w->statusTip());
    }
}

void BsImportR15Dlg::regExpChanged(int currentRow, int currentColumn, int, int)
{
    QTableWidget *grid = qobject_cast<QTableWidget*>(QObject::sender());
    Q_ASSERT(grid);

    if ( currentColumn != 2 || currentRow < 0 || grid->rowCount() < 1 )
        return;

    QModelIndex idx = grid->model()->index(currentRow, 2);
    QPoint pt = grid->viewport()->mapTo(this, grid->visualRect(idx).topRight());

    mpBtnExpTest->setProperty("gridIsColor", grid == mpGrdColor);
    mpBtnExpTest->setGeometry(pt.x() - 38, pt.y() - 1, 40, 22);
    mpBtnExpTest->show();
    mpBtnExpApply->setEnabled(false);
}

void BsImportR15Dlg::testRegExp()
{
    matchCargoRegExp(true);
}

void BsImportR15Dlg::applyRegExp()
{
    matchCargoRegExp(false);
}

void BsImportR15Dlg::switchColorFormat(const bool keepColorsCommaFormat)
{
    mpGrdColor->setEnabled(!keepColorsCommaFormat);
    mpLstColor->setEnabled(!keepColorsCommaFormat);

    if ( keepColorsCommaFormat ) {
        QMap<QString, QString>  mapRegs;
        QSqlDatabase mdb = QSqlDatabase::database(mAccessConn);
        QSqlQuery qry(mdb);
        qry.setForwardOnly(true);
        qry.exec(QStringLiteral("select cargo, colorList from tCargo;"));
        while (qry.next()) {
            mapRegs.insert(qry.value(0).toString(), qry.value(1).toString());
        }
        qry.finish();
        for ( int i = 0, iLen = mpGrdCargo->rowCount(); i < iLen; ++i ) {
            QString cargo = mpGrdCargo->item(i, 0)->text();
            if ( mapRegs.contains(cargo) ) {
                mpGrdCargo->item(i, 2)->setText( mapRegs.value(cargo) );
            }
        }
    }
    else {
        for ( int i = 0, iLen = mpGrdCargo->rowCount(); i < iLen; ++i ) {
            mpGrdCargo->item(i, 2)->setText(QString());
        }
    }
}

QString BsImportR15Dlg::checkReady()
{
    //色号整理
    if ( ! mpChkColorFormat->isChecked() ) {
        int unPicks = 0;
        for ( int i = 0, iLen = mpLstColor->count(); i < iLen; ++i ) {
            if ( !mpLstColor->item(i)->data(Qt::UserRole).toBool() )
                unPicks++;
        }
        if ( unPicks > 0 )
            return QStringLiteral("色号整理分系列未完成，请将原色号列表中色号拖到色号设置表格中，全部变位绿色，一个不能剩！");
    }

    //色号
    if ( mpGrdColor->rowCount() > 0 )
    {
        int nullCount = 0;
        for ( int i = 0; i < mpGrdColor->rowCount(); ++i ) {
            if ( !mpGrdColor->item(i, 1) ||
                 mpGrdColor->item(i, 1)->text().isEmpty() ) {
                nullCount++;
            }
        }
        if ( nullCount > 0 )
            return QStringLiteral("色号表设置未完成！");
    }

    //货号码类
    int nullCount = 0;
    for ( int i = 0; i < mpGrdCargo->rowCount(); ++i ) {
        if ( mpGrdCargo->item(i, 1)->text().isEmpty() ) {
            nullCount++;
        }
    }
    if ( nullCount > 0 ) return QStringLiteral("货号码类设置未完成！");

    //货号色系
    if ( ! mpChkColorFormat->isChecked() ) {
        int nullCount = 0;
        for ( int i = 0; i < mpGrdCargo->rowCount(); ++i ) {
            if ( mpGrdCargo->item(i, 2)->text().isEmpty() ) {
                nullCount++;
            }
        }
        if ( nullCount > 0 ) return QStringLiteral("货号色系设置未完成！");
    }

    //Ready!
    return QString();
}

void BsImportR15Dlg::matchCargoRegExp(const bool forTest)
{
    bool useColor = mpBtnExpTest->property("gridIsColor").toBool();
    QTableWidget *grid = (useColor) ? mpGrdColor : mpGrdSizer;
    int rowIdx = grid->currentRow();
    QString strExp = ( grid->item(rowIdx, 2) ) ? grid->item(rowIdx, 2)->text() : QString();
    int cargoFillCol = (useColor) ? 2 : 1;
    QString fillValue = ( grid->item(rowIdx, 1) ) ? grid->item(rowIdx, 1)->text() : QString();

    if ( !forTest && fillValue.isEmpty() ) {
        QString setCname = (useColor) ? QStringLiteral("品类名") : QStringLiteral("色系名");
        QMessageBox::information(this, QString(), QStringLiteral("请先设置%1！").arg(setCname));
        return;
    }

    int cordCount = 0;
    QString regExpString = QChar('^') + strExp + QChar('$');
    QRegularExpression regExp(regExpString);

    //货号正则表达式识别码类
    if ( regExp.isValid() ) {
        mpGrdCargo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        for ( int i = 0, iLen = mpGrdCargo->rowCount(); i < iLen; ++i ) {
            QString cargo = mpGrdCargo->item(i, 0)->text();
            QRegularExpressionMatch match = regExp.match(cargo);
            mpGrdCargo->item(i, 0)->setForeground(Qt::black);
            if (match.hasMatch()) {
                QString matched = match.captured(0);
                if ( matched == cargo ) {
                    cordCount++;
                    if ( forTest )
                        mpGrdCargo->item(i, 0)->setForeground(Qt::red);
                    else {
                        mpGrdCargo->item(i, cargoFillCol)->setText(fillValue);
                        mpGrdCargo->item(i, cargoFillCol)->setData(Qt::UserRole, rowIdx);
                    }
                }
            }
        }
    }
    //品名或属性列值特征（格式“x[~text”或“x[=text”）（x为0~6，分别表示cgName与attr，[~表包含，[=表等于）
    //约定使用不完全[]对，以使正则表达式无效。
    else if ( strExp.length() > 3 && strExp.at(0).isDigit() && strExp.at(1) == QChar('[') ) {
        int attrIdx = strExp.mid(0, 1).toInt();  //attr0用作cgname
        Q_ASSERT(attrIdx >= 0 && attrIdx <= 6);
        bool textEq = strExp.at(2) == QChar('=');
        QString textReg = strExp.mid(3);
        mpGrdCargo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        for ( int i = 0, iLen = mpGrdCargo->rowCount(); i < iLen; ++i ) {
            QTableWidgetItem *it = mpGrdCargo->item(i, 0);
            QString attrv = it->data(Qt::UserRole + attrIdx).toString();
            it->setForeground(Qt::black);
            bool matched = ( textEq ) ? (attrv == textReg) : attrv.contains(textReg);
            if ( matched ) {
                cordCount++;
                if ( forTest )
                    it->setForeground(Qt::red);
                else {
                    mpGrdCargo->item(i, cargoFillCol)->setText(fillValue);
                    mpGrdCargo->item(i, cargoFillCol)->setData(Qt::UserRole, rowIdx);
                }
            }
        }
    }

    mpGrdCargo->resizeColumnsToContents();
    mpGrdCargo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    if ( forTest ) {
        mpLblHelp->setText(QStringLiteral("共 %1 货号，<font color='#080'><b>%2</b></font> 匹配 %3 个货号，红色显示如右。")
                              .arg(mpGrdCargo->rowCount()).arg(strExp).arg(cordCount));
        mpBtnExpApply->setText((useColor) ? QStringLiteral("填入测试色系") : QStringLiteral("填入测试品类"));
    }
    mpBtnExpApply->setEnabled(true);
}

QString BsImportR15Dlg::initNewSchema(const QStringList &sqls, QSqlDatabase &newdb)
{   
    QString strErr;
    newdb.transaction();
    for ( int i = 0, iLen = sqls.length(); i < iLen; ++i ) {

        QString sql = sqls.at(i);
        sql = sql.trimmed();

        if ( sql.startsWith("BEGIN") || sql.startsWith("begin") ) continue;
        if ( sql.startsWith("COMMIT") || sql.startsWith("commit") ) break;

        if ( sql.length() > 9 ) {
            newdb.exec(sql);
            if ( newdb.lastError().isValid() ) {
                strErr = newdb.lastError().text() + QStringLiteral("[初始化新库时]\n") + sql;
                newdb.rollback();
                break;
            }
        }
    }
    if ( strErr.isEmpty() )
        newdb.commit();

    return strErr;
}

//execute table by table
QString BsImportR15Dlg::importTableByTable(QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    //在后面导入过程中初始化
    mapSetPrice.clear();

    //预备尺码相关数据
    QString strErr = prepareSizeMap();
    if ( !strErr.isEmpty() )
        return strErr;

    //以下逐表导入

    if ( strErr.isEmpty() )
        strErr = importSizerType(newdb);

    if ( strErr.isEmpty() && ! mpChkColorFormat->isChecked() )
        strErr = importColorType(newdb);

    if ( strErr.isEmpty() )
        strErr = importCargo(mdb, newdb);

    if ( strErr.isEmpty() )
        strErr = importStaff(mdb, newdb);

    if ( strErr.isEmpty() )
        strErr = importBaseRef("tStock", "shop", mdb, newdb);

    if ( strErr.isEmpty() )
        strErr = importBaseRef("tSupplier", "supplier", mdb, newdb);

    if ( strErr.isEmpty() )
        strErr = importBaseRef("tCustomer", "customer", mdb, newdb);

    if ( mpChkOnlyStock->isChecked() ) {
        strErr = importStockAsInitSYD(mdb, newdb);
    }
    else {
        if ( strErr.isEmpty() )
            strErr = importSheet("SYD", "syd", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("DBD", "dbd", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("CGJ", "cgj", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("CGT", "cgt", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("LSD", "lsd", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("PFF", "pff", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("PFT", "pft", mdb, newdb);
    }

    //刷新主表合计
    if ( strErr.isEmpty() )
        strErr = updateSheetSum("cgd", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("pfd", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("syd", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("dbd", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("lsd", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("cgj", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("cgt", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("pff", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("pft", newdb);

    //return
    return strErr;
}

QString BsImportR15Dlg::prepareSizeMap()
{
    mOldSizeTypeCount = mpGrdSizer->rowCount();
    mOldSizeColCount = mpGrdSizer->item(0, 0)->data(Qt::UserRole).toStringList().count();

    vecSizerType.clear();
    for ( int i = 0; i < mpGrdSizer->rowCount(); i++ ) {
        QString typeName = mpGrdSizer->item(i, 1) ? mpGrdSizer->item(i, 1)->text().trimmed() : QString();
        QStringList sizers = mpGrdSizer->item(i, 0)->data(Qt::UserRole).toStringList();
        vecSizerType << qMakePair(typeName, sizers);
    }

    mapSizerType.clear();
    for ( int i = 0, iLen = mpGrdCargo->rowCount(); i < iLen; ++i ) {
        mapSizerType.insert(mpGrdCargo->item(i, 0)->text(), mpGrdCargo->item(i, 1)->data(Qt::UserRole).toInt());
    }

    return QString();
}

QString BsImportR15Dlg::importSizerType(QSqlDatabase &newdb)
{
    QStringList sqls;

    for ( int i = 0, iLen = mpGrdSizer->rowCount(); i < iLen; ++i ) {
        QString typeName = mpGrdSizer->item(i, 1) ? mpGrdSizer->item(i, 1)->text().trimmed() : QString();
        if ( !typeName.isEmpty() ) {
            sqls << QStringLiteral("insert into sizertype(tname, namelist) values('%1', '%2');")
                    .arg(mpGrdSizer->item(i, 1)->text()).arg(mpGrdSizer->item(i, 0)->text());
        }
    }

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::importColorType(QSqlDatabase &newdb)
{
    QStringList sqls;

    for ( int i = 0, iLen = mpGrdColor->rowCount(); i < iLen; ++i ) {
        QStringList names = mpGrdColor->item(i, 0)->text().split(QChar(44));
        QStringList codes;
        foreach (QString name, names) {
            int ascChars = 0;
            for ( int j = 0, jLen = name.length(); j < jLen; ++j ) {
                char ch = name.at(j).toLatin1();
                if ( (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') )
                    ascChars++;
            }
            if ( ascChars > 0 )
                codes << name.left(ascChars);
        }
        QString colorCodes = (names.length() == codes.length()) ? codes.join(QChar(44)) : QString();

        sqls << QStringLiteral("insert into colortype(tname, namelist, codelist) values('%1', '%2', '%3');")
                .arg(mpGrdColor->item(i, 1)->text()).arg(mpGrdColor->item(i, 0)->text()).arg(colorCodes);
    }

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::importCargo(QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    //字典
    QMap<QString, QPair<QString, QString> > mapSCType;
    for ( int i = 0, iLen = mpGrdCargo->rowCount(); i < iLen; ++i ) {
        mapSCType.insert( mpGrdCargo->item(i, 0)->text(),
                          qMakePair(mpGrdCargo->item(i, 1)->text(), mpGrdCargo->item(i, 2)->text()) );
    }

    //遍历
    QStringList sqls;
    QSqlQuery mqry(mdb);
    mqry.exec(QStringLiteral("select cargo, cgName, setPrice, retPrice, lotPrice, newPrice, "
                      "attr1, attr2, attr3, attr4, attr5, attr6, upMan, upTime from tCargo where stopp=0;"));
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        QString cargo = mqry.value(0).toString();
        if ( mapSCType.contains(cargo) ) {
            double setPrice = mqry.value(2).toDouble();
            mapSetPrice.insert(cargo, setPrice);
            QDateTime dt = mqry.value(13).toDateTime();
            sqls << QStringLiteral("insert into cargo(hpcode, hpname, sizertype, colortype, "
                                   "setPrice, retPrice, lotPrice, buyPrice, "
                                   "attr1, attr2, attr3, attr4, attr5, attr6, upMan, upTime) "
                                   "values('%1', '%2', '%3', '%4', "
                                   "%5, %6, %7, %8, "
                                   "'%9', '%10', '%11', '%12', '%13', '%14', '%15', %16);")
                    .arg(cargo)
                    .arg(mqry.value(1).toString())
                    .arg(mapSCType.value(cargo).first)
                    .arg(mapSCType.value(cargo).second)
                    .arg(bsNumForSave(mqry.value(2).toDouble()))
                    .arg(bsNumForSave(mqry.value(3).toDouble()))
                    .arg(bsNumForSave(mqry.value(4).toDouble()))
                    .arg(bsNumForSave(mqry.value(5).toDouble()))
                    .arg(mqry.value(6).toString())
                    .arg(mqry.value(7).toString())
                    .arg(mqry.value(8).toString())
                    .arg(mqry.value(9).toString())
                    .arg(mqry.value(10).toString())
                    .arg(mqry.value(11).toString())
                    .arg(mqry.value(12).toString())
                    .arg(dt.toMSecsSinceEpoch() / 1000);
        }
    }

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::importStaff(QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);

    mqry.exec(QStringLiteral("select staff from tStaff;"));
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        sqls << QStringLiteral("insert into staff(kname, upman, uptime)"
                        "values('%1', '导入', %2);")
                .arg(mqry.value(0).toString())
                .arg(QDateTime::currentMSecsSinceEpoch() / 1000);
    }

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::importBaseRef(const QString &oldTable, const QString &newTable,
                              QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);

    mqry.exec(QStringLiteral("select scsname, discount, tName, tAddr, tTele, upMan, upTime from %1 where stopp=0;").arg(oldTable));
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        QDateTime dt = mqry.value(6).toDateTime();
        sqls << QStringLiteral("insert into %1(kname, regdis, regman, regaddr, regtele, upMan, upTime) "
                        "values('%2', %3, '%4', '%5', '%6', '%7', %8);")
                .arg(newTable)
                .arg(mqry.value(0).toString())
                .arg(bsNumForSave(mqry.value(1).toDouble()))
                .arg(mqry.value(2).toString())
                .arg(mqry.value(3).toString())
                .arg(mqry.value(4).toString())
                .arg(mqry.value(5).toString())
                .arg(dt.toMSecsSinceEpoch() / 1000);
    }

    return batchExec(sqls, newdb);
}

//单据导入（尺码等格式转换在此过程中）
QString BsImportR15Dlg::importSheet(const QString &oldSheet, const QString &newSheet,
                            QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);
    QString oldSizeCols = getOldSizeColSum(QStringLiteral("%1dtl.").arg(oldSheet));

    //主表
    QString sql = QStringLiteral("select sheetID, proof, dateD, stock, trader, stype, staff, remark, "
                          "checkk, checker, upMan, upTime from %1;").arg(oldSheet);
    mqry.exec(sql);
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        QDate dateD = mqry.value(2).toDate();
        qint64 chktime = (mqry.value(8).toBool()) ? QDateTime::currentMSecsSinceEpoch() : 0;
        QDateTime upTime = mqry.value(11).toDateTime();
        QString sql = QStringLiteral("insert into %1(sheetID, proof, dateD, shop, trader, stype, staff, remark, "
                              "checker, chktime, upMan, upTime) "
                              "values(%2, '%3', %4, '%5', '%6', '%7', '%8', '%9', '%10', %11, '%12', %13);")
                .arg(newSheet)
                .arg(mqry.value(0).toInt())
                .arg(mqry.value(1).toString().replace(QChar(39), QChar(8217)))
                .arg(QDateTime(dateD).toMSecsSinceEpoch() / 1000)
                .arg(mqry.value(3).toString())
                .arg(mqry.value(4).toString())
                .arg(mqry.value(5).toString())
                .arg(mqry.value(6).toString())
                .arg(mqry.value(7).toString().replace(QChar(39), QChar(8217)))
                .arg(mqry.value(9).toString())
                .arg(chktime)
                .arg(mqry.value(10).toString())
                .arg(upTime.toMSecsSinceEpoch() / 1000);
        sqls << sql;
    }

    //细表
    sql = QStringLiteral("select sheetID, cargo, color, actPrice, "
                          "sum(qty), sum(actMoney), first(rowMark), %1 from %2dtl "
                          "group by sheetID, cargo, color, actPrice;").arg(oldSizeCols).arg(oldSheet);
    mqry.exec(sql);
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();

    //R15行遍历
    qint64 rowTime = QDateTime::currentMSecsSinceEpoch();
    while ( mqry.next() ) {

        int sheetId = mqry.value(0).toInt();
        QString cargo = mqry.value(1).toString().trimmed();  //必须trimmed
        QString color = mqry.value(2).toString();
        double actPrice = mqry.value(3).toDouble();
        int actQty = mqry.value(4).toInt();
        double actMoney = mqry.value(5).toDouble();
        QString rowMark = mqry.value(6).toString().replace(QChar(39), QChar(8217));
        double setprice = (mapSetPrice.contains(cargo)) ? mapSetPrice.value(cargo) : 1000.0;
        double discount = actPrice / setprice;
        double disMoney = (1-discount) * (setprice * actQty);
        rowTime += 100;

        //求具体使用哪个尺码品类
        int vecIdx = ( mapSizerType.contains(cargo) ) ? mapSizerType.value(cargo) : vecSizerType.length() - 1;
        QStringList useSizeCols = vecSizerType.at(vecIdx).second;

        //因为R15缺陷，有的qty列与各SZ列之和并不相等，故重求
        int sumQty = 0;
        for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {
            sumQty += mqry.value(6 + i).toInt();
        }
        bool bugRow = (sumQty != actQty);

        //尺码遍历求sizers
        QStringList sizers;
        for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {

            int qty = mqry.value(6 + i).toInt();

            if ( qty != 0 ) {

                //求和BUG行处理
                if ( bugRow ) {

                    //以合计行为准（现在就退出）
                    if ( actQty == 0 )
                        break;

                    //只将总数放一个尺码，后面break退出for循环，不再处理其它尺码
                    if ( actQty != 0 ) {
                        qty = actQty;
                    }
                }

                QString sizer = useSizeCols.at(i - 1);
                sizers << QStringLiteral("%1\t%2").arg(sizer).arg(qty * 10000);

                //已经全放一个尺码了
                if ( bugRow && actQty != 0 )
                    break;
            }
        }

        //SQL   （教训：values()中逗号后慎加空格，因为sqlite奇葩整形会存储字符串型！）
        QString sql = QStringLiteral("insert into %1dtl(parentid, rowtime, cargo, color, sizers, "
                                     "price, qty, discount, actMoney, disMoney, rowMark) "
                                     "values(%2,%3,'%4','%5','%6',%7,%8,%9,%10,%11,'%12');")
                .arg(newSheet)
                .arg(sheetId)
                .arg(rowTime)
                .arg(cargo)
                .arg(color)
                .arg(sizers.join(QChar(10)))
                .arg(bsNumForSave(actPrice))
                .arg(actQty * 10000)
                .arg(bsNumForSave(discount))
                .arg(bsNumForSave(actMoney))
                .arg(bsNumForSave(disMoney))
                .arg(rowMark);

        sqls << sql;

    }

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::importStockAsInitSYD(QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);
    QString oldSizeCols = getOldSizeColSum(QString());
    QStringList stocks;
    int sheetId = 0;

    //
    QString sql = QStringLiteral("select stock from qUKC group by stock having sum(qty)<>0;");
    mqry.exec(sql);
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        stocks << mqry.value(0).toString();
    }
    mqry.finish();

    //
    foreach (QString stock, stocks) {

        //主表
        sheetId++;
        QDate dateD = QDateTime::currentDateTime().date();
        qint64 nowTime = QDateTime::currentMSecsSinceEpoch();
        QString sql = QStringLiteral("insert into syd("
                                     "sheetID, proof, dateD, shop, trader, stype, staff, remark, "
                                     "checker, chktime, upMan, upTime) "
                                     "values(%1, '导入期初', %2, '%3', '导入期初', '导入期初', '系统', '', "
                                     "'系统', %4, '系统', %4);")
                .arg(sheetId).arg(QDateTime(dateD).toMSecsSinceEpoch() / 1000).arg(stock).arg(nowTime);
        sqls << sql;

        //细表
        sql = QStringLiteral("select cargo, color, sum(qty), %1 from qUKC "
                             "where stock='%2' "
                             "group by cargo, color "
                             "having sum(qty)<>0;").arg(oldSizeCols).arg(stock);
        mqry.exec(sql);
        if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();

        //R15行遍历
        while ( mqry.next() ) {

            QString cargo = mqry.value(0).toString().trimmed();     //必须trimmed
            QString color = mqry.value(1).toString();
            int actQty = mqry.value(2).toInt();

            //求具体使用哪个尺码品类
            int vecIdx = ( mapSizerType.contains(cargo) ) ? mapSizerType.value(cargo) : vecSizerType.length() - 1;
            QStringList useSizeCols = vecSizerType.at(vecIdx).second;

            //因为R15缺陷，有的qty列与各SZ列之和并不相等，故重求
            int sumQty = 0;
            for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {
                sumQty += mqry.value(2 + i).toInt();
            }
            bool bugRow = (sumQty != actQty);

            //尺码遍历求sizers
            QStringList sizers;
            for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {

                int qty = mqry.value(2 + i).toInt();

                if ( qty != 0 ) {

                    //求和BUG行处理
                    if ( bugRow ) {

                        //以合计行为准（现在就退出）
                        if ( actQty == 0 )
                            break;

                        //只将总数放一个尺码，后面break退出for循环，不再处理其它尺码
                        if ( actQty != 0 ) {
                            qty = actQty;
                        }
                    }

                    QString sizer = useSizeCols.at(i - 1);
                    sizers << QStringLiteral("%1\t%2").arg(sizer).arg(qty * 10000);

                    //已经全放一个尺码了
                    if ( bugRow && actQty != 0 )
                        break;
                }
            }

            //SQL   （教训：sql insert 语句 values()中逗号后慎加空格，因为sqlite奇葩整形会存储字符串型！）
            QString sql = QStringLiteral("insert into syddtl(parentid, rowtime, cargo, color, sizers, "
                                         "price, qty, discount, actMoney, disMoney) "
                                         "values(%1,%2,'%3','%4','%5',0,%6,0,0,0);")
                    .arg(sheetId)
                    .arg(nowTime++)
                    .arg(cargo)
                    .arg(color)
                    .arg(sizers.join(QChar(10)))
                    .arg(actQty * 10000);

            sqls << sql;

        }
    }

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::updateSheetSum(const QString &sheet, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery newqry(newdb);

    //更新主表合计
    newqry.exec(QStringLiteral("select parentid, sum(qty) as sqty, sum(actMoney) as mny, sum(dismoney) as sumdis "
                               "from %1dtl group by parentid;").arg(sheet));
    if ( newqry.lastError().isValid() ) return newqry.lastError().text() + "\n" + newqry.lastQuery();
    while ( newqry.next() ) {
        sqls <<  QStringLiteral("update %1 set sumQty=%2, sumMoney=%3, sumdis=%4 where sheetID=%5;")
                 .arg(sheet)
                 .arg(newqry.value(1).toInt())
                 .arg(newqry.value(2).toLongLong())
                 .arg(newqry.value(3).toLongLong())
                 .arg(newqry.value(0).toString());
    }

    //更新主表实付款
    sqls << QStringLiteral("update %1 set actPay=sumMoney;").arg(sheet);

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::saveDebugTestData(QSqlDatabase &newdb)
{
    QStringList sqls;
    sqls << QStringLiteral("insert into barcoderule(barcodexp, sizermiddlee, barcodemark) "
                           "values('(\\w{1,8})-(\\w{1,4})-(\\D{1,6})', 0, 'COLUMNBIA');")
         << QStringLiteral("insert into barcoderule(barcodexp, sizermiddlee, barcodemark) "
                           "values('(\\D{2}\\d{4})(\\d{2})(\\d{3})', 0, '克林斯顿');")
         << QStringLiteral("insert into barcoderule(barcodexp, sizermiddlee, barcodemark) "
                           "values('(\\d\\D\\d{2})(\\d{3})(\\d)', 0, '美丽美');")
         << QStringLiteral("insert into barcoderule(barcodexp, sizermiddlee, barcodemark) "
                           "values('(\\d{4}\\D)(\\d{3})(\\d{3})', 0, '如花');");

    return batchExec(sqls, newdb);
}

QString BsImportR15Dlg::getOldSizeColSel(const QString &prefix)
{
    QStringList sl;
    for ( int i = 1, iLen = mOldSizeColCount; i <= iLen; ++i ) {
        sl << QStringLiteral("%1SZ%2").arg(prefix).arg(i, 2, 10, QLatin1Char('0'));
    }
    return sl.join(QChar(44));
}

QString BsImportR15Dlg::getOldSizeColSum(const QString &prefix)
{
    QStringList sl;
    for ( int i = 1, iLen = mOldSizeColCount; i <= iLen; ++i ) {
        sl << QStringLiteral("SUM(%1SZ%2) AS %2").arg(prefix).arg(i, 2, 10, QLatin1Char('0'));
    }
    return sl.join(QChar(44));
}

QString BsImportR15Dlg::batchExec(const QStringList &sqls, QSqlDatabase &newdb)
{
    QString sql;
    QSqlQuery newqry(newdb);

    newdb.transaction();
    foreach (sql, sqls) {
        if ( sql.trimmed().length() > 1 ) {
            newqry.exec(sql);
            if ( newqry.lastError().isValid() ) {
                newdb.rollback();
                return newqry.lastError().text() + "\n" + sql;
            }
        }
    }
    newdb.commit();

    return "";
}


// BsR15UpgSetGrid ============================================================================
BsR15UpgSetGrid::BsR15UpgSetGrid(QWidget *parent) : QTableWidget(parent)
{
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(20);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setVisible(true);
    horizontalHeader()->setStyleSheet("QHeaderView {border-style:none; border-bottom:1px solid #ccc;}");
}

void BsR15UpgSetGrid::commitData(QWidget *editor)
{
    QLineEdit *edt = qobject_cast<QLineEdit*>(editor);
    if ( edt )
    {
        QString txt = edt->text();
        txt.replace(QStringLiteral("，"), QStringLiteral(","));
        if ( currentColumn() == 0 ) {
            txt.replace(QChar(32), QString());
        }
        edt->setText(txt);
    }
    QTableWidget::commitData(editor);
}


// BsR15UpgColorGrid ============================================================================
BsR15UpgColorGrid::BsR15UpgColorGrid(QWidget *parent) : BsR15UpgSetGrid(parent)
{
    setAcceptDrops(true);
}

void BsR15UpgColorGrid::dragEnterEvent(QDragEnterEvent *e)
{
    e->accept();
}

void BsR15UpgColorGrid::dragMoveEvent(QDragMoveEvent *e)
{
    e->accept();
}

void BsR15UpgColorGrid::dropEvent(QDropEvent *e)
{
    QString text = e->mimeData()->text().trimmed();
    if ( text.isEmpty() )
        return;

    QModelIndex idx = indexAt(e->pos());
    if ( idx.isValid() ) {
        QTableWidgetItem *it = item(idx.row(), 0);
        if ( !it ) {
            it = new QTableWidgetItem(text);
            setItem(idx.row(), 0, it);
        }

        if (it->text().trimmed().isEmpty()) {
            it->setText(text);
        }
        else {
            it->setText(it->text() + QStringLiteral(",%1").arg(text));
        }
    }
    else {
        int rows = rowCount() + 1;
        setRowCount(rows);
        setItem(rows - 1, 0, new QTableWidgetItem(text));
    }

    e->accept();
}


// BsR15UpgColorList ============================================================================
BsR15UpgColorList::BsR15UpgColorList(QWidget *parent) : QListWidget(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
}

void BsR15UpgColorList::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        mStartPos = e->pos();
    QListWidget::mousePressEvent(e);
}

void BsR15UpgColorList::mouseMoveEvent(QMouseEvent *e)
{
    if ( !currentItem() ) {
        QListWidget::mouseMoveEvent(e);
        return;
    }

    if (e->buttons() & Qt::LeftButton){
        int distance = (e->pos() - mStartPos).manhattanLength();
        if (distance >= QApplication::startDragDistance()) {

            QMimeData *mimeData = new QMimeData;
            mimeData->setText( currentItem()->text() );

            QRect rect = visualItemRect(currentItem());
            QPixmap pixmap(rect.width(), rect.height());
            QPainter painter(&pixmap);
            painter.end();
            render(&pixmap, QPoint(0, 0), QRegion(QRect(1, rect.y() + 2, rect.width() + 1, rect.height() + 1)));

            QDrag *drag = new QDrag(this);
            drag->setMimeData(mimeData);
            drag->setPixmap(pixmap);
            drag->setHotSpot(QPoint(2, 2));

            uint dragResult = drag->exec();
            if ( dragResult ) {
                currentItem()->setData(Qt::UserRole, true);
                currentItem()->setForeground(Qt::darkGreen);
            }
        }
    }
    QListWidget::mouseMoveEvent(e);
}


}
