#include "bsimportr16dlg.h"
#include "quuid.h"
#include "comm/pinyincode.h"
#include "main/bailisql.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailifunc.h"

#include <QRegularExpression>

#define COLOR_BIND_FLAG    QStringLiteral("本色")

#define IMPORT_HELP     "升迁建账过程为——创建新的库文件，并从老的数据文件中导入数据，同时改变为新的数据格式，\
以适应新的程序。因此，升迁建账完全不影响原数据文件。如果不满意，可以仍然继续使用原R16系统。因此，请放心操作甚至\
多次练习。"

#define REGEXP_HELP     "<font color='#080'><b>正则表达式</b>简单语法：\\d通配数字，\\D通配字母，\\w通配字母或数字，\
前面不加反斜线为原字符不通配；几种可能用方括号列举，比如[ABC]表示A或B或C。<br/>\
字符种类识别后，还要识别位数。位数表示法是{x,y}的形式。\
x为至少位数，y为最多位数；固定位数用{x}表示；不限最多位数用{x,}表示。<br/>\
例如：<b>\\D{2}\\d{8}</b>表示两位字母后跟8位数字；\
<b>\\d{6,}-\\D\\d{2,4}-\\D{1,8}</b>表示最少6位数字后带减号再后面是一位字母跟2到4位数字，\
再又是减号，后面是1到8位数字。以此类推。</font>"

#define REGEXP_CAP_HELP "提取款式号与色号的方法就是，在正则表达式中，将表示款式号与色号的部分，分别用一对圆括号括起来。"


#define UIDX_CGNAME     0
#define UIDX_SETPRICE   1
#define UIDX_RETPRICE   2
#define UIDX_LOTPRICE   3
#define UIDX_NEWPRICE   4
#define UIDX_ATTR3      5
#define UIDX_ATTR4      6
#define UIDX_ATTR5      7
#define UIDX_ATTR6      8
#define UIDX_SIZETYPE   9
#define UIDX_DIRTY      10

/*
([0-9A-Z]+)([一-龥]*)
([0-9A-Z]+)-(\d+)
*/

namespace BailiSoft {

BsImportR16Dlg::BsImportR16Dlg(QWidget *parent, const QString &accessFile)
    : QDialog(parent), mAccessFile(accessFile), mAccessConn("MSAccessImportR16Conn"),
      mSqliteConn("SqliteImportR16Conn")
{
    //R16预备页
    grdR16Cargo = new QTableWidget(this);
    grdR16Cargo->setColumnCount(5);
    grdR16Cargo->setSelectionBehavior(QAbstractItemView::SelectItems);
    grdR16Cargo->setSelectionMode(QAbstractItemView::SingleSelection);
    grdR16Cargo->setHorizontalHeaderLabels(QStringList()
                                           << QStringLiteral("货号")
                                           << QStringLiteral("    款号    ")
                                           << QStringLiteral("    色号    ")
                                           << QStringLiteral("    品名    ")
                                           << QStringLiteral(" 标牌价 "));
    grdR16Cargo->verticalHeader()->setDefaultSectionSize(18);
    grdR16Cargo->horizontalHeader()->setStyleSheet("QHeaderView {border-style:none; border-bottom:1px solid #ccc;}");
    grdR16Cargo->setStatusTip(QStringLiteral("需要完成<b>码类</b>与<b>色系</b>设置。提示：利用左边色码两表的正则表达式测试功能可加速完成。"));
    grdR16Cargo->verticalHeader()->hide();

    grdR16Cargo->setColumnWidth(2, 80);
    grdR16Cargo->setColumnWidth(4, 60);
    grdR16Cargo->setMinimumSize(480, 400);
    connect(grdR16Cargo, &QTableWidget::itemDoubleClicked, this, &BsImportR16Dlg::gridDoubleClicked);

    mpEdtSql = new QTextEdit(this);

    mpBtnSql = new QPushButton(QStringLiteral("  execute for ready  "), this);
    connect(mpBtnSql, &QPushButton::clicked, this, &BsImportR16Dlg::clickSqlExecute);

    QLabel* lblR16Exp = new QLabel(QStringLiteral("从货号提取款号与色号的正则表达式："), this);
    int capWidth = 3 * lblR16Exp->sizeHint().width() / 2;
    lblR16Exp->setAlignment(Qt::AlignCenter);
    lblR16Exp->setFixedWidth(capWidth);

    mpR16Exp = new QLineEdit(this);
    mpR16Exp->setFixedWidth(capWidth);
    connect(mpR16Exp, SIGNAL(textChanged(QString)), this, SLOT(editR16ExpChanged()));

    mpBtnR16ExpTest = new QPushButton(QStringLiteral("测试"), this);
    connect(mpBtnR16ExpTest, SIGNAL(clicked(bool)), this, SLOT(clickR16ExpTest()));
    mpBtnR16ExpTest->setEnabled(false);

    mpBtnFetchColorName = new QPushButton(QStringLiteral("提取色名"), this);
    mpBtnFetchColorName->setToolTip(QStringLiteral("从品名中智能判断提取色名"));
    connect(mpBtnFetchColorName, &QPushButton::clicked, this, &BsImportR16Dlg::fetchColorName);
    mpBtnFetchColorName->setEnabled(false);

    mpBtnR16ExpRestore = new QPushButton(QStringLiteral("还原"), this);
    connect(mpBtnR16ExpRestore, SIGNAL(clicked(bool)), this, SLOT(clickR16ExpRestore()));
    mpBtnR16ExpRestore->setEnabled(false);

    mpBtnR16ExpApply = new QPushButton(QStringLiteral("确用"), this);
    connect(mpBtnR16ExpApply, SIGNAL(clicked(bool)), this, SLOT(clickR16ExpApply()));
    mpBtnR16ExpApply->setEnabled(false);

    QWidget* pnlR16ExpButtons = new QWidget(this);
    QHBoxLayout* layR16ExpButtons = new QHBoxLayout(pnlR16ExpButtons);
    layR16ExpButtons->addStretch();
    layR16ExpButtons->addWidget(mpBtnR16ExpTest);
    layR16ExpButtons->addWidget(mpBtnFetchColorName);
    layR16ExpButtons->addWidget(mpBtnR16ExpRestore);
    layR16ExpButtons->addWidget(mpBtnR16ExpApply);
    layR16ExpButtons->addStretch();

    mpLblR16ExpHelp = new QLabel(QStringLiteral(REGEXP_HELP), this);
    mpLblR16ExpHelp->setWordWrap(true);

    QLabel* lblR16HelpCap = new QLabel(QStringLiteral(REGEXP_CAP_HELP), this);
    lblR16HelpCap->setWordWrap(true);

    QPushButton *btnDealLeftSame = new QPushButton(QStringLiteral("余下货号款色不分"), this);
    connect(btnDealLeftSame, &QPushButton::clicked, this, &BsImportR16Dlg::dealLeftAsSameStyle);

    QPushButton *btnDealAllSame = new QPushButton(QStringLiteral("所有货号款色不分"), this);
    connect(btnDealAllSame, &QPushButton::clicked, this, &BsImportR16Dlg::dealAllAsSameStyle);

    QWidget* pnlDealSame = new QWidget(this);
    QHBoxLayout *layDealSame = new QHBoxLayout(pnlDealSame);
    layDealSame->addStretch();
    layDealSame->addWidget(btnDealLeftSame);
    layDealSame->addWidget(btnDealAllSame);
    layDealSame->addStretch();

    mpChkOnlyStock = new QCheckBox(QStringLiteral("仅导库存"), this);

    QPushButton* btnR16Next = new QPushButton(QStringLiteral("执行导入"), this);
    btnR16Next->setIcon(QIcon(":/icon/go.png"));
    btnR16Next->setFixedSize(120, 32);
    btnR16Next->setIconSize(QSize(24, 24));
    connect(btnR16Next, SIGNAL(clicked(bool)), this, SLOT(doImport()));

    QPushButton *btnR16Cancel = new QPushButton(mapMsg.value("btn_cancel"), this);
    btnR16Cancel->setFixedSize(70, 32);
    btnR16Cancel->setIconSize(QSize(24, 24));
    connect(btnR16Cancel, SIGNAL(clicked()), this, SLOT(reject()));

    QWidget* pnlR16StepButtons = new QWidget(this);
    QHBoxLayout* layR16StepButtons = new QHBoxLayout(pnlR16StepButtons);
    layR16StepButtons->addStretch();
    layR16StepButtons->addWidget(mpChkOnlyStock);
    layR16StepButtons->addWidget(btnR16Next);
    layR16StepButtons->addWidget(btnR16Cancel);
    layR16StepButtons->addStretch();

    QWidget* pnlR16Cap = new QWidget(this);
    QVBoxLayout *layR16Cap = new QVBoxLayout(pnlR16Cap);
    layR16Cap->addWidget(mpEdtSql, 3);
    layR16Cap->addWidget(mpBtnSql, 0, Qt::AlignCenter);
    layR16Cap->addSpacing(24);
    layR16Cap->addWidget(lblR16Exp, 0, Qt::AlignCenter);
    layR16Cap->addWidget(mpR16Exp, 0, Qt::AlignCenter);
    layR16Cap->addWidget(pnlR16ExpButtons);
    layR16Cap->addWidget(mpLblR16ExpHelp);
    layR16Cap->addWidget(lblR16HelpCap);
    layR16Cap->addWidget(pnlDealSame);
    layR16Cap->addStretch(1);
    layR16Cap->addWidget(pnlR16StepButtons);

    QSplitter *spl = new QSplitter(this);
    spl->setOrientation(Qt::Horizontal);
    spl->addWidget(grdR16Cargo);
    spl->addWidget(pnlR16Cap);

    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->addWidget(spl);

    //窗口
    setMinimumSize(900, 600);
    setWindowTitle(QStringLiteral("R16数据升迁工具"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);

    //信号
    connect(this, SIGNAL(destroyed(QObject*)), this, SLOT(overDestroy()));

    //打开老数据
    initSourceR16Data();
    setMinimumSize(800, 600);
}

void BsImportR16Dlg::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() != Qt::Key_Enter && e->key() != Qt::Key_Return && e->key() != Qt::Key_Escape )
        QDialog::keyPressEvent(e);
}

void BsImportR16Dlg::clickSqlExecute()
{
    QSqlDatabase mdb = QSqlDatabase::database(mAccessConn);
    if (!mdb.isOpen()) {
        mdb.open();
        if ( mdb.lastError().isValid() ) {
            QMessageBox::information(this, QString(), mdb.lastError().text());
            return;
        }
    }

    QString txt = mpEdtSql->toPlainText();
    if ( txt.trimmed().toLower().startsWith("select ") ) {

        QStringList lines;
        QSqlQuery qry(mdb);
        qry.exec(txt);
        QSqlRecord rec = qry.record();
        while ( qry.next() ) {
            QStringList cols;
            for ( int i = 0, iLen = rec.count(); i < iLen; ++i ) {
                cols << qry.value(i).toString();
            }
            lines << cols.join(QChar('\t'));
        }

        QMessageBox::information(this, QString(), lines.join(QChar('\n')));
        return;
    }


    QStringList sqls = mpEdtSql->toPlainText().split(QStringLiteral(";\n"));

    QSqlQuery qry(mdb);
    mdb.transaction();
    foreach (QString s, sqls) {
        QString sql = s.trimmed();
        if ( sql.length() > 10 )
        {
            qry.exec(sql);
            if ( qry.lastError().isValid() )
            {
                mdb.rollback();
                qDebug() << qry.lastError().text();
                qDebug() << sql;
                QMessageBox::information(this, QString(), QStringLiteral("%1\n%2").arg(qry.lastError().text()).arg(sql));
                return;
            }
        }
    }
    mdb.commit();

    QMessageBox::information(this, QString(), QStringLiteral("Done!"));
}

void BsImportR16Dlg::clickR16ExpTest()
{
    QString regExpString = QChar('^') + mpR16Exp->text() + QChar('$');
    QRegularExpression regExp(regExpString);
    if ( !regExp.isValid() ) {
        QMessageBox::information(this, QString(), QStringLiteral("无效的正则表达式%1。错误码：%2")
                                 .arg(regExpString).arg(regExp.errorString()));
        return;
    }

    int count = 0;
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        QString oldCargo = grdR16Cargo->item(i, 0)->text();
        QRegularExpressionMatch match = regExp.match(oldCargo);

        if (match.hasMatch()) {
            QString whole = match.captured(0);
            QString cargo = match.captured(1);
            QString color = match.captured(2);
            QString sp3 = match.captured(3);

            if ( whole == oldCargo ) {
                if ( !cargo.isEmpty() && !color.isEmpty() && sp3.isEmpty() ) {
                    grdR16Cargo->item(i, 0)->setData(Qt::UserRole + UIDX_DIRTY, true);

                    grdR16Cargo->item(i, 1)->setText(cargo);
                    grdR16Cargo->item(i, 1)->setForeground(Qt::red);

                    grdR16Cargo->item(i, 2)->setText(color);
                    grdR16Cargo->item(i, 2)->setForeground(Qt::red);

                    count++;
                }
                else if (!cargo.isEmpty() && !color.isEmpty() && !sp3.isEmpty()) {
                    grdR16Cargo->item(i, 0)->setData(Qt::UserRole + UIDX_DIRTY, true);

                    grdR16Cargo->item(i, 1)->setText(cargo + sp3);
                    grdR16Cargo->item(i, 1)->setForeground(Qt::red);

                    grdR16Cargo->item(i, 2)->setText(color);
                    grdR16Cargo->item(i, 2)->setForeground(Qt::red);

                    count++;
                }
            }
        }
    }

    mpBtnR16ExpRestore->setEnabled(count > 0);
    mpBtnFetchColorName->setEnabled(count > 0);
    mpBtnR16ExpApply->setEnabled(count > 0);

    if ( count > 0 )
        mpLblR16ExpHelp->setText(QStringLiteral("<font color='red'>匹配%1条如左表红色行</font>").arg(count));
    else
        mpLblR16ExpHelp->setText(QStringLiteral("<font color='red'>没有匹配，请检查表达式，重新测试。</font>"));
}

void BsImportR16Dlg::clickR16ExpRestore()
{
    restoreR16Test();
    mpR16Exp->clear();
    mpBtnR16ExpTest->setEnabled(false);
    mpBtnR16ExpRestore->setEnabled(false);
    mpBtnFetchColorName->setEnabled(false);
    mpBtnR16ExpApply->setEnabled(false);
}

void BsImportR16Dlg::clickR16ExpApply()
{
    applyR16Test();
    mpR16Exp->clear();
    mpBtnR16ExpTest->setEnabled(false);
    mpBtnR16ExpRestore->setEnabled(false);
    mpBtnFetchColorName->setEnabled(false);
    mpBtnR16ExpApply->setEnabled(false);
}

void BsImportR16Dlg::editR16ExpChanged()
{
    restoreR16Test();
    mpBtnR16ExpTest->setEnabled(!mpR16Exp->text().isEmpty());
    mpBtnR16ExpRestore->setEnabled(false);
    mpBtnFetchColorName->setEnabled(false);
    mpBtnR16ExpApply->setEnabled(false);
    mpLblR16ExpHelp->setText(QStringLiteral(REGEXP_HELP));
}

void BsImportR16Dlg::doImport()
{
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        if ( grdR16Cargo->item(i, 0)->data(Qt::UserRole + UIDX_DIRTY).toBool() ) {
            QMessageBox::information(this, QString(), QStringLiteral("有红色设置未确认！"));
            return;
        }
        if ( grdR16Cargo->item(i, 1)->text().trimmed().isEmpty()
             || grdR16Cargo->item(i, 2)->text().trimmed().isEmpty() ) {
            QMessageBox::information(this, QString(), QStringLiteral("货号“%1”未设置！")
                                     .arg(grdR16Cargo->item(i, 0)->text()));
            return;
        }
    }

    //货号字典
    mapGridRow.clear();
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        mapGridRow.insert( grdR16Cargo->item(i, 0)->text(), i);
    }

    //大过程
    qApp->setOverrideCursor(Qt::WaitCursor);
    QString strErr = importToSqlite(mSqliteFile);
    qApp->restoreOverrideCursor();

    //报告
    if ( strErr.isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("导入成功！"));
        accept();   //调用外部使用mSqliteFile放到账册列表
    } else
        mpLblR16ExpHelp->setText("导入失败！\n" + strErr);
}

void BsImportR16Dlg::overDestroy()
{
    QSqlDatabase::removeDatabase(mAccessConn);
}

void BsImportR16Dlg::fetchColorName()
{
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        QTableWidgetItem* itAttr2 = grdR16Cargo->item(i, 2);
        QString attr2 =  itAttr2->text();
        QString cgname = grdR16Cargo->item(i, 3)->text();
        QString ccode = fetchCodePrefix(attr2);

        if ( ccode.length() < attr2.length() ) continue;                    //已有色名的不处理
        if ( ! grdR16Cargo->item(i, 0)->data(Qt::UserRole + UIDX_DIRTY).toBool() ) continue; //已经确认过的不处理

        if ( cgname.contains(QStringLiteral("红")) || cgname.contains(QStringLiteral("黄"))
             || cgname.contains(QStringLiteral("橙")) || cgname.contains(QStringLiteral("桔"))
             || cgname.contains(QStringLiteral("绿")) || cgname.contains(QStringLiteral("青"))
             || cgname.contains(QStringLiteral("蓝")) || cgname.contains(QStringLiteral("兰"))
             || cgname.contains(QStringLiteral("紫")) || cgname.contains(QStringLiteral("黑"))
             || cgname.contains(QStringLiteral("白")) || cgname.contains(QStringLiteral("灰"))
             || cgname.contains(QStringLiteral("咖")) || cgname.contains(QStringLiteral("米"))
             || cgname.contains(QStringLiteral("褐")) || cgname.contains(QStringLiteral("棕"))
             || cgname.contains(QStringLiteral("粉")) || cgname.contains(QStringLiteral("花"))
             || cgname.contains(QStringLiteral("条")) || cgname.contains(QStringLiteral("格")) ) {

            QString color = ccode + cgname.left(2);
            itAttr2->setText(color);
        }
    }
}

void BsImportR16Dlg::dealLeftAsSameStyle()
{
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        QString cargo = grdR16Cargo->item(i, 0)->text();
        QTableWidgetItem* itAttr1 = grdR16Cargo->item(i, 1);

        if ( itAttr1->text().trimmed().isEmpty() ) {
            grdR16Cargo->item(i, 0)->setData(Qt::UserRole + UIDX_DIRTY, false);

            itAttr1->setText(cargo);
            itAttr1->setData(Qt::UserRole, cargo);
            itAttr1->setForeground(QBrush(Qt::black));

            QTableWidgetItem* itAttr2 = grdR16Cargo->item(i, 2);
            itAttr2->setText(COLOR_BIND_FLAG);
            itAttr2->setData(Qt::UserRole,  itAttr2->text());
            itAttr2->setForeground(QBrush(Qt::black));
        }
    }
}

void BsImportR16Dlg::dealAllAsSameStyle()
{
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        QString cargo = grdR16Cargo->item(i, 0)->text();
        QTableWidgetItem* itAttr1 = grdR16Cargo->item(i, 1);

        grdR16Cargo->item(i, 0)->setData(Qt::UserRole + UIDX_DIRTY, false);

        itAttr1->setText(cargo);
        itAttr1->setData(Qt::UserRole, cargo);
        itAttr1->setForeground(QBrush(Qt::black));

        QTableWidgetItem* itAttr2 = grdR16Cargo->item(i, 2);
        itAttr2->setText(COLOR_BIND_FLAG);
        itAttr2->setData(Qt::UserRole,  itAttr2->text());
        itAttr2->setForeground(QBrush(Qt::black));
    }
}

void BsImportR16Dlg::gridDoubleClicked(QTableWidgetItem *item)
{
    if ( item && item->column() == 0 ) {
        mpEdtSql->setText(mpEdtSql->toPlainText() + "\n" + item->text());
    }
}

void BsImportR16Dlg::applyR16Test()
{
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        QTableWidgetItem* itAttr1 = grdR16Cargo->item(i, 1);
        itAttr1->setData(Qt::UserRole, itAttr1->text());
        itAttr1->setForeground(QBrush(Qt::black));

        QTableWidgetItem* itAttr2 = grdR16Cargo->item(i, 2);
        itAttr2->setData(Qt::UserRole, itAttr2->text());
        itAttr2->setForeground(QBrush(Qt::black));

        grdR16Cargo->item(i, 0)->setData(Qt::UserRole + UIDX_DIRTY, false);
    }
}

void BsImportR16Dlg::restoreR16Test()
{
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        QTableWidgetItem* itAttr1 = grdR16Cargo->item(i, 1);
        itAttr1->setText(itAttr1->data(Qt::UserRole).toString());
        itAttr1->setForeground(QBrush(Qt::black));

        QTableWidgetItem* itAttr2 = grdR16Cargo->item(i, 2);
        itAttr2->setText(itAttr2->data(Qt::UserRole).toString());
        itAttr2->setForeground(QBrush(Qt::black));

        grdR16Cargo->item(i, 0)->setData(Qt::UserRole + UIDX_DIRTY, false);
    }
}

void BsImportR16Dlg::initSourceR16Data()
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
        QFileInfo fInfo(mAccessFile);
        mBookName = fInfo.baseName();

        //文件名
        QString fileBase = QFileInfo(mAccessFile).baseName() + QStringLiteral(".jyb");
        QDir dir(dataDir);
        mSqliteFile = dir.absoluteFilePath(fileBase);
    }
    else {
        qDebug() << mdb.lastError().text();
        QMessageBox::information(this, QString(), QStringLiteral("数据打开不成功，请确是有效的R16账册文件！\n"));
        return;
    }

    //开始读取表格老数据
    QSqlQuery qry(mdb);
    qry.setForwardOnly(true);
    qry.exec("select cargo, attr1, attr2, cgName, setPrice, retPrice, lotPrice, newPrice, "
             "attr3, attr4, attr5, attr6, sizeType from tcargo order by cargo;");
    int rows = 0;
    bool sizeTypeAllSet = true;
    while ( qry.next() ) {
        grdR16Cargo->setRowCount(++rows);
        QTableWidgetItem *itCargo = new QTableWidgetItem(qry.value(0).toString().toUpper());
        itCargo->setData(Qt::UserRole + UIDX_CGNAME, qry.value(3).toFloat());
        itCargo->setData(Qt::UserRole + UIDX_SETPRICE, qry.value(4).toFloat());
        itCargo->setData(Qt::UserRole + UIDX_RETPRICE, qry.value(5).toFloat());
        itCargo->setData(Qt::UserRole + UIDX_LOTPRICE, qry.value(6).toFloat());
        itCargo->setData(Qt::UserRole + UIDX_NEWPRICE, qry.value(7).toFloat());
        itCargo->setData(Qt::UserRole + UIDX_ATTR3, qry.value(8).toString());
        itCargo->setData(Qt::UserRole + UIDX_ATTR4, qry.value(9).toString());
        itCargo->setData(Qt::UserRole + UIDX_ATTR5, qry.value(10).toString());
        itCargo->setData(Qt::UserRole + UIDX_ATTR6, qry.value(11).toString());
        itCargo->setData(Qt::UserRole + UIDX_SIZETYPE, qry.value(12).toString());
        itCargo->setBackground(QColor(240, 240, 240));
        itCargo->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        QTableWidgetItem *itAttr1 = new QTableWidgetItem(qry.value(1).toString().toUpper());
        itAttr1->setData(Qt::UserRole, qry.value(1).toString());

        QTableWidgetItem *itAttr2 = new QTableWidgetItem(qry.value(2).toString().toUpper());
        itAttr2->setData(Qt::UserRole, qry.value(2).toString());

        QTableWidgetItem *itCgname = new QTableWidgetItem(qry.value(3).toString());
        itCgname->setData(Qt::UserRole, qry.value(3).toString());
        itCgname->setFlags(Qt::NoItemFlags);
        itCgname->setForeground(Qt::gray);

        QTableWidgetItem *itSetprice = new QTableWidgetItem(qry.value(4).toString());
        itSetprice->setData(Qt::UserRole, qry.value(4).toString());
        itSetprice->setFlags(Qt::NoItemFlags);
        itSetprice->setForeground(Qt::gray);

        grdR16Cargo->setItem(rows - 1, 0, itCargo);
        grdR16Cargo->setItem(rows - 1, 1, itAttr1);
        grdR16Cargo->setItem(rows - 1, 2, itAttr2);
        grdR16Cargo->setItem(rows - 1, 3, itCgname);
        grdR16Cargo->setItem(rows - 1, 4, itSetprice);

        if ( qry.value(12).toString().trimmed().isEmpty() ) sizeTypeAllSet = false;
    }

    if ( !sizeTypeAllSet ) {
        mpLblR16ExpHelp->setText(QStringLiteral("发现有R16数据中有货号的尺码未设置，请设置完全后再来导入！"));
        mpLblR16ExpHelp->setStyleSheet(QLatin1String("color:red;"));
    }
}

QString BsImportR16Dlg::importToSqlite(const QString &sqliteFile)
{
    //先检查删除库文件
    if ( QFile::exists(sqliteFile) ) {
        if ( ! QFile::remove(sqliteFile) ) {
            return QStringLiteral("%1文件被占用，无法删除，请关闭Sqlite工具！").arg(sqliteFile);
        }
    }

    //创建库文件并初始化为R18数据结构
    QString strErr;
    {
        //初始化新数据库表结构完整SQL
        QStringList sqls = BailiSoft::sqliteInitSqls(mBookName, true);

        //启动数据库连接
        QSqlDatabase sdb = QSqlDatabase::addDatabase("QSQLITE", mSqliteConn);
        sdb.setDatabaseName(sqliteFile);
        if ( sdb.open() ) {
            strErr = initNewSchema(sqls, sdb);
        } else {
            strErr = QStringLiteral("系统不支持Sqlite，%1创建不成功。错误信息：%2").arg(sqliteFile).arg(sdb.lastError().text());
        }

        //执行导入过程
        if ( strErr.isEmpty() ) {
            QSqlDatabase mdb = QSqlDatabase::database(mAccessConn);
            strErr = importTableByTable(mdb, sdb);
        }
    }

    //移除连接必须在addDatabase()作用域花括号外
    QSqlDatabase::removeDatabase(mSqliteConn);

    return strErr;
}

QString BsImportR16Dlg::initNewSchema(const QStringList &sqls, QSqlDatabase &newdb)
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

QString BsImportR16Dlg::importTableByTable(QSqlDatabase &mdb, QSqlDatabase &newdb)  //execute table by table
{
    //预备尺码相关数据
    QString strErr = prepareSizeMap(mdb);
    if ( !strErr.isEmpty() )
        return strErr;

    //以下逐表导入

    if ( strErr.isEmpty() )
        strErr = importSizerType(newdb);

    if ( strErr.isEmpty() )
        strErr = importColorType(newdb);

    if ( strErr.isEmpty() )
        strErr = importCargo(newdb);

    if ( strErr.isEmpty() )
        strErr = importStaff(mdb, newdb);

    if ( strErr.isEmpty() )
        strErr = importBaseRef("shop", mdb, newdb);

    if ( strErr.isEmpty() )
        strErr = importBaseRef("supplier", mdb, newdb);

    if ( strErr.isEmpty() )
        strErr = importBaseRef("customer", mdb, newdb);

    if ( mpChkOnlyStock->isChecked() ) {
        QString sql = "select stock, sum(qty) from qUKCbyDB group by stock having sum(qty)>0 order by stock;";
        QSqlQuery mqry(mdb);
        mqry.exec(sql);
        if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
        QStringList stocks;
        while ( mqry.next() ) { stocks << mqry.value(0).toString(); }
        mqry.finish();

        int newSheetId = 0;
        for ( int i = 0, iLen = stocks.length(); i < iLen; ++i ) {
            if ( strErr.isEmpty() ) {
                strErr = importStock(++newSheetId, stocks.at(i), mdb, newdb);
            }
        }
    }
    else {
        if ( strErr.isEmpty() )
            strErr = importSheet("DBD", "dbd", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("JHD", "cgj", mdb, newdb);

        if ( strErr.isEmpty() )
            strErr = importSheet("XSD", "lsd", mdb, newdb);
    }

    //刷新主表合计
    if ( strErr.isEmpty() )
        strErr = updateSheetSum("dbd", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("cgj", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("lsd", newdb);

    if ( strErr.isEmpty() )
        strErr = updateSheetSum("syd", newdb);

    //return
    return strErr;
}

QString BsImportR16Dlg::prepareSizeMap(QSqlDatabase &mdb)
{
    QSqlQuery qry(mdb);
    qry.setForwardOnly(true);
    qry.exec(QStringLiteral("select * from sysSizeType;"));
    mOldSizeColCount = (qry.record().count() - 9) / 2;
    qry.finish();
    if ( mOldSizeColCount < 2 ) {
        return QStringLiteral("读取R16账册sysSizeType列数不对！");
    }

    mapSizeType.clear();

    QString SZFields = getOldSizeColSel();  //必须在取得mOldSizeColCount之后
    QString sql = QStringLiteral("select sizeType, %1 from sysSizeType;").arg(SZFields);
    qry.exec(sql);
    while ( qry.next() ) {
        QString r16SizeTypeName = qry.value(0).toString();
        QStringList sizers;
        for ( int i = 0; i < mOldSizeColCount; ++i ) {
            sizers << qry.value(i + 1).toString();
        }
        mapSizeType.insert(r16SizeTypeName, sizers);
    }

    return QString();
}

QString BsImportR16Dlg::importSizerType(QSqlDatabase &newdb)
{
    QStringList sqls;

    QMapIterator<QString, QStringList>  it(mapSizeType);
    while ( it.hasNext() ) {
        it.next();
        QString tname = it.key();
        QStringList sizeList = it.value();
        sqls << QStringLiteral("insert into sizertype(tname, namelist) values('%1', '%2');")
                .arg(tname).arg(sizeList.join(QChar(',')));
    }

    return batchExec(sqls, newdb);
}

QString BsImportR16Dlg::importColorType(QSqlDatabase &newdb)
{
    /* 一律使用逗号列表式色号登记，故而忽略

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
    */

    Q_UNUSED(newdb);
    return QString();
}

QString BsImportR16Dlg::importCargo(QSqlDatabase &newdb)
{
    //合并各色，万一定价不同取大，品名不同取长
    QStringList cargos;
    qDeleteAll(mapCargo);
    mapCargo.clear();
    for ( int i = 0, iLen = grdR16Cargo->rowCount(); i < iLen; ++i ) {
        QString code = grdR16Cargo->item(i, 1)->text();
        QString color = grdR16Cargo->item(i, 2)->text();
        QString name = grdR16Cargo->item(i, 3)->text();
        double price = grdR16Cargo->item(i, 4)->text().toDouble();
        QString stype = grdR16Cargo->item(i, 0)->data(Qt::UserRole + UIDX_SIZETYPE).toString();

        if ( mapCargo.contains(code) ) {
            BsImpCargo *cargob = mapCargo.value(code);
            QString colorb = cargob->colors;
            QString nameb = cargob->hpname;
            double priceb = cargob->setPrice;

            cargob->colors = QStringLiteral("%1,%2").arg(colorb).arg(color);
            cargob->hpname = (nameb.length() > name.length()) ? nameb : name;
            cargob->setPrice = (priceb > price) ? priceb : price;
        }
        else {
            cargos << code;
            double setPrice = (price > 0.01) ? price : 1000.0;
            mapCargo.insert(code, new BsImpCargo(name, color, stype, setPrice, i));
        }
    }

    //构造登记R17货品sql
    QStringList sqls;
    for ( int i = 0, iLen = cargos.length(); i < iLen; ++i ) {
        QString hpcode = cargos.at(i);
        BsImpCargo *cargo = mapCargo.value(hpcode);
        QTableWidgetItem* it = grdR16Cargo->item(cargo->grdIndex, 0);

        sqls << QStringLiteral("insert into cargo(hpcode, hpname, sizertype, colortype, setPrice, retPrice, lotPrice, buyPrice, "
                        "attr1, attr2, attr3, attr4, upMan, upTime) values("
                        "'%1', '%2', '%3', '%4', %5, %6, %7, %8, '%9', '%10', '%11', '%12', 'admin', %13);")
                .arg(hpcode)
                .arg(cargo->hpname)
                .arg(cargo->sizetype)
                .arg(cargo->colors)
                .arg(bsNumForSave(cargo->setPrice))
                .arg(bsNumForSave(it->data(Qt::UserRole + UIDX_RETPRICE).toDouble()))
                .arg(bsNumForSave(it->data(Qt::UserRole + UIDX_LOTPRICE).toDouble()))
                .arg(bsNumForSave(it->data(Qt::UserRole + UIDX_NEWPRICE).toDouble()))
                .arg(it->data(Qt::UserRole + UIDX_ATTR3).toString())
                .arg(it->data(Qt::UserRole + UIDX_ATTR4).toString())
                .arg(it->data(Qt::UserRole + UIDX_ATTR5).toString())
                .arg(it->data(Qt::UserRole + UIDX_ATTR6).toString())
                .arg(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
    }

    return batchExec(sqls, newdb);
}

QString BsImportR16Dlg::importStaff(QSqlDatabase &mdb, QSqlDatabase &newdb)
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

QString BsImportR16Dlg::importBaseRef(const QString &newTable, QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);

    int defType = 1;
    if ( newTable == QStringLiteral("supplier") )
        defType = 2;
    if ( newTable == QStringLiteral("customer") )
        defType = 3;

    mqry.exec(QStringLiteral("select neter, tMan, tAdd, tTel from tNeter where defType=%1 and stopp=0;")
              .arg(defType));
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        sqls << QStringLiteral("insert into %1(kname, regdis, regman, regaddr, regtele, upMan, upTime) "
                        "values('%2', 10000, '%3', '%4', '%5', 'admin', %6);")
                .arg(newTable)
                .arg(mqry.value(0).toString())
                .arg(mqry.value(1).toString())
                .arg(mqry.value(2).toString())
                .arg(mqry.value(3).toString())
                .arg(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
    }

    return batchExec(sqls, newdb);
}

QString BsImportR16Dlg::importSheet(const QString &oldSheet, const QString &newSheet,
                            QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);
    QString oldSizeCols = getOldSizeColSum(QStringLiteral("%1dtl.").arg(oldSheet));

    //主表
    QString sql = QStringLiteral("select billID, proof, dateD, stock, trader, stype, staff, remark, "
                          "countt, countMan, countTime, upMan, upTime from %1;").arg(oldSheet);
    mqry.exec(sql);
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        QDate dateD = mqry.value(2).toDate();
        qint64 upTime = mqry.value(12).toDateTime().toMSecsSinceEpoch() / 1000;
        qint64 chktime = (mqry.value(8).toBool()) ? mqry.value(10).toDateTime().toMSecsSinceEpoch() / 1000 : 0;
        QString sql = QStringLiteral("insert into %1(sheetID, proof, dateD, shop, trader, stype, staff, remark, "
                              "checker, chktime, upMan, upTime) "
                              "values(%2, '%3', %4, '%5', '%6', '%7', '%8', '%9', '%10', %11, '%12', %13);")
                .arg(newSheet)
                .arg(mqry.value(0).toInt())
                .arg(mqry.value(1).toString())
                .arg(QDateTime(dateD).toMSecsSinceEpoch() / 1000)
                .arg(mqry.value(3).toString())
                .arg(mqry.value(4).toString())
                .arg(mqry.value(5).toString())
                .arg(mqry.value(6).toString())
                .arg(mqry.value(7).toString().replace(QChar(39), QStringLiteral("''")))
                .arg(mqry.value(9).toString())
                .arg(chktime)
                .arg(mqry.value(9).toString())
                .arg(upTime);
        sqls << sql;
    }

    //细表
    sql = QStringLiteral("select billID, cargo, actPrice, "
                          "sum(qty), sum(actMoney), first(rowMark), %1 from %2dtl "
                          "group by billID, cargo, actPrice;").arg(oldSizeCols).arg(oldSheet);
    mqry.exec(sql);
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();

    //R16行遍历
    qint64 rowTime = QDateTime::currentMSecsSinceEpoch();
    while ( mqry.next() ) {
        int sheetId = mqry.value(0).toInt();
        QString r16cargo = mqry.value(1).toString().toUpper();
        double actPrice = mqry.value(2).toDouble();
        int actQty = mqry.value(3).toInt();
        double actMoney = mqry.value(4).toDouble();
        QString rowMark = mqry.value(5).toString().replace(QChar(39), QString());
        rowTime += 100;

        QString r17cargo = r16cargo;
        QString r17color = r16cargo;
        QStringList useSizeCols;
        int gridRow = ( mapGridRow.contains(r16cargo) ) ? mapGridRow.value(r16cargo) : -1;
        if ( gridRow >= 0 ) {   //R16有登记
            r17cargo = grdR16Cargo->item(gridRow, 1)->text();
            r17color = grdR16Cargo->item(gridRow, 2)->text();
            QString sizeTypeName = grdR16Cargo->item(gridRow, 0)->data(Qt::UserRole + UIDX_SIZETYPE).toString();
            useSizeCols = mapSizeType.value(sizeTypeName);
        }
        else {                  //R16无登记
            r17cargo = r16cargo;
            r17color = QString();
            QString sizeTypeName = grdR16Cargo->item(0, 0)->data(Qt::UserRole + UIDX_SIZETYPE).toString();
            useSizeCols = mapSizeType.value(sizeTypeName);
        }
        double setprice = (mapCargo.contains(r17cargo)) ? mapCargo.value(r17cargo)->setPrice : 1000.0;
        double discount = actPrice / setprice;
        double disMoney = (1-discount) * (setprice * actQty);

        //因为R16缺陷，有的qty列与各SZ列之和并不相等，故重求
        int sumQty = 0;
        for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {
            sumQty += mqry.value(5 + i).toInt();
        }
        bool bugRow = (sumQty != actQty);

        //尺码遍历求sizers
        QStringList sizers;
        for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {

            int qty = mqry.value(5 + i).toInt();

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

        //SQL
        QString sql = QStringLiteral("insert into %1dtl(parentid, rowtime, cargo, color, sizers, price, qty, "
                                     "discount, actMoney, disMoney, rowMark) values("
                                     "%2, %3, '%4', '%5', '%6', %7, %8, %9, %10, %11, '%12');")
                .arg(newSheet)
                .arg(sheetId)
                .arg(rowTime)
                .arg(r17cargo)
                .arg(r17color)
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

QString BsImportR16Dlg::importStock(const int newSheetId, const QString &shop, QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);
    QString oldSizeCols = getOldSizeColSum(QStringLiteral("qUKCbyDB."));
    QString oldSizeHaving = getOldSizeColHaving(QStringLiteral("qUKCbyDB."));

    //主表
    QDate dt = QDate::currentDate();
    QString sql = QStringLiteral("insert into syd(sheetID, proof, dateD, shop, trader, stype, staff, remark, "
                                 "checker, chktime, upMan, upTime) "
                                 "values(%1, '升级期初', %2, '%3', '%3', '升级期初', '', '', '管理员', %2, '管理员', %2);")
            .arg(newSheetId)
            .arg(QDateTime(dt).toSecsSinceEpoch())
            .arg(shop);
    sqls << sql;

    //细表
    sql = QStringLiteral("select cargo, sum(qty), %1 from qUKCbyDB where stock='%2' "
                         "group by cargo HAVING %3;").arg(oldSizeCols).arg(shop).arg(oldSizeHaving);
    mqry.exec(sql);
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();

    //R16行遍历
    qint64 rowTime = QDateTime::currentMSecsSinceEpoch();
    while ( mqry.next() ) {
        QString r16cargo = mqry.value(0).toString().toUpper();
        int actQty = mqry.value(1).toInt();
        rowTime += 10;

        QString r17cargo = r16cargo;
        QString r17color = r16cargo;
        QStringList useSizeCols;
        int gridRow = ( mapGridRow.contains(r16cargo) ) ? mapGridRow.value(r16cargo) : -1;
        if ( gridRow >= 0 ) {   //R16有登记
            r17cargo = grdR16Cargo->item(gridRow, 1)->text();
            r17color = grdR16Cargo->item(gridRow, 2)->text();
            QString sizeTypeName = grdR16Cargo->item(gridRow, 0)->data(Qt::UserRole + UIDX_SIZETYPE).toString();
            useSizeCols = mapSizeType.value(sizeTypeName);
        } else {                //R16无登记
            r17cargo = r16cargo;
            r17color = QString();
            QString sizeTypeName = grdR16Cargo->item(0, 0)->data(Qt::UserRole + UIDX_SIZETYPE).toString();
            useSizeCols = mapSizeType.value(sizeTypeName);
        }

        //因为R16缺陷，有的qty列与各SZ列之和并不相等，故重求
        int sumQty = 0;
        for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {
            sumQty += mqry.value(1 + i).toInt();
        }
        bool bugRow = (sumQty != actQty);

        //尺码遍历求sizers
        QStringList sizers;
        for ( int i = 1, iLen = useSizeCols.length(); i <= iLen; ++i ) {

            int qty = mqry.value(1 + i).toInt();

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

        //SQL
        QString sql = QStringLiteral("insert into syddtl"
                                     "(parentid, rowtime, cargo, color, sizers, price, qty, discount, actMoney, disMoney) "
                                     "values(%1, %2, '%3', '%4', '%5', 0, %6, 0, 0, 0);")
                .arg(newSheetId)
                .arg(rowTime)
                .arg(r17cargo)
                .arg(r17color)
                .arg(sizers.join(QChar(10)))
                .arg(actQty * 10000);

        sqls << sql;

    }

    return batchExec(sqls, newdb);
}

QString BsImportR16Dlg::updateSheetSum(const QString &sheet, QSqlDatabase &newdb)
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

QString BsImportR16Dlg::saveDebugTestData(QSqlDatabase &newdb)
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

QString BsImportR16Dlg::getOldSizeColSel(const QString &prefix)
{
    QStringList sl;
    for ( int i = 1, iLen = mOldSizeColCount; i <= iLen; ++i ) {
        sl << QStringLiteral("%1SZ%2").arg(prefix).arg(i, 2, 10, QLatin1Char('0'));
    }
    return sl.join(QChar(44));
}

QString BsImportR16Dlg::getOldSizeColSum(const QString &prefix)
{
    QStringList sl;
    for ( int i = 1, iLen = mOldSizeColCount; i <= iLen; ++i ) {
        sl << QStringLiteral("SUM(%1SZ%2) AS %2").arg(prefix).arg(i, 2, 10, QLatin1Char('0'));
    }
    return sl.join(QChar(44));
}

QString BsImportR16Dlg::getOldSizeColHaving(const QString &prefix)
{
    QStringList sl;
    for ( int i = 1, iLen = mOldSizeColCount; i <= iLen; ++i ) {
        sl << QStringLiteral("SUM(%1SZ%2)<>0").arg(prefix).arg(i, 2, 10, QLatin1Char('0'));
    }
    return sl.join(QStringLiteral(" OR "));
}

QString BsImportR16Dlg::batchExec(const QStringList &sqls, QSqlDatabase &newdb)
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

    return QString();
}

}
