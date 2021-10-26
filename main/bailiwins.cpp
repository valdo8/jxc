#include "bailiwins.h"
#include "bailicode.h"
#include "bailidata.h"
#include "bailiedit.h"
#include "bailigrid.h"
#include "bailifunc.h"
#include "bailicustom.h"
#include "baililabel.h"
#include "bailidialog.h"
#include "comm/bsflowlayout.h"
#include "comm/pinyincode.h"
#include "misc/bsimportregdlg.h"
#include "misc/lxbzprinter.h"
#include "misc/lxbzprintsetting.h"
#include "misc/bsalarmsetting.h"
#include "misc/bshistorywin.h"
#include "misc/bsmdiarea.h"
#include "misc/bsfielddefinedlg.h"
#include "dialog/bsbarcodesimportdlg.h"
#include "dialog/bspickdatedlg.h"
#include "dialog/bslicwarning.h"
#include "dialog/bscopyimportsheetdlg.h"
#include "dialog/bscreatecolortype.h"
#include "dialog/bsshoplocdlg.h"
#include "dialog/bstagselectdlg.h"
#include <iostream>
#include <QNetworkRequest>
#include <QNetworkReply>

// TODO LIST
//开关盒：关闭条码扫描提示音
//开关盒：（收支单）保存时是否强制检查收支平衡
//工具箱：合并同货同色同价行
//工具箱：当前行使用客商折扣(Ctrl+Z)、全单使用客商折扣(Ctrl+Shif+Z)

namespace BailiSoft {

// BsWin
BsWin::BsWin(QWidget *parent, const QString &mainTable, const QStringList &fields, const uint bsWinType)
    : QWidget(parent), mMainTable(mainTable), mBsWinType(bsWinType), mGuideTipSwitch(false)
{
    //用于主窗口查找判断
    setProperty(BSWIN_TABLE, mainTable);

    //主窗
    mppMain = qobject_cast<QMainWindow*>(parent);
    Q_ASSERT(mppMain);

    //表格指针
    mpGrid              = nullptr;
    mpQryGrid           = nullptr;
    mpFormGrid          = nullptr;
    mpRegGrid           = nullptr;
    mpSheetGrid         = nullptr;
    mpSheetCargoGrid    = nullptr;
    mpSheetFinanceGrid  = nullptr;

    //常用变量
    mDiscDots = mapOption.value("dots_of_discount").toInt();
    mPriceDots = mapOption.value("dots_of_price").toInt();
    mMoneyDots = mapOption.value("dots_of_money").toInt();

    //字段表（继承类中可以用getFieldByName()获取字段指针以修改具体特别设置，比如不同的trader名称等）
    for ( int i = 0, iLen = fields.length(); i < iLen; ++i )
    {
        QString fld = fields.at(i);
        QStringList defs = mapMsg.value(QStringLiteral("fld_%1").arg(fld)).split(QChar(9));
        Q_ASSERT(defs.count() > 4);

        int fldLen = QString(defs.at(4)).toInt();
        if ( fld == QStringLiteral("kname") && mainTable == QStringLiteral("subject") ) fldLen = 100;
        if ( fld == QStringLiteral("subject") ) fldLen = 100;
        BsField *bsFld = new BsField(fld,
                                     defs.at(0),
                                     QString(defs.at(3)).toUInt(),
                                     fldLen,
                                     defs.at(2));
        resetFieldDotsDefine(bsFld);
        mFields << bsFld;

        //用户定义名称
        QString defKey = QStringLiteral("%1_%2").arg(mainTable).arg(fld);
        if ( mapFldUserSetName.contains(defKey) )
            bsFld->mFldCnName = mapFldUserSetName.value(defKey);
    }

    //补设attr字段中文名
    QString attrObj = ( mainTable.contains(QStringLiteral("szd")) || mainTable == QStringLiteral("subject"))
            ? QStringLiteral("subject") : QStringLiteral("cargo");
    for ( int i = 1, iLen = mFields.length(); i < iLen; ++i ) {
        BsField* fld = mFields.at(i);
        if ( fld->mFldName.startsWith(QStringLiteral("attr")) ) {
            QString optkey = QStringLiteral("%1_%2_name").arg(attrObj).arg(fld->mFldName);
            QString optval = mapOption.value(optkey);
            fld->mFldCnName = optval;
        }
    }

    //后代公用数据源
    if ( bsWinType != bswtMisc && bsWinType != bswtReg ) {

        //mpDsStype
        QString baseTable = mainTable;
        if ( mainTable.startsWith("vi_") ) {  //以下根据视图命名规则硬编码
            QStringList nameParts = mainTable.split(QChar('_'));
            QString vtableName = nameParts.at(1);
            if ( vtableName.length() == 3 ) {
                baseTable = vtableName;
            } else {
                baseTable = "invalidGroupConditionLetNoSelect";
            }
        }
        QString stypeListSql = QStringLiteral("select vsetting from bailioption where optcode='stypes_%1';").arg(baseTable);
        mpDsStype = new BsListModel(this, stypeListSql);
        mpDsStype->reload();

        //mpDsStaff
        QString staffListSql = QStringLiteral("select kname from staff");
        if ( mMainTable.toLower().startsWith("cg") ) {
            staffListSql += QStringLiteral(" where cancg<>0;");
        }
        else if ( mMainTable.toLower().startsWith("pf") ) {
            staffListSql += QStringLiteral(" where canpf<>0;");
        }
        else if ( mMainTable.toLower().startsWith("ls") ) {
            staffListSql += QStringLiteral(" where canls<>0;");
        }
        else if ( mMainTable.toLower().startsWith("db") ) {
            staffListSql += QStringLiteral(" where candb<>0;");
        }
        else if ( mMainTable.toLower().startsWith("xs") ) {
            staffListSql += QStringLiteral(" where canpf<>0 or canls<>0;");
        }
        else if ( mMainTable.toLower().startsWith("sy") ) {
            staffListSql += QStringLiteral(" where cansy<>0;");
        }

        mpDsStaff = new BsSqlModel(this, staffListSql);
        mpDsStaff->reload();

        //mpDsTrader
        if ( mMainTable.contains("cg") )
            mpDsTrader = dsSupplier;
        else if ( mMainTable.contains("pf") || mMainTable.contains("xs") || mMainTable.contains("lsd") )
            mpDsTrader = dsCustomer;
        else if ( mMainTable.contains(QStringLiteral("szd")) )
            mpDsTrader = new BsSqlModel(this, QStringLiteral("select kname from supplier union all "
                                                             "select kname from customer order by kname;"));
        else
            mpDsTrader = dsShop;

        mpDsTrader->reload();
    }

    //向导条
    mpGuide = new QLabel(this);
    mpGuide->setTextFormat(Qt::RichText);
    mpGuide->setAlignment(Qt::AlignCenter);
    mpGuide->setStyleSheet("QLabel{background-color:#ff8;}");

    //通知条
    mpPnlMessage = new QWidget(this);
    mpPnlMessage->setStyleSheet(QLatin1String(".QWidget{background-color:#bb0;}"));
    QVBoxLayout *layMessage = new QVBoxLayout(mpPnlMessage);

    int pt = qApp->font().pointSize();
    mpLblMessage = new QLabel(this);
    mpLblMessage->setAlignment(Qt::AlignCenter);
    mpLblMessage->setStyleSheet(QStringLiteral("color:red;font-size:%1pt").arg(5 * pt / 4));
    mpLblMessage->setWordWrap(true);

    mpBtnMessage = new QPushButton(mapMsg.value("word_i_have_know"), this);
    mpBtnMessage->setMinimumSize(90, 30);
    mpBtnMessage->setStyleSheet(QLatin1String("background-color:red; color:white; font-weight:900; "
                                              "border:3px solid white; border-radius:8px;"));
    connect(mpBtnMessage, SIGNAL(clicked(bool)), this, SLOT(hideFoceMessage()));

    layMessage->addWidget(mpLblMessage, 1);
    layMessage->addWidget(mpBtnMessage, 0, Qt::AlignCenter);
    mpPnlMessage->hide();

    //工具条基本结构
    mpMenuToolCase = new QMenu(this);
    mpMenuOptionBox = new QMenu(this);

    QToolButton *btnToolCase = new QToolButton(this);
    btnToolCase->setMenu(mpMenuToolCase);
    btnToolCase->setIcon(QIcon(":/icon/tool.png"));
    btnToolCase->setMinimumWidth(72);
    btnToolCase->setText(mapMsg.value("btn_toolcase").split(QChar(9)).at(0));
    btnToolCase->setStatusTip(mapMsg.value("btn_toolcase").split(QChar(9)).at(1));
    btnToolCase->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnToolCase->setPopupMode(QToolButton::InstantPopup);

    QToolButton *btnOptionBox = new QToolButton(this);
    btnOptionBox->setMenu(mpMenuOptionBox);
    btnOptionBox->setIcon(QIcon(":/icon/option.png"));
    btnOptionBox->setMinimumWidth(72);
    btnOptionBox->setText(mapMsg.value("btn_optionbox").split(QChar(9)).at(0));
    btnOptionBox->setStatusTip(mapMsg.value("btn_optionbox").split(QChar(9)).at(1));
    btnOptionBox->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnOptionBox->setPopupMode(QToolButton::InstantPopup);

    mpToolBar = new QToolBar(this);
    mpToolBar->setFloatable(false);
    mpToolBar->setMovable(false);
    mpToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpToolBar->setIconSize(QSize(48, 32));

    mpAcToolSeprator = mpToolBar->addSeparator();
    QAction* acToolCase = mpToolBar->addWidget(btnToolCase);
    QAction* acOptionBox = mpToolBar->addWidget(btnOptionBox);
    mpToolBar->addSeparator();
    mpAcMainHelp = mpToolBar->addAction(QIcon(":/icon/help.png"), mapMsg.value("btn_help").split(QChar(9)).at(0),
                                    this, SLOT(clickHelp()));

    //工具箱
    mpAcToolExport = mpMenuToolCase->addAction(QIcon(":/icon/export.png"),
                                           mapMsg.value("tool_export"),
                                           this, SLOT(clickToolExport()));

    //开关盒
    mpAcOptGuideNotShowAnymore = mpMenuOptionBox->addAction(QIcon(),
                                                            mapMsg.value("opt_dont_show_guide_anymore"),
                                                            this, SLOT(clickOptGuideNotShowAnymore()));
    mpAcOptGuideNotShowAnymore->setProperty("optname", "opt_dont_show_guide_anymore");

    mpAcMainHelp->setProperty(BSACFLAGS, 0);
    mpAcToolExport->setProperty(BSACFLAGS, bsacfClean | bsacfPlusId);
    mpAcOptGuideNotShowAnymore->setProperty(BSACFLAGS, 0);

    acToolCase->setProperty(BSACRIGHT, true);
    acOptionBox->setProperty(BSACRIGHT, true);
    mpAcMainHelp->setProperty(BSACRIGHT, true);
    mpAcOptGuideNotShowAnymore->setProperty(BSACRIGHT, true);

    //主体   
    mpBody = new QWidget(this);
    mpBody->setObjectName(QStringLiteral("mpBody"));
    mpBody->setStyleSheet(QLatin1String("QWidget#mpBody{background-color:white;}"));

    mpTaber = new QTabWidget(this);
    mpTaber->setMinimumHeight(mpTaber->tabBar()->sizeHint().height());
    mpTaber->setStyleSheet(QLatin1String("QTabWidget::pane {position:absolute; border-top:1px solid #ddd;} "
                                         "QTabWidget::tab-bar {alignment:center;}"));
    mpTaber->hide();

    mpSpl = new QSplitter(this);
    mpSpl->setOrientation(Qt::Vertical);
    mpSpl->addWidget(mpBody);
    mpSpl->addWidget(mpTaber);
    mpSpl->setCollapsible(0, false);
    mpSpl->setCollapsible(1, false);
    mpSpl->setStyleSheet(QLatin1String("QSplitter::handle:vertical {background-color:#ade; height:5px;}"));

    //状态条
    mpStatusBar = new QStatusBar(this);
    mpStatusBar->setStyleSheet("QStatusBar{border-style:none; border-bottom:1px solid silver; color:#160;}");

    //总布局
    QVBoxLayout *lay = new QVBoxLayout;
    lay->addWidget(mpGuide);
    lay->addWidget(mpPnlMessage);
    lay->addWidget(mpToolBar);
    lay->addWidget(mpSpl, 99);
    lay->addWidget(mpStatusBar);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    setLayout(lay);
    setWindowTitle(mapMsg.value(QStringLiteral("win_%1").arg(mainTable)).split(QChar(9)).at(0));
}

BsWin::~BsWin()
{
    qDeleteAll(mFields);
    mFields.clear();
}

BsField *BsWin::getFieldByName(const QString &name)
{
    for ( int i = 0, iLen = mFields.length(); i < iLen; ++i ) {
        BsField *fld = mFields.at(i);
        if ( fld->mFldName == name ) {
            return fld;
        }
    }
    return nullptr;
}

void BsWin::setQuickDate(const QString &periodName, BsFldBox *dateB, BsFldBox *dateE, QToolButton *button)
{
    QDate date = QDate::currentDate();
    qint64 mindv = QDateTime(date.addDays(-36500)).toMSecsSinceEpoch() / 1000;
    qint64 setdv;

    //今天
    if ( periodName.contains(mapMsg.value("menu_today")) ) {
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);

        dateE->mpEditor->setDataValue(setdv);
        button->setText(mapMsg.value("menu_today"));
        return;
    }

    //昨天
    if ( periodName.contains(mapMsg.value("menu_yesterday")) ) {
        date = date.addDays(-1);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);
        dateE->mpEditor->setDataValue(setdv);
        button->setText(mapMsg.value("menu_yesterday"));
        return;
    }

    //本周
    if ( periodName.contains(mapMsg.value("menu_this_week")) ) {
        while ( date.dayOfWeek() != 1 ) { date = date.addDays(-1); }
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);

        date = date.addDays(6);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        dateE->mpEditor->setDataValue(setdv);
        button->setText(mapMsg.value("menu_this_week"));
        return;
    }

    //上周
    if ( periodName.contains(mapMsg.value("menu_last_week")) ) {
        while ( date.dayOfWeek() != 1 ) { date = date.addDays(-1); }
        date = date.addDays(-7);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);

        date = date.addDays(6);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        dateE->mpEditor->setDataValue(setdv);

        button->setText(mapMsg.value("menu_last_week"));
        return;
    }

    //本月
    if ( periodName.contains(mapMsg.value("menu_this_month")) ) {
        while ( date.day() != 1 ) { date = date.addDays(-1); }
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);

        date = date.addDays(32);
        while ( date.day() != 1 ) { date = date.addDays(-1); }
        date = date.addDays(-1);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        dateE->mpEditor->setDataValue(setdv);
        button->setText(mapMsg.value("menu_this_month"));
        return;
    }

    //上月
    if ( periodName.contains(mapMsg.value("menu_last_month")) ) {
        while ( date.day() != 1 ) { date = date.addDays(-1); }
        date = date.addDays(-26);
        while ( date.day() != 1 ) { date = date.addDays(-1); }
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);

        date = date.addDays(32);
        while ( date.day() != 1 ) { date = date.addDays(-1); }
        date = date.addDays(-1);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        dateE->mpEditor->setDataValue(setdv);

        button->setText(mapMsg.value("menu_last_month"));
        return;
    }

    //今年
    if ( periodName.contains(mapMsg.value("menu_this_year")) ) {
        date = QDate(date.year(), 1, 1);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);

        date = QDate(date.year(), 12, 31);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        dateE->mpEditor->setDataValue(setdv);
        button->setText(mapMsg.value("menu_this_year"));
        return;
    }

    //去年
    if ( periodName.contains(mapMsg.value("menu_last_year")) ) {
        date = QDate(date.year() - 1, 1, 1);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        if ( dateB->mpEditor->isEnabled() ) dateB->mpEditor->setDataValue((setdv > mindv) ? setdv : mindv);

        date = QDate(date.year(), 12, 31);
        setdv = QDateTime(date).toMSecsSinceEpoch() / 1000;
        dateE->mpEditor->setDataValue(setdv);

        button->setText(mapMsg.value("menu_last_year"));
        return;
    }
}

void BsWin::exportGrid(const BsGrid *grid, const QStringList headerPairs)
{
    //用户选择文件位置及命名
    QString deskPath = QStandardPaths::locate(QStandardPaths::DesktopLocation, QString(), QStandardPaths::LocateDirectory);
    QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                    mapMsg.value("tool_export"),
                                                    deskPath,
                                                    mapMsg.value("i_formatted_csv_file")
#ifdef Q_OS_MAC
                                                    ,0
                                                    ,QFileDialog::DontUseNativeDialog
#endif
                                                    );
    if (fileName.length() < 1)
        return;

    //准备数据
    QStringList rows;

    //表头数据
    rows << headerPairs;

    //列名
    QStringList cols;
    for ( int j = 0, jLen = grid->columnCount(); j < jLen; ++j ) {
        if ( ! grid->isColumnHidden(j) )
            cols << grid->model()->headerData(j, Qt::Horizontal).toString();
    }
    rows << cols.join(QChar(44));

    //数据
    for ( int i = 0, iLen = grid->rowCount(); i < iLen; ++i ) {
        if ( ! grid->isRowHidden(i) ) {
            QStringList cols;
            for ( int j = 0, jLen = grid->columnCount(); j < jLen; ++j ) {
                if ( ! grid->isColumnHidden(j) ) {
                    if ( grid->item(i, j) )
                        cols << grid->item(i, j)->text();
                    else
                        cols << QString();
                }
            }
            rows << cols.join(QChar(44));
        }
    }

    //合计
    if ( grid->mpFooter && grid->columnCount() == grid->mpFooter->columnCount() ) {
        QStringList cols;
        for ( int j = 0, jLen = grid->columnCount(); j < jLen; ++j ) {
            if ( ! grid->isColumnHidden(j) ) {
                bool fldCargoo = (grid->mCols.at(j)->mFldName == QStringLiteral("cargo"));
                QString footerText = (grid->mpFooter->item(0, j)) ? grid->mpFooter->item(0, j)->text() : QString();
                QString txt = ( j == 0 && (fldCargoo || footerText.indexOf(QChar('>')) > 0) )
                        ? mapMsg.value("word_total")
                        : footerText;
                if ( txt.indexOf(QChar('>')) > 0 )
                    txt = QString();
                cols << txt;
            }
        }
        rows << cols.join(QChar(44));
    }

    //数据
    QString fileData = rows.join(QChar(10));    //除非用户定制，否则不要考虑加解密什么的。

    //保存
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream strm(&f);
#ifdef Q_OS_WIN
        //EXCEL打不开无BOM头的UTF-8文件（记事本带智能检测，故可以）。
        strm.setGenerateByteOrderMark(true);
#endif
        strm << fileData;
        f.close();
    }
}

QString BsWin::pairTextToHtml(const QStringList &pairs, const bool lastRed)
{
    QString html;
    for ( int i = 0, iLen = pairs.length(); i < iLen; ++i ) {
        QStringList pair = QString(pairs.at(i)).split(QChar(9));
        QString color = ( i == iLen - 1 && lastRed ) ? "red" : "#260";
        html += QStringLiteral("<b>%1：</b><font color='%2'>%3</font>&nbsp;&nbsp;&nbsp;&nbsp;")
                .arg(pair.at(0)).arg(color).arg(pair.at(1));
    }
    return html;
}

bool BsWin::getOptValueByOptName(const QString &optName)
{
    for ( int i = 0, iLen = mpMenuOptionBox->actions().length(); i < iLen; ++i ) {
        QAction *act = mpMenuOptionBox->actions().at(i);
        if ( act->property("optname").toString() == optName )
            return act->isChecked();
    }
    return false;
}

void BsWin::displayGuideTip(const QString &tip)
{
    if ( tip.isEmpty() ) {
        mpGuide->setText( (mGuideTipSwitch) ? mGuideClassTip : mGuideObjectTip);
    } else {
        mpGuide->setText(tip);
        mGuideTipSwitch = !mGuideTipSwitch;
    }
}

void BsWin::forceShowMessage(const QString &msg)
{
    mpLblMessage->setText(msg);
    mpPnlMessage->show();
    QResizeEvent *e = new QResizeEvent(size(), size());
    resizeEvent(e);
    delete e;
}

void BsWin::hideFoceMessage()
{
    mpPnlMessage->hide();
    QResizeEvent *e = new QResizeEvent(size(), size());
    resizeEvent(e);
    delete e;
}

void BsWin::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    loadAllOptionSettings();

    //此处应用option
    mpGuide->setVisible(!mpAcOptGuideNotShowAnymore->isChecked());
}

void BsWin::closeEvent(QCloseEvent *e)
{
    saveAllOptionSettings();
    QWidget::closeEvent(e);
}

void BsWin::doToolExport() {
    exportGrid(mpGrid);
}

void BsWin::clickHelp()
{
    QString winName;
    switch ( mBsWinType ) {
    case bswtMisc:
        winName = QStringLiteral("misc");
        break;
    case bswtReg:
        winName = QStringLiteral("regis");
        break;
    case bswtSheet:
        winName = QStringLiteral("sheet");
        break;
    default:
        winName = QStringLiteral("query");
    }
    QString url = QStringLiteral("https://www.bailisoft.com/passage/jyb_win_%1.html?win=%2").arg(winName).arg(mRightWinName);
    QDesktopServices::openUrl(QUrl(url));
}

void BsWin::clickOptGuideNotShowAnymore()
{
    if ( mpAcOptGuideNotShowAnymore->isChecked() ) {
        mpGuide->hide();
    }
}

void BsWin::loadAllOptionSettings()
{
    QSettings settings;
    settings.beginGroup(BSR17OptionBox);
    settings.beginGroup(mMainTable);
    for ( int i = 0, iLen = mpMenuOptionBox->actions().length(); i < iLen; ++i ) {
        QAction *act = mpMenuOptionBox->actions().at(i);
        act->setCheckable(true);
        act->setChecked( settings.value(act->text(), false).toBool() );
    }
    settings.endGroup();
    settings.endGroup();
}

void BsWin::saveAllOptionSettings()
{
    QSettings settings;
    settings.beginGroup(BSR17OptionBox);
    settings.beginGroup(mMainTable);
    for ( int i = 0, iLen = mpMenuOptionBox->actions().length(); i < iLen; ++i ) {
        QAction *act = mpMenuOptionBox->actions().at(i);
        settings.setValue(act->text(), act->isChecked());
    }
    settings.endGroup();
    settings.endGroup();
}


// BsQryWin
BsQryWin::BsQryWin(QWidget *parent, const QString &name, const QStringList &fields, const uint qryFlags)
    : BsWin(parent, name, fields, bswtQuery), mQryFlags(qryFlags)
{
    //用于权限判断
    mRightWinName = name;
    mRightWinName.replace(QChar('_'), QString());
    //Q_ASSERT(lstQueryWinTableNames.indexOf(mRightWinName) >= 0);
    //由于viszd也是用的BsQryWin但没有权限细设，所以不能用此断言。
    //此断言主要怕vi类命名太多没搞一致，这儿用不用没关系。

    //具体化mFields（包括列名、帮助提示、特性等）
    //以下代码，与单据表以及视图命名强关联，特别注意。
    //sheetname变量只是用于使用mapMsg查找字段中文名称，因此可以替换，但按mapMsg键约定必须用某单据表名
    QStringList nameSecs = name.split(QChar('_'));
    Q_ASSERT(nameSecs.length() >= 2);
    QString sheetName = nameSecs.at(1);
    if ( sheetName == QStringLiteral("cg") )
        sheetName = QStringLiteral("cgj");
    if ( sheetName == QStringLiteral("pf") || sheetName == QStringLiteral("xs") )
        sheetName = QStringLiteral("pff");

    BsField *fld;
    fld = getFieldByName("shop");
    fld->mFldCnName = mapMsg.value(QStringLiteral("fldcname_%1_shop").arg(sheetName));
    if ( (qryFlags & bsqtSumStock) == bsqtSumStock || (qryFlags & bsqtViewAll) == bsqtViewAll )
        fld->mFldCnName = mapMsg.value("qry_stock_shop_cname");

    fld = getFieldByName("trader");
    fld->mFldCnName = mapMsg.value(QStringLiteral("fldcname_%1_trader").arg(sheetName));
    if ( nameSecs.contains("xs") )
        fld->mFldCnName = mapMsg.value("qry_pff_lsd_trader_cname");

    fld = getFieldByName("actpay");
    if ( fld )
        fld->mFldCnName = mapMsg.value(QStringLiteral("fldcname_%1_actpay").arg(sheetName));


    //帮助提示条
    mGuideObjectTip = mapMsg.value(QStringLiteral("win_%1").arg(name)).split(QChar(9)).at(1);
    mGuideClassTip = mapMsg.value("win_<query>").split(QChar(9)).at(1);
    mpGuide->setText(mGuideClassTip);
    mpStatusBar->hide();

    //工具条
    QLabel *lblWinTitle = new QLabel(this);
    lblWinTitle->setText(mapMsg.value(QStringLiteral("win_%1").arg(name)).split(QChar(9)).at(0));
    lblWinTitle->setStyleSheet("font-size:30pt; padding-left:10px; padding-right:10px; ");
    lblWinTitle->setAlignment(Qt::AlignCenter);
    mpToolBar->insertWidget(mpAcToolSeprator, lblWinTitle);

    QToolButton *btnQryBack = new QToolButton(this);
    btnQryBack->setText(mapMsg.value("btn_back_requery").split(QChar(9)).at(0));
    btnQryBack->setIcon(QIcon(":/icon/backqry.png"));
    btnQryBack->setIconSize(QSize(32, 32));
    btnQryBack->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnQryBack->setStyleSheet(QStringLiteral("QToolButton {border: 1px solid #999; border-radius: 6px; %1} "
                                             "QToolButton:hover{background-color: #ccc;} ")
                              .arg(mapMsg.value("css_vertical_gradient")));
    btnQryBack->setMinimumWidth(90);
    connect(btnQryBack, SIGNAL(clicked(bool)), this, SLOT(clickQryBack()));
    mpAcMainBackQry = mpToolBar->insertWidget(mpAcToolSeprator, btnQryBack);

    mpAcMainHistory = mpToolBar->addAction(QIcon(":/icon/history.png"), mapMsg.value("btn_history").split(QChar(9)).at(0),
                                     this, SLOT(clickHistory()));
    mpAcMainPrint = mpToolBar->addAction(QIcon(":/icon/print.png"), mapMsg.value("btn_print").split(QChar(9)).at(0),
                                     this, SLOT(clickPrint()));

    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainHistory);
    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainPrint);

    QString hisHint = ( (qryFlags & bsqtSumCash) == bsqtSumCash )
            ? (name.contains("cg") ? mapMsg.value("hint_history_cg") : mapMsg.value("hint_history_pf"))
            : mapMsg.value("btn_history").split(QChar(9)).at(1);
    mpAcMainHistory->setToolTip(hisHint);
    mpAcMainBackQry->setProperty(BSACRIGHT, true);
    mpAcMainHistory->setProperty(BSACRIGHT, true);
    mpAcMainPrint->setProperty(BSACRIGHT, canDo(mRightWinName, bsrqPrint));
    mpAcToolExport->setProperty(BSACRIGHT, canDo(mRightWinName, bsrqExport));

    mpAcMainHistory->setProperty(BSACFLAGS, bsacfQryReturned);
    mpAcMainPrint->setProperty(BSACFLAGS, bsacfQryReturned);
    mpAcToolExport->setProperty(BSACFLAGS, bsacfQryReturned);

    if ( !name.endsWith("cash") ) {

        mpToolAddCalcSetMoney = mpMenuToolCase->addAction(mapMsg.value("tool_setmoney_calc"),
                                                          this, SLOT(doToolAddCalcSetMoney()));
        mpToolAddCalcRetMoney = mpMenuToolCase->addAction(mapMsg.value("tool_retmoney_calc"),
                                                          this, SLOT(doToolAddCalcRetMoney()));
        mpToolAddCalcLotMoney = mpMenuToolCase->addAction(mapMsg.value("tool_lotmoney_calc"),
                                                          this, SLOT(doToolAddCalcLotMoney()));
        mpToolAddCalcBuyMoney = mpMenuToolCase->addAction(mapMsg.value("tool_buymoney_calc"),
                                                          this, SLOT(doToolAddCalcBuyMoney()));
        mpToolAddCalcSetMoney->setProperty(BSACRIGHT, true);
        mpToolAddCalcRetMoney->setProperty(BSACRIGHT, canRett);
        mpToolAddCalcLotMoney->setProperty(BSACRIGHT, canLott);
        mpToolAddCalcBuyMoney->setProperty(BSACRIGHT, canBuyy);

        mpToolAddCalcSetMoney->setProperty(BSACFLAGS, bsacfQryReturned);
        mpToolAddCalcRetMoney->setProperty(BSACFLAGS, bsacfQryReturned);
        mpToolAddCalcLotMoney->setProperty(BSACFLAGS, bsacfQryReturned);
        mpToolAddCalcBuyMoney->setProperty(BSACFLAGS, bsacfQryReturned);
    }


    //开关盒


    //条件范围
    QMenu *mnPeriod = new QMenu(this);
    mnPeriod->addAction(mapMsg.value("menu_today"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_yesterday"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addSeparator();
    mnPeriod->addAction(mapMsg.value("menu_this_week"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_last_week"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addSeparator();
    mnPeriod->addAction(mapMsg.value("menu_this_month"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_last_month"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addSeparator();
    mnPeriod->addAction(mapMsg.value("menu_this_year"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_last_year"), this, SLOT(clickQuickPeriod()));
    mnPeriod->setStyleSheet("QMenu::item {padding-left:16px;}");
    mpBtnPeriod  = new QToolButton(this);
    mpBtnPeriod->setIcon(QIcon(":/icon/calendar.png"));
    mpBtnPeriod->setText(mapMsg.value("i_period_quick_select"));
    mpBtnPeriod->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mpBtnPeriod->setIconSize(QSize(12, 12));
    mpBtnPeriod->setMenu(mnPeriod);
    mpBtnPeriod->setPopupMode(QToolButton::InstantPopup);

    bool useTristate = (qryFlags & bsqtSumStock) != bsqtSumStock &&
            (qryFlags & bsqtSumRest) != bsqtSumRest &&
            (qryFlags & bsqtSumCash) != bsqtSumCash &&
            (qryFlags & bsqtViewAll) != bsqtViewAll;
    mpConCheck = new BsConCheck(this, useTristate);

    mpConDateB  = new BsFldBox(this, getFieldByName("dated"), nullptr, true);
    mpConDateB->mpEditor->setMyPlaceText(mapMsg.value("i_date_begin_text"));
    mpConDateB->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConDateB->mpLabel->setText(mapMsg.value("i_date_begin_label"));
    mpConDateB->mpEditor->setDataValue(QDateTime(QDate::currentDate()).toMSecsSinceEpoch() / 1000);

    mpConDateE  = new BsFldBox(this, getFieldByName("dated"), nullptr, true);
    mpConDateE->mpEditor->setMyPlaceText(mapMsg.value("i_date_end_text"));
    mpConDateE->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConDateE->mpLabel->setText(mapMsg.value("i_date_end_label"));
    mpConDateE->mpEditor->setDataValue(QDateTime(QDate::currentDate()).toMSecsSinceEpoch() / 1000);

    BsField *fldShop = getFieldByName("shop");
    mpConShop   = new BsFldBox(this, fldShop, dsShop, true);
    mpConShop->mpEditor->setMyPlaceText(fldShop->mFldCnName);
    mpConShop->mpEditor->setMyPlaceColor(QColor(Qt::gray));

    BsField *fldStype = getFieldByName("stype");
    mpConStype  = new BsFldBox(this, fldStype, mpDsStype, true);
    mpConStype->mpEditor->setMyPlaceText(fldStype->mFldCnName);
    mpConStype->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConStype->setEnabled(mpDsStype->rowCount() > 0);

    BsField *fldStaff = getFieldByName("staff");
    mpConStaff  = new BsFldBox(this, fldStaff, mpDsStaff, true);
    mpConStaff->mpEditor->setMyPlaceText(fldStaff->mFldCnName);
    mpConStaff->mpEditor->setMyPlaceColor(QColor(Qt::gray));

    BsField *fldTrader = getFieldByName("trader");
    mpConTrader = new BsFldBox(this, fldTrader, mpDsTrader, true);
    mpConTrader->mpEditor->setMyPlaceText(fldTrader->mFldCnName);
    mpConTrader->mpEditor->setMyPlaceColor(QColor(Qt::gray));

    if ( name.contains("szd") ) {
        BsField *fldCargo = getFieldByName("subject");
        mpConCargo   = new BsFldBox(this, fldCargo, dsSubject, true);
        mpConCargo->mpEditor->setMyPlaceText(fldCargo->mFldCnName);
        mpConCargo->mpEditor->setMyPlaceColor(QColor(Qt::gray));

        mpConColorType = nullptr;

        mpConSizerType = nullptr;
    }
    else {
        BsField *fldCargo = getFieldByName("cargo");
        mpConCargo   = new BsFldBox(this, fldCargo, dsCargo, true);
        mpConCargo->mpEditor->setMyPlaceText(fldCargo->mFldCnName);
        mpConCargo->mpEditor->setMyPlaceColor(QColor(Qt::gray));
        mpConCargo->setStatusTip(QStringLiteral("可以限定具体货号，也可以使用下划线与百分号通配符限定。"));

        BsField *fldColorType = getFieldByName("colortype");
        mpConColorType   = new BsFldBox(this, fldColorType, dsColorType, true);
        mpConColorType->mpEditor->setMyPlaceText(fldColorType->mFldCnName);
        mpConColorType->mpEditor->setMyPlaceColor(QColor(Qt::gray));

        BsField *fldSizerType = getFieldByName("sizertype");
        mpConSizerType   = new BsFldBox(this, fldSizerType, dsSizer, true);

        if ( dsSizer->rowCount() == 1 ) {   //升级版改增
            QString sizerType = dsSizer->data(dsSizer->index(0, 0)).toString();
            mpConSizerType->mpEditor->setDataValue(sizerType);
            mpConSizerType->setEnabled(false);
        }
        else {
            mpConSizerType->mpEditor->setMyPlaceText(fldSizerType->mFldCnName);
            mpConSizerType->mpEditor->setMyPlaceColor(QColor(Qt::gray));
        }

        connect(mpConCargo, &BsFldBox::inputEditing, this, &BsQryWin::cargoInputing);
    }

    mpPnlCon = new QGroupBox(mapMsg.value("qry_panel_con"), this);
    QGridLayout *layCon = new QGridLayout(mpPnlCon);
    layCon->setContentsMargins(6, 6, 6, 9);
    layCon->setHorizontalSpacing(20);

    layCon->addWidget(mpBtnPeriod,      0, 0);
    layCon->addWidget(mpConDateB,       0, 1);
    layCon->addWidget(mpConDateE,       0, 2);
    layCon->addWidget(mpConShop,        0, 3);

    layCon->addWidget(mpConCheck,       1, 0, Qt::AlignCenter);

    //条件范围特别设置
    if ( (qryFlags & bsqtSumStock) == bsqtSumStock || (qryFlags & bsqtViewAll) == bsqtViewAll ) {
        layCon->addWidget(mpConColorType,       1, 1);
        layCon->addWidget(mpConSizerType,       1, 2);
        layCon->addWidget(mpConCargo,           1, 3);
        mpConStype->hide();
        mpConStaff->hide();
        mpConTrader->hide();
    }
    else if ( (qryFlags & bsqtSumCash) == bsqtSumCash ) {

        layCon->addWidget(mpConStype,       1, 1);
        layCon->addWidget(mpConStaff,       1, 2);
        layCon->addWidget(mpConTrader,      1, 3);
        mpConColorType->hide();
        mpConSizerType->hide();
        mpConCargo->hide();
    }
    else if ( (qryFlags & bsqtSumRest) == bsqtSumRest ) {

        layCon->addWidget(mpConColorType,       1, 1);
        layCon->addWidget(mpConSizerType,       1, 2);
        layCon->addWidget(mpConCargo,       1, 3);
        layCon->replaceWidget(mpConShop, mpConTrader);
        mpConStype->hide();
        mpConStaff->hide();
        mpConShop->hide();
    }
    else {

        layCon->addWidget(mpConStype,       1, 1);
        layCon->addWidget(mpConStaff,       1, 2);
        layCon->addWidget(mpConTrader,      1, 3);

        if ( mpConColorType && mpConSizerType ) {
            layCon->addWidget(mpConColorType,   2, 1);
            layCon->addWidget(mpConSizerType,   2, 2);
            layCon->addWidget(mpConCargo,       2, 3);
        }
        else {
            layCon->addWidget(mpConCargo,       2, 1, 1, 3);
        }

        if ( name.contains(QStringLiteral("syd")) )
            mpConTrader->hide();
    }

    layCon->setColumnStretch(0, 0);
    layCon->setColumnStretch(1, 2);
    layCon->setColumnStretch(2, 2);
    layCon->setColumnStretch(3, 3);

    //选择项显示与否及统计值中文名特别设置
    if ( (qryFlags & bsqtSumStock) == bsqtSumStock || (qryFlags & bsqtViewAll) == bsqtViewAll ) {

        fld = getFieldByName(QStringLiteral("yeard"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("monthd"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("dated"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("trader"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("actpay"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;

        fld = getFieldByName(QStringLiteral("actowe"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;

        fld = getFieldByName(QStringLiteral("sumqty"));
        if ( fld ) fld->mFldCnName = mapMsg.value("qry_stock_qty");        // QStringLiteral("库存数量");

        fld = getFieldByName(QStringLiteral("summoney"));
        if ( fld ) fld->mFldCnName = mapMsg.value("qry_money_occupied");   // QStringLiteral("资金占用");

        fld = getFieldByName(QStringLiteral("sumdis"));
        if ( fld ) fld->mFldCnName = mapMsg.value("qry_dismoney_gain");    // QStringLiteral("折扣盈利");
    }
    else {

        fld = getFieldByName(QStringLiteral("sumqty"));
        if ( fld ) fld->mFldCnName = mapMsg.value("qry_qty_cname");

        fld = getFieldByName(QStringLiteral("summoney"));
        if ( fld ) fld->mFldCnName = mapMsg.value("qry_money_cname");

        fld = getFieldByName(QStringLiteral("sumdis"));
        if ( fld ) fld->mFldCnName = mapMsg.value("qry_dismoney_cname");

        fld = getFieldByName(QStringLiteral("actpay"));
        if ( fld ) fld->mFldCnName = name.contains(QStringLiteral("cg"))
                ? mapMsg.value("qry_actpayout_cname")
                : mapMsg.value("qry_actpayin_cname");

        fld = getFieldByName(QStringLiteral("actowe"));
        if ( fld ) fld->mFldCnName = mapMsg.value("qry_actowe_cname");
    }

    //补正查询flag
    if ( (qryFlags & bsqtSumOrder) == bsqtSumOrder ) {

        fld = getFieldByName(QStringLiteral("sumdis"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;

        fld = getFieldByName(QStringLiteral("actpay"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;

        fld = getFieldByName(QStringLiteral("actowe"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;
    }

    if ( (qryFlags & bsqtSumRest) == bsqtSumRest ) {  //Rest包括Stock

        mpConDateB->mpEditor->setDataValue(QDateTime(QDate::fromString("1900-01-01", "yyyy-MM-dd")).toMSecsSinceEpoch() / 1000);
        mpConDateB->disableShow();

        fld = getFieldByName(QStringLiteral("yeard"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("monthd"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("dated"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("stype"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("staff"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        if ( (qryFlags & bsqtSumStock) != bsqtSumStock ) {

            fld = getFieldByName(QStringLiteral("shop"));
            if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

            fld = getFieldByName(QStringLiteral("sumdis"));
            if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;
        }
    }

    if ( (qryFlags & bsqtViewAll) == bsqtViewAll ) {

        fld = getFieldByName(QStringLiteral("stype"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("staff"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("shop"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        /*
        fld = getFieldByName(QStringLiteral("hpname"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("setprice"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;
        */

        fld = getFieldByName(QStringLiteral("unit"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;
    }

    if ( (qryFlags & bsqtSumCash) == bsqtSumCash ) {

        mpConDateB->mpEditor->setDataValue(QDateTime(QDate::fromString("1900-01-01", "yyyy-MM-dd")).toMSecsSinceEpoch() / 1000);
        mpConDateB->disableShow();

        fld = getFieldByName(QStringLiteral("yeard"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("monthd"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("dated"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("shop"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("cargo"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("hpname"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("setprice"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("unit"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("color"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("sizers"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("attr1"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("attr2"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("attr3"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("attr4"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("attr5"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("attr6"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;
    }
    else {

        fld = getFieldByName(QStringLiteral("actpay"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;

        fld = getFieldByName(QStringLiteral("actowe"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;
    }

    if ( name.contains("syd") ) {

        fld = getFieldByName(QStringLiteral("trader"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsSel;

        fld = getFieldByName(QStringLiteral("actpay"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;

        fld = getFieldByName(QStringLiteral("actowe"));
        if ( fld ) fld->mFlags = fld->mFlags &~ bsffQryAsVal;
    }


    //统计角度
    mpPnlSel = new QGroupBox(mapMsg.value("qry_panel_sel"), this);
    BsFlowLayout *laySel = new BsFlowLayout(mpPnlSel, 10, 30, 5);
    for ( int i = 0, iLen = fields.length(); i < iLen; ++i ) {
        BsField *fld = getFieldByName(fields.at(i));
        if ( fld ) {
            if ( (fld->mFlags & bsffQryAsSel) || (fld->mFlags & bsffCargoRel) ) {
                if ( (name.contains("stock") || name.contains("all") || name.endsWith("cash") || name.endsWith("rest"))
                     && fld->mFldName == QStringLiteral("sheetid") ) continue;
                if ( name.endsWith("cash") ) {
                     if ( (fld->mFlags & bsffCargoRel)
                          || fld->mFldName == QStringLiteral("color")
                          || fld->mFldName == QStringLiteral("sizers") ) continue;
                }
                if ( fld->mFldName == QStringLiteral("colortype") || fld->mFldName == QStringLiteral("sizertype") ) continue;
                bool showBold = (fld->mFlags & bsffQrySelBold) == bsffQrySelBold;
                BsQryCheckor *chkor = ( fld->mFldName.endsWith("sizers") )
                        ? new BsQrySizerCheckor(mpPnlSel, fld, showBold)
                        : new BsQryCheckor(mpPnlSel, fld, showBold);
                laySel->addWidget(chkor);

                if ( fld->mFldName.endsWith("sizers") ) {
                    mpSizerCheckor = chkor;

                    connect(mpConSizerType, SIGNAL(editingFinished()), this, SLOT(conSizeTypeChanged()));

                    BsQrySizerCheckor *chks = qobject_cast<BsQrySizerCheckor*>(chkor);
                    Q_ASSERT(chks);
                    connect(chks, SIGNAL(typePicked(QString)), this, SLOT(chkSizersPicked(QString)));
                }
            }
        }
    }

    //合计数值
    mpPnlVal = new QGroupBox(mapMsg.value("qry_panel_val"), this);
    BsFlowLayout *layVal = new BsFlowLayout(mpPnlVal, 10, 30, 5);
    for ( int i = 0, iLen = fields.length(); i < iLen; ++i ) {
        BsField *fld = getFieldByName(fields.at(i));
        if ( fld ) {
            if ( name.endsWith("cash") && fld->mFldName == QStringLiteral("sumqty") ) continue;
            if ( (fld->mFlags & bsffQryAsVal) == bsffQryAsVal ) {
                BsQryCheckor *chkor = new BsQryCheckor(mpPnlVal, fld);
                layVal->addWidget(chkor);
                //权限禁止
                if ( (fld->mFldName.contains(QStringLiteral("money")) && !canDo(mRightWinName, bsrqMny)) ||
                     (fld->mFldName.contains(QStringLiteral("dis")) && !canDo(mRightWinName, bsrqDis)) ||
                     (fld->mFldName.contains(QStringLiteral("actpay")) && !canDo(mRightWinName, bsrqPay)) ||
                     (fld->mFldName.contains(QStringLiteral("actowe")) && !canDo(mRightWinName, bsrqOwe))    )
                    chkor->setEnabled(false);
            }
        }
    }

    //选择范围标签（用于查询结果出来后显示）
    mpLblCon = new QLabel(this);
    mpLblCon->setWordWrap(true);

    //面板总排版
    mpPanel = new QWidget(this);
    QVBoxLayout *layPanel = new QVBoxLayout(mpPanel);
    layPanel->setContentsMargins(0, 3, 0, 0);
    layPanel->addWidget(mpPnlCon);
    layPanel->addWidget(mpPnlSel);
    layPanel->addWidget(mpPnlVal);
    layPanel->addWidget(mpLblCon);

    //是否显示零库存或已经完成订单欠货
    mpHaving = new QCheckBox(this);

    mpHaving->setText((name.contains("stock") || (qryFlags & bsqtViewAll) == bsqtViewAll)
                      ? mapMsg.value("qry_show_zero_stock")
                      : mapMsg.value("qry_show_finished_items"));

    mpHaving->setVisible(name.contains("stock") ||
                         name.contains("rest") ||
                         (qryFlags & bsqtViewAll) == bsqtViewAll );

    mpHaving->setStyleSheet("font-style:italic;");

    //表格
    mpQryGrid = new BsQueryGrid(this);
    mpGrid = mpQryGrid;

    //总布局
    QVBoxLayout *layBody = new QVBoxLayout(mpBody);
    layBody->setContentsMargins(3, 0, 3, 3);
    layBody->setSpacing(0);
    layBody->addWidget(mpPanel);
    layBody->addWidget(mpQryGrid);
    mpBody->setObjectName("qrywinbody");
    mpBody->setStyleSheet("QWidget#qrywinbody{background-color:#e9e9e9;}");

    //大按钮面板
    mpPnlQryConfirm = new QWidget(this);
    QVBoxLayout *layConfirm = new QVBoxLayout(mpPnlQryConfirm);
    layConfirm->setContentsMargins(0, 0, 0, 0);
    mpPnlQryConfirm->setStyleSheet(QStringLiteral("QWidget {background: #fff;} "
                                     "QToolButton{border: 1px solid #999; border-radius: 16px; %1} "
                                     "QToolButton:hover{background: #ccc;} ")
                             .arg(mapMsg.value("css_vertical_gradient")));

    mpBtnBigOk = new QToolButton(this);
    mpBtnBigOk->setIcon(QIcon(":/icon/bigok.png"));
    mpBtnBigOk->setIconSize(QSize(128, 128));
    connect(mpBtnBigOk, SIGNAL(clicked(bool)), this, SLOT(clickQryExecute()));

    mpBtnBigCancel = new QToolButton(this);
    mpBtnBigCancel->setIcon(QIcon(":/icon/bigcancel.png"));
    mpBtnBigCancel->setIconSize(QSize(128, 128));
    connect(mpBtnBigCancel, SIGNAL(clicked(bool)), this, SLOT(clickBigCancel()));

    QHBoxLayout *layButtonRow = new QHBoxLayout;
    layButtonRow->addStretch();
    layButtonRow->addWidget(mpBtnBigOk);
    layButtonRow->addWidget(mpBtnBigCancel);
    layButtonRow->addStretch();

    layConfirm->addStretch();
    layConfirm->addLayout(layButtonRow);
    layConfirm->addStretch();

    //具体化StatusTip
    mpAcMainBackQry->setStatusTip(mapMsg.value("btn_back_requery").split(QChar(9)).at(1));

    //外观细节
    mpToolBar->setIconSize(QSize(64, 32));
    mpBtnBigCancel->hide();
    mpLblCon->hide();
    mpAcMainBackQry->setVisible(false);
    mpAcMainHistory->setVisible(false);
    mpAcMainPrint->setVisible(false);

    //一览特别隐藏
    mpPnlVal->setVisible((mQryFlags & bsqtViewAll) != bsqtViewAll);

    //加载选择下拉数据集
    if ( (qryFlags & bsqtSumCash) != bsqtSumCash ) {
        dsCargo->reload();
        dsSubject->reload();
        dsColorType->reload();
        dsSizer->reload();
    }
}

void BsQryWin::showEvent(QShowEvent *e)
{
    BsWin::showEvent(e);

    //显示浮动大按钮
    setFloatorGeometry();

    //初始状态
    updateToolActions();
}

void BsQryWin::resizeEvent(QResizeEvent *e)
{
    BsWin::resizeEvent(e);
    setFloatorGeometry();
}

void BsQryWin::doToolExport()
{
    QStringList headPairs;
    foreach (QString pair, mLabelPairs ) {
        headPairs << pair.replace(QChar(9), QChar(44));
    }
    exportGrid(mpGrid, headPairs);
}

void BsQryWin::clickQuickPeriod()
{
    QAction *act = qobject_cast<QAction*>(QObject::sender());
    Q_ASSERT(act);
    setQuickDate(act->text(), mpConDateB, mpConDateE, mpBtnPeriod);
}

void BsQryWin::clickQryExecute()
{
    //保存列宽
    if ( mpQryGrid->rowCount() > 0 )
        mpQryGrid->saveColWidths();

    //绑店检查
    if ( !loginShop.isEmpty() ) {
        QString conShopText = mpConShop->mpEditor->getDataValue();
        QString conCargoText = mpConCargo->mpEditor->getDataValue();
        if ( conShopText.isEmpty() ) {
            if ( !conCargoText.isEmpty() || conCargoText.contains(QChar('%')) || conCargoText.contains(QChar('_')) ) {
                QMessageBox::information(this, QString(), mapMsg.value("i_bind_shop_need_or_cargo"));
                return;
            }
        }
    }

    //执行查询
    QString errReport = doSqliteQuery();

    //报告
    if ( errReport.isEmpty() ) {
        mpLblCon->setText(pairTextToHtml(mLabelPairs, false));
        mpLblCon->show();
        mpAcMainBackQry->setVisible(true);
        mpAcMainHistory->setVisible(!mMainTable.contains("all") && !mMainTable.contains("szd"));
        mpAcMainPrint->setVisible(true);
        mpPnlCon->hide();
        mpPnlSel->hide();
        mpPnlVal->hide();
        mpPnlQryConfirm->hide();
        mpHaving->hide();

        mpAcMainHistory->setEnabled(canListHistory());

        //更新状态
        updateToolActions();
    }
    else
        QMessageBox::information(this, QString(), errReport);
}

void BsQryWin::clickBigCancel()
{
    mpLblCon->show();
    mpAcMainBackQry->setVisible(true);
    mpAcMainHistory->setVisible(!mMainTable.contains("all") && !mMainTable.contains("szd"));
    mpAcMainPrint->setVisible(true);
    mpPnlCon->hide();
    mpPnlSel->hide();
    mpPnlVal->hide();
    mpPnlQryConfirm->hide();
    mpHaving->hide();

    //更新状态
    updateToolActions();
}

void BsQryWin::clickQryBack()
{
    mpLblCon->hide();
    mpAcMainBackQry->setVisible(false);
    mpAcMainHistory->setVisible(false);
    mpAcMainPrint->setVisible(false);
    mpPnlCon->show();
    mpPnlSel->show();
    mpPnlVal->setVisible((mQryFlags & bsqtViewAll) != bsqtViewAll);
    mpPnlQryConfirm->show();
    mpBtnBigCancel->show();
    mpHaving->setVisible(mMainTable.contains("stock") ||
                         mMainTable.contains("rest") ||
                         (mQryFlags & bsqtViewAll) == bsqtViewAll);
    setFloatorGeometry();

    //更新状态
    updateToolActions();
}

void BsQryWin::clickHistory()
{
    //由于cargo是“核对”按钮显示的必要条件，因此，所有cargo登记表字段，无论con里，还是sel里，都不要————
    //因为，库存对账用到vi_stock_history视图，该视图没有任何cargo表字段，包括colortype与sizertype。
    bool winCash  = mMainTable.contains("cash");
    bool winRest  = mMainTable.contains("rest");
    bool winStock = mMainTable.contains("stock");

    bool shopSelected   = checkFieldIfSelected("shop");
    bool traderSelected = checkFieldIfSelected("trader");

    bool shopLimited    = !mpConShop->mpEditor->getDataValue().isEmpty();
    bool traderLimited  = !mpConTrader->mpEditor->getDataValue().isEmpty();
    bool cargoLimited   = !mpConCargo->mpEditor->getDataValue().isEmpty();

    //必须选择具体一行
    if ( mpQryGrid->currentRow() < 0 ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_please_pick_a_row"));
        return;
    }

    //选择期初日期（假如需要的话）
    qint64 qcDate = 0;
    if ( (winStock && (shopSelected   || shopLimited)  ) ||
         (winRest  && (traderSelected || traderLimited)) ||  winCash ) {
        BsPickDateDlg dlg(this);
        if ( dlg.exec() == QDialog::Accepted ) {
            if ( dlg.mpTypeLeft->isChecked() ) {
                qcDate = dlg.mpEdtDate->getDataValue().toLongLong();
            }
        }
        else
            return;
    }

    //当前货品或欠款单位
    QString keyFld;
    QString keyVal;

    if ( winCash ) {
        keyFld = "trader";
        int idx = mpQryGrid->getColumnIndexByFieldName(keyFld);
        keyVal = (idx >= 0) ? mpQryGrid->item(mpQryGrid->currentRow(), idx)->text() : mpConTrader->mpEditor->getDataValue();
    }
    else {
        keyFld = ( mMainTable.contains("szd") ) ? "subject" : "cargo";
        int idx = mpQryGrid->getColumnIndexByFieldName(keyFld);
        keyVal = (idx >= 0) ? mpQryGrid->item(mpQryGrid->currentRow(), idx)->text() : mpConCargo->mpEditor->getDataValue();
    }

    //对账关联门店或欠款单位
    QString relKeyFld;
    QString relKeyVal;

    if ( qcDate > 0 ) {
        if ( winRest ) {
            relKeyFld = "trader";
            int idx = mpQryGrid->getColumnIndexByFieldName(relKeyFld);
            relKeyVal = (idx >= 0) ? mpQryGrid->item(mpQryGrid->currentRow(), idx)->text() : mpConTrader->mpEditor->getDataValue();
        }
        else if ( winStock ) {
            relKeyFld = "shop";
            int idx = mpQryGrid->getColumnIndexByFieldName(relKeyFld);
            relKeyVal = (idx >= 0) ? mpQryGrid->item(mpQryGrid->currentRow(), idx)->text() : mpConShop->mpEditor->getDataValue();
        }
    }

    //SQL组合
    QStringList cons, sels, ords, grps;
    QString fromSource = ( winStock && qcDate > 0 ) ? QStringLiteral("vi_stock_history") : mFromSource;
    QString qcHtmlPair;

    //查询期初值
    if ( qcDate > 0 ) {

        //面板条件
        cons = getConExpPairsFromMapRangeCon(false, false);

        //第三位条件，用prepend()
        cons.prepend(QStringLiteral("dated < %1").arg(qcDate));

        //第二位条件，用prepend()
        if ( winRest || winStock )
            cons.prepend(QStringLiteral("%1='%2'").arg(relKeyFld).arg(relKeyVal));

        //最前条件，用prepend()
        cons.prepend(QStringLiteral("%1='%2'").arg(keyFld).arg(keyVal));

        //最后补充条件，用<<
        for ( int i = 0, iLen = mpQryGrid->mCols.length(); i < iLen; ++i ) {
            BsField *bsCol = mpQryGrid->mCols.at(i);
            uint colf = bsCol->mFlags;
            if ( (colf & bsffAggSum) != bsffAggSum && (colf & bsffCargoRel) != bsffCargoRel &&
                 (colf & bsffBool) != bsffBool && bsCol->mFldName != QStringLiteral("sizers") ) {
                QString sqlValue = mpQryGrid->getSqlValueFromDisplay(mpQryGrid->currentRow(), i);
                cons << QStringLiteral("%1=%2").arg(bsCol->mFldName).arg(sqlValue);
            }
        }

        //sels, ords, grps;
        if ( winCash )
            sels << "SUM(sumqty) AS sumqty" << "SUM(summoney) AS summoney"
                 << "SUM(actpay) AS actpay" << "SUM(actowe) AS actowe";
        else
            sels << "SUM(qty) AS qty";

        //收付中文名
        QString payCName = (mMainTable.contains("cg")) ? mapMsg.value("word_sum_payo") : mapMsg.value("word_sum_payi");

        //sql
        QString sql = QStringLiteral("SELECT %1 FROM %2 WHERE %3;")
                .arg(sels.join(QChar(44))).arg(fromSource).arg(cons.join(" AND "));
        QSqlQuery qry;
        qry.exec(sql);
        qry.next();
        QStringList qcvs;
        if ( winCash )
            qcvs << QStringLiteral("%1%2").arg(mapMsg.value("word_sum_qty")).arg(bsNumForRead(qry.value(0).toLongLong(), 0))
                 << QStringLiteral("%1%2").arg(mapMsg.value("word_sum_mny")).arg(bsNumForRead(qry.value(1).toLongLong(), mMoneyDots))
                 << QStringLiteral("%1%2").arg(payCName).arg(bsNumForRead(qry.value(2).toLongLong(), mMoneyDots))
                 << QStringLiteral("%1%2").arg(mapMsg.value("word_sum_owe")).arg(bsNumForRead(qry.value(3).toLongLong(), mMoneyDots));
        else
            qcvs << bsNumForRead(qry.value(0).toLongLong(), 0);

        //取得
        qcHtmlPair = QStringLiteral("%1\t%2").arg("期初值").arg(qcvs.join(','));
    }

    //下面开始表格清单查询
    QStringList labelPairs = mLabelPairs;

    //面板条件
    cons = getConExpPairsFromMapRangeCon(true, false);

    //第三位条件，用prepend()
    if ( qcDate > 0 ) {
        cons.prepend(QStringLiteral("dated >= %1").arg(qcDate));
        QDate dtv = QDateTime::fromMSecsSinceEpoch(qcDate * 1000).date();
        QString dts = dtv.toString("yyyy-MM-dd");
        labelPairs.prepend(QStringLiteral("%1\t%2").arg(mpConDateB->getFieldCnName()).arg(dts));
    }

    //第二位条件，用prepend()
    if ( qcDate > 0 && (winRest || winStock) ) {
        cons.prepend(QStringLiteral("%1='%2'").arg(relKeyFld).arg(relKeyVal));
        labelPairs.prepend(QStringLiteral("%1\t%2").arg(mapMsg.value("word_list_belong")).arg(relKeyVal));
    }

    //最前条件，用prepend()
    cons.prepend(QStringLiteral("%1='%2'").arg(keyFld).arg(keyVal));
    labelPairs.prepend(QStringLiteral("%1\t%2").arg(mapMsg.value("word_list_object")).arg(keyVal));

    //最后补充条件，用<<
    for ( int i = 0, iLen = mpQryGrid->mCols.length(); i < iLen; ++i ) {
        BsField *bsCol = mpQryGrid->mCols.at(i);
        uint colf = bsCol->mFlags;
        if ( (colf & bsffAggSum) != bsffAggSum && (colf & bsffCargoRel) != bsffCargoRel &&
             (colf & bsffBool) != bsffBool && bsCol->mFldName != QStringLiteral("sizers") ) {
            QString showValue = mpQryGrid->item(mpQryGrid->currentRow(), i)->text();
            QString sqlValue = mpQryGrid->getSqlValueFromDisplay(mpQryGrid->currentRow(), i);
            cons << QStringLiteral("%1=%2").arg(bsCol->mFldName).arg(sqlValue);
            labelPairs.prepend(QStringLiteral("%1\t%2").arg(bsCol->mFldCnName).arg(showValue));
        }
    }

    //最后补上期初文字（如果有）
    if ( !qcHtmlPair.isEmpty() )
        labelPairs << qcHtmlPair;

    //sels, ords, grps
    sels.clear();
    ords.clear();
    grps.clear();
    QString sizerType;
    if ( qcDate > 0 ) {

        sels << "dated" << "sheetname" << "sheetid" << "proof" << "stype" << "staff";

        if ( winStock )
            sels << "trader";
        else
            sels << "shop";

        if ( winCash )
            sels << "sumqty" << "summoney" << "actpay" << "actowe";
        else
            sels << "color" << "qty";

        ords << "dated" << "sheetname" << "sheetid";
    }
    else {
        sels << "color" << "SUM(qty) AS qty" << "GROUP_CONCAT(sizers, '') AS sizers";
        ords << "color";
        grps << "color";

        QString cargo;
        if ( cargoLimited )
            cargo = mpConCargo->mpEditor->getDataValue();
        else
            cargo = keyVal;

        sizerType = dsCargo->getValue(cargo, "sizertype");
    }

    //sql
    QString grpSql = (grps.isEmpty()) ? QString() : QStringLiteral("GROUP BY %1").arg(grps.join(QChar(44)));
    QString sql = QStringLiteral("SELECT %1 FROM %2 WHERE %3 %4 ORDER BY %5;")
            .arg(sels.join(QChar(44))).arg(fromSource).arg(cons.join(" AND "))
            .arg(grpSql).arg(ords.join(QChar(44)));

    //texts
    QString payCName = (mMainTable.contains("cg")) ? mapMsg.value("word_payo") : mapMsg.value("word_payi");
    QStringList colTitles;
    colTitles << QStringLiteral("actpay\t%1").arg(payCName);
    colTitles << QStringLiteral("shop\t%1").arg(mapMsg.value("word_list_relative"));
    colTitles << QStringLiteral("trader\t%1").arg(mapMsg.value("word_list_relative"));

    //窗口
    BsHistoryWin *win = new BsHistoryWin(mppMain, this, labelPairs, sql, colTitles, sizerType, qcDate > 0);

    QList<QDockWidget *> docks = mppMain->findChildren<QDockWidget *>();
    for ( int i = docks.length() - 1; i >= 0; --i ) {
        QDockWidget *dock = docks.at(i);
        delete dock->widget();
        delete dock;
    }
    QDockWidget *dock = new QDockWidget(mapMsg.value("btn_history").split(QChar(9)).at(0), this);
    dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    dock->setWidget(win);
    mppMain->addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void BsQryWin::clickPrint()
{
    QStringList conPairs;
    if ( mpConDateB->mpEditor->isEnabled() )
        conPairs << (mpConDateB->getFieldCnName() + QChar(9) + mpConDateB->mpEditor->text());

    conPairs << (mpConDateE->getFieldCnName() + QChar(9) + mpConDateE->mpEditor->text());

    if ( !mpConStype->mpEditor->getDataValue().isEmpty() )
        conPairs << (mpConStype->getFieldCnName() + QChar(9) + mpConStype->mpEditor->getDataValue());

    if ( !mpConStaff->mpEditor->getDataValue().isEmpty() )
        conPairs << (mpConStaff->getFieldCnName() + QChar(9) + mpConStaff->mpEditor->getDataValue());

    if ( !mpConShop->mpEditor->getDataValue().isEmpty() )
        conPairs << (mpConShop->getFieldCnName() + QChar(9) + mpConShop->mpEditor->getDataValue());

    if ( !mpConTrader->mpEditor->getDataValue().isEmpty() )
        conPairs << (mpConTrader->getFieldCnName() + QChar(9) + mpConTrader->mpEditor->getDataValue());

    if ( !mpConCargo->mpEditor->getDataValue().isEmpty() )
        conPairs << (mpConCargo->getFieldCnName() + QChar(9) + mpConCargo->mpEditor->getDataValue());

    if ( mpConColorType && mpConSizerType ) {
        if ( !mpConColorType->mpEditor->getDataValue().isEmpty() )
            conPairs << (mpConColorType->getFieldCnName() + QChar(9) + mpConColorType->mpEditor->getDataValue());

        if ( !mpConSizerType->mpEditor->getDataValue().isEmpty() )
            conPairs << (mpConSizerType->getFieldCnName() + QChar(9) + mpConSizerType->mpEditor->getDataValue());
    }

    mpQryGrid->doPrint(windowTitle(), conPairs, loginer, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
}

void BsQryWin::cargoInputing(const QString &text, const bool)
{
    if ( dsCargo->keyExists(text) ) {
        QString colorType = dsCargo->getValue(text, "colortype");
        QString limColorType = dsColorType->keyExists(colorType) ? colorType : QString();
        mpConColorType->mpEditor->setDataValue(limColorType);
        mpConColorType->setEnabled(false);

        QString sizerType = dsCargo->getValue(text, "sizertype");
        mpConSizerType->mpEditor->setDataValue(sizerType);
        mpConSizerType->setEnabled(false);
    }
    else {
        mpConColorType->mpEditor->setDataValue(QString());
        mpConColorType->setEnabled(true);

        mpConSizerType->mpEditor->setDataValue(QString());
        mpConSizerType->setEnabled(true);
    }
}

void BsQryWin::conSizeTypeChanged()
{
    if ( mpConSizerType->mpEditor->getDataValue().isEmpty() ) {
        mpSizerCheckor->setChecked(false);
    }
}

void BsQryWin::chkSizersPicked(const QString &stype)
{
    mpConSizerType->mpEditor->setDataValue(stype);
}

void BsQryWin::doToolAddCalcSetMoney()
{
    QString err = mpQryGrid->addCalcMoneyColByPrice(QStringLiteral("setprice"));
    if ( ! err.isEmpty() ) QMessageBox::information(this, QString(), err);
}

void BsQryWin::doToolAddCalcRetMoney()
{
    QString err = mpQryGrid->addCalcMoneyColByPrice(QStringLiteral("retprice"));
    if ( ! err.isEmpty() ) QMessageBox::information(this, QString(), err);
}

void BsQryWin::doToolAddCalcLotMoney()
{
    QString err = mpQryGrid->addCalcMoneyColByPrice(QStringLiteral("lotprice"));
    if ( ! err.isEmpty() ) QMessageBox::information(this, QString(), err);
}

void BsQryWin::doToolAddCalcBuyMoney()
{
    QString err = mpQryGrid->addCalcMoneyColByPrice(QStringLiteral("buyprice"));
    if ( ! err.isEmpty() ) QMessageBox::information(this, QString(), err);
}

void BsQryWin::updateToolActions()
{
    //状态
    uint stateFlags;

    if ( mpPnlCon->isVisible() )
        stateFlags = bsacfQrySelecting;
    else
        stateFlags = bsacfQryReturned;

    if ( mpQryGrid->getColumnIndexByFieldName("shop") >= 0 )
        stateFlags |= bsacfQryByShop;

    if ( mpQryGrid->getColumnIndexByFieldName("trader") >= 0 )
        stateFlags |= bsacfQryByTrader;

    if ( mpQryGrid->getColumnIndexByFieldName("cargo") >= 0 )
        stateFlags |= bsacfQryByCargo;

    //主按钮
    for ( int i = 0, iLen = mpToolBar->actions().length(); i < iLen; ++i ) {
        QAction *act = mpToolBar->actions().at(i);
        uint acFlags = act->property(BSACFLAGS).toUInt();
        bool rightAllow = act->property(BSACRIGHT).toBool();
        act->setEnabled(rightAllow && (stateFlags & acFlags) == acFlags);
    }

    //工具箱
    for ( int i = 0, iLen = mpMenuToolCase->actions().length(); i < iLen; ++i ) {
        QAction *act = mpMenuToolCase->actions().at(i);
        uint acFlags = act->property(BSACFLAGS).toUInt();
        bool rightAllow = act->property(BSACRIGHT).toBool();
        act->setEnabled(rightAllow && (stateFlags & acFlags) == acFlags);
    }

    //开关盒
    for ( int i = 0, iLen = mpMenuOptionBox->actions().length(); i < iLen; ++i ) {
        QAction *act = mpMenuOptionBox->actions().at(i);
        uint acFlags = act->property(BSACFLAGS).toUInt();
        bool rightAllow = act->property(BSACRIGHT).toBool();
        act->setEnabled(rightAllow && (stateFlags & acFlags) == acFlags);
    }
}

bool BsQryWin::checkFieldIfSelected(const QString &fldName)
{
    for ( int i = 0, iLen = mpPnlSel->children().length(); i < iLen; ++i ) {
        BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlSel->children().at(i));
        if ( chkor ) {
            if ( chkor->isChecked() ) {
                if (chkor->mField->mFldName == fldName ) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool BsQryWin::canListHistory()
{
    bool traderSelected = checkFieldIfSelected("trader");
    bool cargoSelected = checkFieldIfSelected("cargo");

    if ( mMainTable.contains("stock") ) {
        QString strCargo = mpConCargo->mpEditor->getDataValue();
        return ( (cargoSelected || (!strCargo.isEmpty() && strCargo.indexOf(QChar('%')) < 0 && strCargo.indexOf(QChar('_')) < 0))
               ) && mpQryGrid->rowCount() > 0;
    }
    else if ( mMainTable.contains("rest") ) {
        return ( (cargoSelected || !mpConCargo->mpEditor->getDataValue().isEmpty()) ) && mpQryGrid->rowCount() > 0;
    }
    else if ( mMainTable.contains("cash") ) {
        return ( (traderSelected || !mpConTrader->mpEditor->getDataValue().isEmpty()) ) && mpQryGrid->rowCount() > 0;
    }
    else if ( ! mMainTable.contains("all") ) {
        return cargoSelected  && mpQryGrid->rowCount() > 0;
    }

    return false;
}

void BsQryWin::setFloatorGeometry()
{
    QPoint pointGridLeftTop = mpQryGrid->mapTo(this, QPoint(0, 0));
    mpPnlQryConfirm->setGeometry(pointGridLeftTop.x(), pointGridLeftTop.y(), mpQryGrid->width(), mpQryGrid->height());

    QSize havSize = mpHaving->sizeHint();
    if ((mQryFlags & bsqtViewAll) == bsqtViewAll) {

        QPoint refPoint = mpQryGrid->mapTo(this, QPoint(mpQryGrid->width(), 0));

        mpHaving->setGeometry(refPoint.x() - havSize.width(),
                              refPoint.y() - havSize.height(),
                              havSize.width(),
                              havSize.height());
    }
    else {
        QPoint refPoint = mpPnlVal->mapTo(this, QPoint(mpPnlVal->width(), 0));

        mpHaving->setGeometry(refPoint.x() - havSize.width() - 10,
                              refPoint.y() + (mpPnlVal->height() - havSize.height()) / 2 + 3,
                              havSize.width(),
                              havSize.height());
    }
}

QStringList BsQryWin::getConExpPairsFromMapRangeCon(const bool includeDate, const bool includeCargoRel)
{
    QStringList conExps;

    if ( includeDate ) {
        if ( mapRangeCon.contains("dateb") )
            conExps << QStringLiteral("(dated BETWEEN %1 AND %2)").arg(mapRangeCon.value("dateb")).arg(mapRangeCon.value("datee"));
        else
            conExps << QStringLiteral("dated<=%1").arg(mapRangeCon.value("datee"));
    }

    if ( mapRangeCon.contains("cargo") ) {
        QString strCargo = mapRangeCon.value("cargo");
        if ( strCargo.indexOf(QChar('%')) >= 0 || strCargo.indexOf(QChar('_')) >= 0 )
            conExps << QStringLiteral("cargo LIKE '%1'").arg(mapRangeCon.value("cargo"));
        else
            conExps << QStringLiteral("cargo='%1'").arg(mapRangeCon.value("cargo"));
    }

    if ( mapRangeCon.contains("shop") )
        conExps << QStringLiteral("shop='%1'").arg(mapRangeCon.value("shop"));

    if ( mapRangeCon.contains("trader") )
        conExps << QStringLiteral("trader='%1'").arg(mapRangeCon.value("trader"));

    if ( mapRangeCon.contains("stype") )
        conExps << QStringLiteral("stype='%1'").arg(mapRangeCon.value("stype"));

    if ( mapRangeCon.contains("staff") )
        conExps << QStringLiteral("staff='%1'").arg(mapRangeCon.value("staff"));

    if ( includeCargoRel ) {
        if ( mapRangeCon.contains("colortype") )
            conExps << QStringLiteral("colortype='%1'").arg(mapRangeCon.value("colortype"));

        if ( mapRangeCon.contains("sizertype") )
            conExps << QStringLiteral("sizertype='%1'").arg(mapRangeCon.value("sizertype"));
    }

    if ( mapRangeCon.contains("chktime") )
        conExps << QStringLiteral("chktime%1").arg(mapRangeCon.value("chktime"));

    return conExps;
}

QStringList BsQryWin::getSizersQtySplitSql(const QStringList &selExps,
                                           const QString &fromSource,
                                           const QStringList &conExps,
                                           const QString &sizerType,
                                           const QString &tmpTableName,
                                           const bool forStockk)            //仅用于进销存一览
{
    QStringList unionUnitSqls;
    QStringList sizerNames = dsSizer->getSizerList(sizerType);
    foreach (QString sizerName, sizerNames) {
        if ( !sizerName.isEmpty() ) {
            QStringList uSels;
            uSels << selExps;

            if ( forStockk )
                uSels << QStringLiteral("'%1' AS sizer, (CASE SUBSTR(sizers,2,1) WHEN '\v' THEN "
                                        "(CASE WHEN INSTR(sizers,'%2')>2 THEN "
                                        "CAST(SUBSTR(sizers,INSTR(sizers,'%2')+%3,"
                                        "INSTR(SUBSTR(sizers,INSTR(sizers,'%2'))||'\n','\n')-"
                                        "INSTR(SUBSTR(sizers,INSTR(sizers,'%2')),'\t')-1) AS INTEGER) "
                                        "ELSE 0 END) ELSE "
                                        "(CASE WHEN INSTR(sizers,'%2')>2 THEN "
                                        "-1*CAST(SUBSTR(sizers,INSTR(sizers,'%2')+%3,"
                                        "INSTR(SUBSTR(sizers,INSTR(sizers,'%2'))||'\n','\n')-"
                                        "INSTR(SUBSTR(sizers,INSTR(sizers,'%2')),'\t')-1) AS INTEGER) "
                                        "ELSE 0 END) "
                                        "END) AS qty")
                         .arg(sizerName).arg(sizerName + QChar(9)).arg(sizerName.length() + 1);
            else
                uSels << QStringLiteral("'%1' AS sizer, (CASE WHEN INSTR(sizers,'%2')>2 THEN "
                                        "CAST(SUBSTR(sizers,INSTR(sizers,'%2')+%3,"
                                        "INSTR(SUBSTR(sizers,INSTR(sizers,'%2'))||'\n','\n')-"
                                        "INSTR(SUBSTR(sizers,INSTR(sizers,'%2')),'\t')-1) AS INTEGER) "
                                        "ELSE 0 END) AS qty")
                         .arg(sizerName).arg(sizerName + QChar(9)).arg(sizerName.length() + 1);

            unionUnitSqls << QStringLiteral("SELECT %1 FROM %2 WHERE %3 ")
                          .arg(uSels.join(QChar(44))).arg(fromSource).arg(conExps.join(QStringLiteral(" AND ")));
        }
    }

    QStringList sqls;
    sqls << QStringLiteral("DROP TABLE IF EXISTS temp.%1;").arg(tmpTableName);
    sqls << QStringLiteral("CREATE TEMP TABLE %1 AS %2;")
            .arg(tmpTableName).arg(unionUnitSqls.join(QStringLiteral(" UNION ALL ")));

    return sqls;
}

QString BsQryWin::prepairViewAllData(const QSet<QString> &setSel,
                                     const QStringList &noTimeConExps)      //仅用于进销存一览
{
    //基本角度
    QStringList selExps;

    //if ( setSel.contains(QStringLiteral("cargo")) )
    selExps << QStringLiteral("cargo");

    if ( setSel.contains(QStringLiteral("color")) )
        selExps << QStringLiteral("color");

    //期末条件
    QStringList qmCons;
    qmCons << QStringLiteral("(dated<=%1)")
                  .arg(mpConDateE->mpEditor->getDataValue());
    qmCons << noTimeConExps;

    //期间条件
    QStringList periodCons;
    periodCons << QStringLiteral("(dated BETWEEN %1 AND %2)")
                  .arg(mpConDateB->mpEditor->getDataValue())
                  .arg(mpConDateE->mpEditor->getDataValue());
    periodCons << noTimeConExps;

    //各值数据源与数据名标
    QString qmTable = (mapRangeCon.contains("shop")) ? "vi_stock_attr" : "vi_stock_nodb_attr";
    QStringList dataTables;     //两调拨约定放最后，见后面代码
    dataTables << "vi_cgj_attr"
               << "vi_cgt_attr"
               << "vi_pff_attr"
               << "vi_pft_attr"
               << "vi_lsd_attr"
               << "vi_syd_attr"
               << "vi_dbd_attr"
               << "vi_dbr_attr";

    //批量执行语句
    QStringList sqls;

    //分拆尺码数量明细，改设数据源
    if ( mpSizerCheckor->isChecked() )
    {
        //必要条件
        QString sizerType = mpConSizerType->mpEditor->getDataValue();

        //期末库存数据
        QString tmpQm = QStringLiteral("tmp_%1_%2").arg(qmTable).arg(loginer);
        sqls << getSizersQtySplitSql(selExps, qmTable, qmCons, sizerType, tmpQm, true);

        //各期间数据
        QStringList tmpTables;
        for ( int i = 0, iLen = dataTables.length(); i < iLen; ++i ) {
            if ( i < iLen - 2 || mapRangeCon.contains("shop") ) {     //这就是上面为什么约定两调拨放最后的原因
                QString tmpTbl = QStringLiteral("tmp_%1_%2").arg(dataTables.at(i)).arg(loginer);
                sqls << getSizersQtySplitSql(selExps, dataTables.at(i), periodCons, sizerType, tmpTbl);
                tmpTables << tmpTbl;
            }
        }

        //角度添加（费好大力气得来的）
        selExps << "sizer";
    }

    //准备期末库存，同时创建最终JXC数据临时用表
    QStringList qmStockSels;
    qmStockSels << selExps;
    qmStockSels << QStringLiteral("qty AS stock");

    QString tmpViewAllTable = QStringLiteral("tmp_jxc_%1").arg(loginer);

    QString fromStockSource = ( mpSizerCheckor->isChecked() )
            ? QStringLiteral("tmp_%1_%2").arg(qmTable).arg(loginer)
            : qmTable;

    QString whereStockSql = ( mpSizerCheckor->isChecked() )
            ? QString()
            : QStringLiteral("WHERE %1").arg(qmCons.join(" AND "));

    sqls << QStringLiteral("DROP TABLE IF EXISTS temp.%1;").arg(tmpViewAllTable);
    sqls << QStringLiteral("CREATE TEMP TABLE %1 AS SELECT %2 FROM %3 %4;")
            .arg(tmpViewAllTable).arg(qmStockSels.join(QChar(44))).arg(fromStockSource).arg(whereStockSql);

    //追加各列数据
    for ( int i = 0, iLen = dataTables.length(); i < iLen; ++i )
    {
        if ( i < iLen - 2 || mapRangeCon.contains("shop") ) {
            //取列名
            QString dataTable = dataTables.at(i);
            QStringList nameParts = dataTable.split(QChar('_'));

            //增列
            QString dataColName = nameParts.at(1);  //第0位是vi_
            sqls << QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 INTEGER DEFAULT 0;")
                    .arg(tmpViewAllTable).arg(dataColName);

            //添数据
            QStringList vflds, sflds;
            vflds << selExps;
            sflds << selExps;
            if ( mpSizerCheckor->isChecked() ) {
                vflds << "sizer";
                sflds << "sizer";
            }
            vflds << dataColName;
            sflds << QStringLiteral("qty AS %1").arg(dataColName);

            QString fromColSource = ( mpSizerCheckor->isChecked() )
                    ? QStringLiteral("tmp_%1_%2").arg(dataTable).arg(loginer)
                    : dataTable;

            QString whereColSql = ( mpSizerCheckor->isChecked() )
                    ? QString()
                    : QStringLiteral("WHERE %1").arg(periodCons.join(" AND "));

            sqls << QStringLiteral("INSERT INTO %1(%2) SELECT %3 FROM %4 %5;")
                    .arg(tmpViewAllTable).arg(vflds.join(QChar(44))).arg(sflds.join(QChar(44)))
                    .arg(fromColSource).arg(whereColSql);
        }
    }

    //加期初列
    sqls << QStringLiteral("ALTER TABLE %1 ADD COLUMN base INTEGER DEFAULT 0;").arg(tmpViewAllTable);

    //加货品属性列
    for ( int i = 0, iLen = mpPnlSel->children().length(); i < iLen; ++i ) {
        BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlSel->children().at(i));
        if ( chkor ) {
            BsField *fld = chkor->mField;
            QString fname = fld->mFldName;
            bool byCargoCol = fname == QStringLiteral("hpname") || fname == QStringLiteral("setprice") || fname.startsWith(QStringLiteral("attr"));
            if ( chkor->isChecked() && byCargoCol ) {
                QString def = ( (fld->mFlags & bsffInt) == bsffInt )
                        ? QStringLiteral("INTEGER DEFAULT 0")
                        : QStringLiteral("TEXT DEFAULT ''");
                sqls << QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3;")
                        .arg(tmpViewAllTable).arg(fname).arg(def);
            }
        }
    }

    //计算期初
    if ( mapRangeCon.contains("shop") )
        sqls << QStringLiteral("UPDATE %1 SET base=ifnull(stock,0)-cgj+cgt+pff-pft+lsd-syd+dbd-dbr;").arg(tmpViewAllTable);
    else
        sqls << QStringLiteral("UPDATE %1 SET base=ifnull(stock,0)-cgj+cgt+pff-pft+lsd-syd;").arg(tmpViewAllTable);

    //更新货品属性
    for ( int i = 0, iLen = mpPnlSel->children().length(); i < iLen; ++i ) {
        BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlSel->children().at(i));
        if ( chkor ) {
            BsField *fld = chkor->mField;
            QString fname = fld->mFldName;
            bool byCargoCol = fname == QStringLiteral("hpname") || fname == QStringLiteral("setprice") || fname.startsWith(QStringLiteral("attr"));
            if ( chkor->isChecked() && byCargoCol ) {
                sqls << QStringLiteral("UPDATE %1 SET %2=(select %2 from cargo where cargo.hpcode=%1.cargo);")
                        .arg(tmpViewAllTable).arg(fname);
            }
        }
    }

    //批量执行
    QString strErr = sqliteCommit(sqls);
    if ( !strErr.isEmpty() )
        return QString();

    //返回最终全数据表名
    return tmpViewAllTable;
}

QString BsQryWin::doSqliteQuery()
{
    //条件范围
    mLabelPairs.clear();
    mapRangeCon.clear();

    if ( mpConDateB->mpEditor->isEnabled() ) {
        mapRangeCon.insert("dateb", mpConDateB->mpEditor->getDataValue());
        mLabelPairs << QStringLiteral("%1\t%2").arg(mpConDateB->getFieldCnName()).arg(mpConDateB->mpEditor->text());
    }

    mapRangeCon.insert("datee", mpConDateE->mpEditor->getDataValue());
    mLabelPairs << QStringLiteral("%1\t%2").arg(mpConDateE->getFieldCnName()).arg(mpConDateE->mpEditor->text());

    if ( mpConStype->isVisible() && !mpConStype->mpEditor->getDataValue().isEmpty() ) {
        mapRangeCon.insert(mpConStype->getFieldName(), mpConStype->mpEditor->getDataValue());
        mLabelPairs << QStringLiteral("%1\t%2").arg(mpConStype->getFieldCnName()).arg(mpConStype->mpEditor->text());
    }

    if ( mpConStaff->isVisible() && !mpConStaff->mpEditor->getDataValue().isEmpty() ) {
        mapRangeCon.insert(mpConStaff->getFieldName(), mpConStaff->mpEditor->getDataValue());
        mLabelPairs << QStringLiteral("%1\t%2").arg(mpConStaff->getFieldCnName()).arg(mpConStaff->mpEditor->text());
    }

    if ( mpConShop->isVisible() && !mpConShop->mpEditor->getDataValue().isEmpty() ) {
        mapRangeCon.insert(mpConShop->getFieldName(), mpConShop->mpEditor->getDataValue());
        mLabelPairs << QStringLiteral("%1\t%2").arg(mpConShop->getFieldCnName()).arg(mpConShop->mpEditor->text());
    }

    if ( mpConTrader->isVisible() && !mpConTrader->mpEditor->getDataValue().isEmpty() ) {
        mapRangeCon.insert(mpConTrader->getFieldName(), mpConTrader->mpEditor->getDataValue());
        mLabelPairs << QStringLiteral("%1\t%2").arg(mpConTrader->getFieldCnName()).arg(mpConTrader->mpEditor->text());
    }

    if ( mpConCargo->isVisible() && !mpConCargo->mpEditor->getDataValue().isEmpty() ) {
        mapRangeCon.insert(mpConCargo->getFieldName(), mpConCargo->mpEditor->getDataValue());
        mLabelPairs << QStringLiteral("%1\t%2").arg(mpConCargo->getFieldCnName()).arg(mpConCargo->mpEditor->text());
    }

    if ( mpConColorType && mpConSizerType ) {
        if ( mpConColorType->isVisible() && !mpConColorType->mpEditor->getDataValue().isEmpty() ) {
            mapRangeCon.insert(mpConColorType->getFieldName(), mpConColorType->mpEditor->getDataValue());
            mLabelPairs << QStringLiteral("%1\t%2").arg(mpConColorType->getFieldCnName()).arg(mpConColorType->mpEditor->text());
        }

        if ( mpConSizerType->isVisible() && !mpConSizerType->mpEditor->getDataValue().isEmpty() ) {
            mapRangeCon.insert(mpConSizerType->getFieldName(), mpConSizerType->mpEditor->getDataValue());
            mLabelPairs << QStringLiteral("%1\t%2").arg(mpConSizerType->getFieldCnName()).arg(mpConSizerType->mpEditor->text());
        }
    }

    QString currentChkConVal;

    if ( mpConCheck->isTristate() ) {
        if ( mpConCheck->checkState() == Qt::Checked ) {
            currentChkConVal = "<>0";
            mLabelPairs << QStringLiteral("%1\t%2").arg(mapMsg.value("word_check_range")).arg(mapMsg.value("word_only_checked"));
        }
        else if ( mpConCheck->checkState() == Qt::Unchecked ) {
            currentChkConVal = "=0";
            mLabelPairs << QStringLiteral("%1\t%2").arg(mapMsg.value("word_check_range")).arg(mapMsg.value("word_not_checked"));
        }
        else
            mLabelPairs << QStringLiteral("%1\t%2").arg(mapMsg.value("word_check_range")).arg(mapMsg.value("word_any_checked"));
    }
    else {
        if ( mpConCheck->checkState() == Qt::Checked ) {
            currentChkConVal = "<>0";
            mLabelPairs << QStringLiteral("%1\t%2").arg(mapMsg.value("word_check_range")).arg(mapMsg.value("word_only_checked"));
        }
        else
            mLabelPairs << QStringLiteral("%1\t%2").arg(mapMsg.value("word_check_range")).arg(mapMsg.value("word_any_checked"));
    }

    if ( !currentChkConVal.isEmpty() ) {
        mapRangeCon.insert("chktime", currentChkConVal);
    }


    //统计角度
    QSet<QString>   setSel;
    for ( int i = 0, iLen = mpPnlSel->children().length(); i < iLen; ++i )
    {
        BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlSel->children().at(i));
        if ( chkor ) {
            if ( chkor->isChecked() ) {
                if ( !mapRangeCon.contains(chkor->mField->mFldName) || chkor->mField->mFldName == QStringLiteral("cargo") ) {
                    setSel.insert(chkor->mField->mFldName);
                }
            }
        }
    }


    //计算数值预备（确保至少有一个选择）
    bool noneCheck = true;
    for ( int i = 0, iLen = mpPnlVal->children().length(); i < iLen; ++i )
    {
        BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlVal->children().at(i));
        if ( chkor ) {
            if ( chkor->isChecked() ) {
                noneCheck = false;
                break;
            }
        }
    }
    if ( noneCheck ) {
        for ( int i = 0, iLen = mpPnlVal->children().length(); i < iLen; ++i )
        {
            BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlVal->children().at(i));
            if ( chkor ) {
                if ( chkor->isEnabled() ) {
                    chkor->setChecked(true);
                    break;
                }
            }
        }
    }

    //计算数值正式获取
    QSet<QString>   setVal;
    for ( int i = 0, iLen = mpPnlVal->children().length(); i < iLen; ++i )
    {
        BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlVal->children().at(i));
        if ( chkor ) {
            if ( chkor->isChecked() ) {
                setVal.insert(chkor->mField->mFldName);
            }
        }
    }

    //判断是否用_attr视图
    bool useAttr = false;

    if ( mapRangeCon.contains("setprice") ||
         mapRangeCon.contains("unit") ||
         mapRangeCon.contains("colortype") ||
         mapRangeCon.contains("sizertype") )
        useAttr = true;

    if ( setSel.contains("setprice") ||
         setSel.contains("hpname") ||
         setSel.contains("unit") ||
         setSel.contains("attr1") ||
         setSel.contains("attr2") ||
         setSel.contains("attr3") ||
         setSel.contains("attr4") ||
         setSel.contains("attr5") ||
         setSel.contains("attr6") )
        useAttr = true;


    //如果是库存，判断是否用_nodb视图
    bool useNodb = false;
    if ( mMainTable.contains("stock") ) {
        useNodb = !mapRangeCon.contains("shop") && !setSel.contains("shop");
    }


    //数据源
    mFromSource = mMainTable;

    if ( useNodb )
        mFromSource += ("_nodb");

    if ( useAttr )
        mFromSource += ("_attr");

    //select exps ( grp, sum, order )
    QStringList selExps;
    QStringList grpFlds;
    QStringList ordFlds;
    QStringList cnameDefines;

    for ( int i = 0, iLen = mpPnlSel->children().length(); i < iLen; ++i ) {    //不用QSetIterator是因为QSet无序
        BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlSel->children().at(i));
        if ( chkor && chkor->isChecked() ) {
            QString fldName = chkor->mField->mFldName;
            if ( (!mapRangeCon.contains(fldName) || fldName == QStringLiteral("cargo") ) && fldName != QStringLiteral("sizers") ) {
                selExps << fldName;
                grpFlds << fldName;
                ordFlds << fldName;
                cnameDefines << QStringLiteral("%1\t%2").arg(fldName).arg(chkor->text());
            }
        }
    }

    if ( (mQryFlags & bsqtViewAll) != bsqtViewAll )
    {
        for ( int i = 0, iLen = mpPnlVal->children().length(); i < iLen; ++i ) {    //不用QSetIterator是因为QSet无序
            BsQryCheckor *chkor = qobject_cast<BsQryCheckor*>(mpPnlVal->children().at(i));
            if ( chkor ) {
                QString fldName = chkor->mField->mFldName;

                if ( (mQryFlags & bsqtSumCash) != bsqtSumCash ) {
                    if ( fldName == QStringLiteral("sumqty") )
                        fldName = QStringLiteral("qty");

                    if ( fldName == QStringLiteral("summoney") )
                        fldName = QStringLiteral("actmoney");

                    if ( fldName == QStringLiteral("sumdis") )
                        fldName = QStringLiteral("dismoney");
                }

                if ( chkor->isChecked() ) {
                    selExps << QStringLiteral("SUM(%1.%2) AS %2").arg(mFromSource).arg(fldName);
                    cnameDefines << QStringLiteral("%1\t%2").arg(fldName).arg(chkor->text());
                }
            }
        }

        if ( setSel.contains("sizers") ) {
            selExps << QStringLiteral("GROUP_CONCAT(%1.sizers, '') AS sizers").arg(mFromSource);
        }
    }
    else
    {
        if ( mpSizerCheckor->isChecked() )
        {
            selExps << "sizer";
            grpFlds << "sizer";
            ordFlds << "sizer";
        }

        selExps << "SUM(base) AS base" << "SUM(cgj) AS cgj" << "SUM(cgt) AS cgt";

        if ( mapRangeCon.contains("shop") )
            selExps << "SUM(dbd) AS dbd" << "SUM(dbr) AS dbr";

        selExps << "SUM(syd) AS syd" << "SUM(pff) AS pff" << "SUM(pft) AS pft" << "SUM(lsd) AS lsd"
                << "SUM(stock) AS stock";
    }


    //进销存一览特别处理
    if ( (mQryFlags & bsqtViewAll) == bsqtViewAll )
        mFromSource = prepairViewAllData(setSel, getConExpPairsFromMapRangeCon(false));
    if ( mFromSource.isEmpty() )
        return mapMsg.value("i_qry_execute_failed");

    //havSql
    QString havSql;
    if ( grpFlds.length() > 0 && mpHaving->isVisible() && !mpHaving->isChecked() ) {
        havSql = ( mMainTable.contains("stock") )
                ? QStringLiteral("HAVING SUM(qty)<>0")          //库存
                : QStringLiteral("HAVING SUM(qty)>0");          //订单欠货
        if ( (mQryFlags & bsqtViewAll) == bsqtViewAll )
            havSql = QStringLiteral("HAVING SUM(stock)<>0");    //一览
    }

    //whereSql
    QStringList conExps = getConExpPairsFromMapRangeCon(true);
    QString whereSql = ( (mQryFlags & bsqtViewAll) == bsqtViewAll )
            ? QString()
            : QStringLiteral("WHERE %1").arg(conExps.join(QStringLiteral(" AND ")));

    //grpSql
    QString grpSql = (grpFlds.length() > 0) ? QStringLiteral("GROUP BY %1").arg(grpFlds.join(QChar(44))) : QString();

    //orderSql
    QString orderSql = (ordFlds.length() > 0) ? QStringLiteral("ORDER BY %1").arg(ordFlds.join(QChar(44))) : QString();

    //最终SQL
    QString sql = QStringLiteral("SELECT %1 FROM %2 %3 %4 %5 %6;")
            .arg(selExps.join(QChar(44))).arg(mFromSource).arg(whereSql).arg(grpSql).arg(havSql).arg(orderSql);

    //刷新表格
    QString useSizerType = ( mpConSizerType ) ? mpConSizerType->mpEditor->getDataValue() : QString();
    mpQryGrid->loadData(sql, cnameDefines, useSizerType);

    //加载列宽
    mpQryGrid->loadColWidths();

    //无错返回
    return QString();
}


// BsAbstractFormWin
BsAbstractFormWin::BsAbstractFormWin(QWidget *parent, const QString &name, const QStringList &fields, const uint bsWinType)
    : BsWin(parent, name, fields, bsWinType)
{
    //主按钮
    mpAcMainNew = mpToolBar->addAction(QIcon(":/icon/new.png"), QString(), this, SLOT(clickNew()));
    mpAcMainEdit = mpToolBar->addAction(QIcon(":/icon/edit.png"), QString(), this, SLOT(clickEdit()));
    mpAcMainDel = mpToolBar->addAction(QIcon(":/icon/del.png"), QString(), this, SLOT(clickDel()));
    mpAcMainSave = mpToolBar->addAction(QIcon(":/icon/save.png"), QString(), this, SLOT(clickSave()));
    mpAcMainCancel = mpToolBar->addAction(QIcon(":/icon/cancel.png"), QString(), this, SLOT(clickCancel()));

    mpAcMainNew->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
    mpAcMainEdit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
    mpAcMainSave->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));

    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainNew);
    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainEdit);
    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainDel);
    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainSave);
    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainCancel);

    //工具箱
    mpAcToolImport = mpMenuToolCase->addAction(QIcon(), mapMsg.value("tool_import_data"),
                                               this, SLOT(clickToolImport()));

    mpMenuToolCase->addSeparator();

    mpToolHideCurrentCol = mpMenuToolCase->addAction(mapMsg.value("tool_hide_current_col"));    //事件槽表格尚未创建，后代connect
    mpToolHideCurrentCol->setProperty(BSACFLAGS,  0);
    mpToolHideCurrentCol->setProperty(BSACRIGHT, true);

    mpToolShowAllCols = mpMenuToolCase->addAction(mapMsg.value("tool_show_all_cols"));    //事件槽表格尚未创建，后代connect
    mpToolShowAllCols->setProperty(BSACFLAGS,  0);
    mpToolShowAllCols->setProperty(BSACRIGHT, true);

    //开关盒
    mpAcOptHideDropRow = mpMenuOptionBox->addAction(QIcon(), mapMsg.value("opt_hide_drop_red_row"),
                                                    this, SLOT(clickOptHideDropRow()));
    mpAcOptHideDropRow->setProperty("optname", "opt_hide_drop_red_row");

    mpAcMainNew->setProperty(BSACFLAGS, bsacfClean);
    mpAcMainEdit->setProperty(BSACFLAGS, bsacfClean  | bsacfPlusId | bsacfNotChk);
    mpAcMainDel->setProperty(BSACFLAGS, bsacfClean | bsacfPlusId | bsacfNotChk);
    mpAcMainSave->setProperty(BSACFLAGS, bsacfDirty);
    mpAcMainCancel->setProperty(BSACFLAGS, bsacfDirty);
    mpAcToolImport->setProperty(BSACFLAGS, bsacfDirty);
    mpAcOptHideDropRow->setProperty(BSACFLAGS, bsacfDirty);

    mpAcMainSave->setProperty(BSACRIGHT, true);
    mpAcMainCancel->setProperty(BSACRIGHT, true);

    //状态栏
    mpSttValKey = new QLabel(this);
    mpSttValKey->setMinimumWidth(100);

    mpSttLblUpman = new QLabel(mapMsg.value("fld_upman").split(QChar(9)).at(0), this);
    mpSttLblUpman->setStyleSheet("color:#666; font-weight:900;");

    mpSttValUpman = new QLabel(this);
    mpSttValUpman->setMinimumWidth(50);

    mpSttLblUptime = new QLabel(mapMsg.value("fld_uptime").split(QChar(9)).at(0), this);
    mpSttLblUptime->setStyleSheet("color:#666; font-weight:900;");

    mpSttValUptime = new QLabel(this);
    mpSttValUptime->setMinimumWidth(150);

    mpStatusBar->addWidget(mpSttValKey);
    mpStatusBar->addWidget(mpSttLblUpman);
    mpStatusBar->addWidget(mpSttValUpman);
    mpStatusBar->addWidget(mpSttLblUptime);
    mpStatusBar->addWidget(mpSttValUptime);
}

QSize BsAbstractFormWin::sizeHint() const
{
    QSettings settings;
    settings.beginGroup(BSR17WinSize);
    QStringList winSize = settings.value(mMainTable).toString().split(QChar(','));
    int winW = ( winSize.length() == 2 ) ? QString(winSize.at(0)).toInt() : 800;
    int winH = ( winSize.length() == 2 ) ? QString(winSize.at(1)).toInt() : 550;
    if ( winW <= 0 ) winW = 800;
    if ( winH <= 0 ) winH = 550;
    settings.endGroup();
    return QSize(winW, winH);
}

void BsAbstractFormWin::setEditable(const bool editt)
{
    mEditable = editt;

    //状态
    uint stateFlags;

    if ( editt )
        stateFlags = bsacfDirty;
    else
        stateFlags = bsacfClean;

    if ( isSheetChecked() )
        stateFlags |= bsacfChecked;
    else
        stateFlags |= bsacfNotChk;

    if ( isValidRealSheetId() )
        stateFlags |= bsacfPlusId;
    else
        stateFlags |= bsacfNotPlusId;

    //主按钮
    for ( int i = 0, iLen = mpToolBar->actions().length(); i < iLen; ++i ) {
        QAction *act = mpToolBar->actions().at(i);
        uint acFlags = act->property(BSACFLAGS).toUInt();
        bool rightAllow = act->property(BSACRIGHT).toBool();
        act->setEnabled(rightAllow && (stateFlags & acFlags) == acFlags);
    }

    //工具箱
    for ( int i = 0, iLen = mpMenuToolCase->actions().length(); i < iLen; ++i ) {
        QAction *act = mpMenuToolCase->actions().at(i);
        uint acFlags = act->property(BSACFLAGS).toUInt();
        bool rightAllow = act->property(BSACRIGHT).toBool();
        act->setEnabled(rightAllow && (stateFlags & acFlags) == acFlags);
    }

    //开关盒
    for ( int i = 0, iLen = mpMenuOptionBox->actions().length(); i < iLen; ++i ) {
        QAction *act = mpMenuOptionBox->actions().at(i);
        uint acFlags = act->property(BSACFLAGS).toUInt();
        bool rightAllow = act->property(BSACRIGHT).toBool();
        act->setEnabled(rightAllow && (stateFlags & acFlags) == acFlags);
    }

    //附加切换不同状态下的提示
    mpAcMainHelp->setStatusTip((editt) ? mapMsg.value("i_edit_mode_tip") : mapMsg.value("i_read_mode_tip"));
    mpFormGrid->setStatusTip((editt) ? mapMsg.value("i_edit_mode_tip") : mapMsg.value("i_read_mode_tip"));
}

void BsAbstractFormWin::closeEvent(QCloseEvent *e)
{
    //编辑态禁止关闭，但无明细空表格除外
    if ( mainNeedSaveDirty() || mpFormGrid->needSaveDirty() ) {

        if ( ! confirmDialog(this,
                           QStringLiteral("窗口处于编辑状态，直接关闭窗口会放弃最近未保存的修改和编辑。"),
                           QStringLiteral("确定要放弃保存吗？"),
                           mapMsg.value("btn_giveup_save"),
                           mapMsg.value("btn_cancel"),
                           QMessageBox::Question) ) {
            e->ignore();
            return;
        }
    }

    //保存窗口大小
    QSettings settings;
    settings.beginGroup(BSR17WinSize);
    settings.setValue(mMainTable, QStringLiteral("%1,%2").arg(width()).arg(height()));
    settings.endGroup();

    //保存表格列宽
    mpFormGrid->saveColWidths();

    //完成
    e->accept();

    //必须，里面有保存列宽
    BsWin::closeEvent(e);
}

void BsAbstractFormWin::clickOptHideDropRow()
{
    mpFormGrid->setDroppedRowByOption(mpAcOptHideDropRow->isChecked());
}


// BsRegWin
BsRegWin::BsRegWin(QWidget *parent, const QString &name, const QStringList &fields)
    : BsAbstractFormWin(parent, name, fields, bswtReg)
{
    //用于权限判断
    mRightWinName = name;
    //Q_ASSERT(lstRegisWinTableNames.indexOf(mRightWinName) >= 0);
    //由于barcode等也是用的BsRegWin但没有权限细设，所以不能用此断言。
    //此断言主要怕vi类命名太多没搞一致，这儿用不用没关系。

    //禁权字段
    if ( !canRett ) mDenyFields << "retprice";
    if ( !canLott ) mDenyFields << "lotprice";
    if ( !canBuyy ) mDenyFields << "buyprice";

    //首先，具体化mFields（包括列名、帮助提示、特性、……）
    BsField *fld = nullptr;

    fld = getFieldByName("upman");      Q_ASSERT(fld);
    fld->mFlags |= bsffGrid;

    fld = getFieldByName("uptime");     Q_ASSERT(fld);
    fld->mFlags |= bsffGrid;

    if ( name == QStringLiteral("sizertype") )
    {
        fld = getFieldByName("tname");      Q_ASSERT(fld);
        fld->mFldCnName = QStringLiteral("品类名");

        fld = getFieldByName("namelist");      Q_ASSERT(fld);
        fld->mFldCnName = QStringLiteral("各尺码从小到大依次列举");
    }

    if ( name == QStringLiteral("colortype") )
    {
        fld = getFieldByName("tname");      Q_ASSERT(fld);
        fld->mFldCnName = QStringLiteral("系列名");

        fld = getFieldByName("namelist");      Q_ASSERT(fld);
        fld->mFldCnName = QStringLiteral("各颜色逐一列举");
    }

    mpSttValUpman->setMinimumWidth(100);

    //具体化help_tip
    mGuideObjectTip = mapMsg.value(QStringLiteral("win_%1").arg(name)).split(QChar(9)).at(1);
    mGuideClassTip = mapMsg.value("win_<reg>").split(QChar(9)).at(1);
    mpGuide->setText(mGuideClassTip);

    //新编辑模式按钮
    QAction *acEditMode = mpToolBar->addAction(QIcon(":/icon/edit.png"),
                                               mapMsg.value("btn_reg_edit").split(QChar(9)).at(0),
                                               this, SLOT(doOpenEditMode()));
    mpToolBar->insertAction(mpAcMainNew, acEditMode);
    acEditMode->setStatusTip(mapMsg.value("btn_reg_edit").split(QChar(9)).at(1));
    acEditMode->setProperty(BSACFLAGS, bsacfClean);
    acEditMode->setProperty(BSACRIGHT, canDo(mRightWinName, bsrrNew) ||
                                        canDo(mRightWinName, bsrrUpd) ||
                                        canDo(mRightWinName, bsrrDel));

    //重置
    mpAcMainNew->setText(mapMsg.value("btn_reg_new").split(QChar(9)).at(0));
    mpAcMainNew->setStatusTip(mapMsg.value("btn_reg_new").split(QChar(9)).at(1));
    mpAcMainNew->setIcon(QIcon(":/icon/add.png"));
    mpAcMainNew->setProperty(BSACFLAGS, bsacfDirty);
    mpAcMainNew->setProperty(BSACRIGHT, canDo(mRightWinName, bsrrNew));

    mpAcMainEdit->setVisible(false);
    mpAcMainDel->setVisible(false);

    mpAcMainSave->setText(mapMsg.value("btn_reg_save").split(QChar(9)).at(0));
    mpAcMainSave->setStatusTip(mapMsg.value("btn_reg_save").split(QChar(9)).at(1));

    mpAcMainCancel->setText(mapMsg.value("btn_reg_cancel").split(QChar(9)).at(0));
    mpAcMainCancel->setStatusTip(mapMsg.value("btn_reg_cancel").split(QChar(9)).at(1));

    mpAcOptHideDropRow->setProperty(BSACRIGHT, canDo(mRightWinName, bsrrDel));
    mpAcToolImport->setProperty(BSACRIGHT, canDo(mRightWinName, bsrrNew));
    mpAcToolExport->setProperty(BSACRIGHT, canDo(mRightWinName, bsrrExport));
    mpAcToolExport->setProperty(BSACFLAGS, bsacfClean);


    //工具箱（没有设计表格行变化动态更新按钮的事件，因此RegWin不要使用bsacfPlusId与bsacfChecked等相关位）
    if ( name == QStringLiteral("cargo") ) {
        mpToolCreateColorType = mpMenuToolCase->addAction(mapMsg.value("tool_create_colortype"),
                                                       this, SLOT(clickToolCreateColorType()));
        mpToolCreateColorType->setProperty(BSACFLAGS, bsacfClean);
        mpToolCreateColorType->setProperty(BSACRIGHT, canDo(mRightWinName, bsrrNew));

        mpToolAlarmSetting = mpMenuToolCase->addAction(mapMsg.value("tool_alarm_setting"),
                                                       this, SLOT(clickToolAlarmSetting()),
                                                       QKeySequence(QStringLiteral("Ctrl+J")));
        mpToolAlarmSetting->setProperty(BSACFLAGS, bsacfClean);
        mpToolAlarmSetting->setProperty(BSACRIGHT, loginAsAdminOrBoss);

        mpToolAlarmRemove = mpMenuToolCase->addAction(mapMsg.value("tool_alarm_remove"),
                                                      this, SLOT(clickToolAlarmRemove()),
                                                      QKeySequence(QStringLiteral("Ctrl+Shift+J")));
        mpToolAlarmRemove->setProperty(BSACFLAGS, bsacfClean);
        mpToolAlarmRemove->setProperty(BSACRIGHT, loginAsAdminOrBoss);
    }

    //开关盒
    QAction* acOptNewRowMode = mpMenuOptionBox->addAction(QIcon(), mapMsg.value("opt_reg_new_row_directly"));
    acOptNewRowMode->setProperty("optname", "opt_reg_new_row_directly");
    //acOptNewRowMode->setProperty(BSACFLAGS, bsacfDirty | bsacfClean);
    acOptNewRowMode->setProperty(BSACRIGHT, true);

    //爱美功能————定坐标
    if ( name == QStringLiteral("shop") ) {
        mpAcAmGeo = mpToolBar->addAction(QIcon(":/icon/geo.png"), mapMsg.value("btn_reg_geo").split(QChar(9)).at(0),
                                         this, SLOT(clickAmGeo()));
        mpAcAmGeo->setStatusTip(mapMsg.value("btn_reg_geo").split(QChar(9)).at(1));
        mpToolBar->insertAction(mpAcToolSeprator, mpAcAmGeo);
        mpAcAmGeo->setProperty(BSACFLAGS, bsacfClean);
        mpAcAmGeo->setProperty(BSACRIGHT, loginAsAdminOrBoss);

        mpToolAmUnGeo = mpMenuToolCase->addAction(QStringLiteral("删除本店坐标定位"), this, SLOT(clickToolAmUnGeo()));
        mpToolAmUnGeo->setProperty(BSACFLAGS, bsacfClean);
        mpToolAmUnGeo->setProperty(BSACRIGHT, loginAsAdminOrBoss);
    }

    //爱美功能————打标签
    if ( name == QStringLiteral("cargo") ) {
        mpAcAmTag = mpToolBar->addAction(QIcon(":/icon/tag.png"), mapMsg.value("btn_reg_tag").split(QChar(9)).at(0),
                                         this, SLOT(clickAmTag()));
        mpAcAmTag->setStatusTip(mapMsg.value("btn_reg_tag").split(QChar(9)).at(1));
        mpToolBar->insertAction(mpAcToolSeprator, mpAcAmTag);
        mpAcAmTag->setProperty(BSACFLAGS, bsacfClean);
        mpAcAmTag->setProperty(BSACRIGHT, loginAsAdminOrBoss);

        mpToolAmUnTag = mpMenuToolCase->addAction(QStringLiteral("删除平台标签"), this, SLOT(clickToolAmUnTag()));
        mpToolAmUnTag->setProperty(BSACFLAGS, bsacfClean);
        mpToolAmUnTag->setProperty(BSACRIGHT, loginAsAdminOrBoss);
    }

    //表格
    mpRegGrid = new BsRegGrid(this, name, mFields);
    mpGrid = mpRegGrid;
    mpFormGrid = mpRegGrid;

    mpRegGrid->setAllowFlags(canDo(mRightWinName, bsrrNew), canDo(mRightWinName, bsrrUpd), canDo(mRightWinName, bsrrDel));
    mpRegGrid->mDenyFields << mDenyFields;

    connect(mpToolHideCurrentCol, SIGNAL(triggered(bool)), mpRegGrid, SLOT(hideCurrentCol()));
    connect(mpToolShowAllCols, SIGNAL(triggered(bool)), mpRegGrid, SLOT(showHiddenCols()));

    //布局
    QVBoxLayout *lay = new QVBoxLayout(mpBody);
    lay->setContentsMargins(3, 0, 3, 0);
    lay->addWidget(mpRegGrid, 1);
    setWindowTitle(mapMsg.value("main_register") + mapMsg.value(QStringLiteral("win_%1").arg(name)).split(QChar(9)).at(0));

    //信号槽
    connect(mpRegGrid, SIGNAL(shootHintMessage(QString)),  this, SLOT(displayGuideTip(QString)));
    connect(mpRegGrid, SIGNAL(shootForceMessage(QString)), this, SLOT(forceShowMessage(QString)));
    connect(mpRegGrid, SIGNAL(filterDone()), mpToolBar, SLOT(hide()));
    connect(mpRegGrid, SIGNAL(filterEmpty()), mpToolBar, SLOT(show()));
    connect(mpRegGrid, SIGNAL(shootCurrentRowSysValue(QStringList)), this, SLOT(showStatus(QStringList)));
}

void BsRegWin::showEvent(QShowEvent *e)
{
    BsAbstractFormWin::showEvent(e);

    if ( mpRegGrid->rowCount() > 0 )
        return;

    //加载数据
    QStringList sels;
    for ( int i = 0, iLen = mFields.length(); i < iLen; ++i )
        sels << mFields.at(i)->mFldName;

    QString sql = QStringLiteral("SELECT %1 FROM %2 ORDER BY %3;")
            .arg(sels.join(QChar(44))).arg(mMainTable).arg(sels.at(0));
    mpRegGrid->loadData(sql);

    //加载列宽
    mpRegGrid->loadColWidths();

    //页脚
    if ( mMainTable != QStringLiteral("cargo") )
        mpRegGrid->hideFooterText();

    //可编辑性
    mpRegGrid->setEditable(false);
    this->setEditable(false);

    //用户设置
    mpRegGrid->setFontPoint(9);
    mpRegGrid->setRowHeight(22);
}

void BsRegWin::closeEvent(QCloseEvent *e)
{
    BsAbstractFormWin::closeEvent(e);

    if ( mMainTable == QStringLiteral("sizertype") )
        dsSizer->reload();

    if ( mMainTable == QStringLiteral("colortype") ) {
        dsColorType->reload();
        dsColorList->reload();
    }

    if ( mMainTable == QStringLiteral("cargo") )
        dsCargo->reload();

    if ( mMainTable == QStringLiteral("shop") )
        dsShop->reload();

    if ( mMainTable == QStringLiteral("customer") )
        dsCustomer->reload();

    if ( mMainTable == QStringLiteral("supplier") )
        dsSupplier->reload();
}

void BsRegWin::doNew()
{
    bool optDirect = getOptValueByOptName(QStringLiteral("opt_reg_new_row_directly"));
    if ( optDirect ) {
        mpRegGrid->checkCreateNewRow();
        return;
    }

    BsDialog dlg(this, true, mMainTable, mFields);
    dlg.setWindowTitle(mapMsg.value("word_append") + mapMsg.value(QStringLiteral("reg_%1").arg(mMainTable)));
    if ( dlg.exec() == QDialog::Accepted ) {
        QStringList flds;
        QStringList txts;
        for ( int i = 0, iLen = dlg.mEditors.length(); i < iLen; ++i ) {
            BsFldEditor *edt = dlg.mEditors.at(i)->mpEditor;
            flds << edt->mpField->mFldName;
            QString txt = edt->text();
            if ( edt->mpField->mFldName == QStringLiteral("setprice") ) {
                bool ok;
                double val = txt.toDouble(&ok);
                if ( (val < 0.001 && val > -0.001) || !ok ) {
                    txt = "1000.0";
                }
            }
            txts << txt;
        }

        mpRegGrid->appendNewRow();
        int row = mpRegGrid->rowCount() - 1;
        for ( int i = 0, iLen = mpRegGrid->mCols.length(); i < iLen; ++i ) {
            BsField *fld = mpRegGrid->mCols.at(i);
            int idx = flds.indexOf(fld->mFldName);
            if ( idx >= 0 ) {
                QTableWidgetItem *cell = mpRegGrid->item(row, i);
                cell->setText(txts.at(idx));
                cell->setData(Qt::ForegroundRole, QColor(Qt::darkGreen));
            }
        }
        mpRegGrid->item(row, 0)->setData(Qt::UserRole + OFFSET_EDIT_STATE, bsesNew);
        mpRegGrid->setCurrentCell(row, 0);
    }
}

void BsRegWin::doSave()
{    
    uint ret = mpRegGrid->saveCheck();
    if ( ret == bsccError )
    {
        QMessageBox::critical(nullptr, QString(), mapMsg.value("i_save_found_error"));
        return;
    }
    else if ( ret == bsccWarning )
    {
        if ( ! confirmDialog(this,
                             mapMsg.value("i_save_found_warning"),
                             mapMsg.value("i_save_ask_warning"),
                             mapMsg.value("btn_ok"),
                             mapMsg.value("btn_cancel"),
                             QMessageBox::Warning) )
            return;
    }
    QStringList sqls = mpRegGrid->getSqliteSaveSql().split(QStringLiteral(";\n"), QString::SkipEmptyParts);
    QString sqlErr = sqliteCommit(sqls);
    if ( sqlErr.isEmpty() )
    {
        mpRegGrid->savedReconcile();
        mpRegGrid->setEditable(false);
        setEditable(false);
    }
    else
    {
        QMessageBox::information(nullptr, mapMsg.value("app_name"), sqlErr);
    }
}

void BsRegWin::doCancel()
{
    mpRegGrid->cancelRestore();
    mpRegGrid->setEditable(false);
    setEditable(false);
}

void BsRegWin::doToolImport()
{
    BsImportRegDlg dlg(this, mpRegGrid);
    dlg.adjustSize();
    dlg.exec();
}

bool BsRegWin::isValidRealSheetId()
{
    int row = mpGrid->currentRow();
    return row >= 0 && !mpGrid->item(row, 0)->text().trimmed().isEmpty();
}

void BsRegWin::doOpenEditMode()
{
    mpRegGrid->setEditable(true);
    setEditable(true);
    mpGuide->setText(mapMsg.value("i_edit_mode_tip"));
}

void BsRegWin::showStatus(const QStringList &values)
{
    Q_ASSERT(values.length()==3);

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(1000 * QString(values.at(2)).toLongLong());

    mpSttValKey->setText(values.at(0));
    mpSttValUpman->setText(values.at(1));
    mpSttValUptime->setText(dt.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
}

void BsRegWin::clickToolAlarmSetting()
{
    int row = mpRegGrid->currentRow();
    if ( row < 0 ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_need_pick_one_grid_row"));
        return;
    }
    QString cargo = mpRegGrid->item(row, 0)->text();
    QString colorType = dsCargo->getValue(cargo, "colortype");
    QString sizerType = dsCargo->getValue(cargo, "sizertype");
    if ( colorType.isEmpty() || sizerType.isEmpty() )
        QMessageBox::information(this, QString(), mapMsg.value("i_need_sizertype_befor_alarm_setting"));
    else {
        BsAlarmSetting dlg(cargo, this);
        dlg.setWindowTitle(mapMsg.value("tool_alarm_setting"));
        if ( dlg.exec() == QDialog::Accepted ) {
            QString ret = dlg.saveData();
            QStringList limits = ret.split(QChar(','));
            int minCol = mpRegGrid->getColumnIndexByFieldName("almin");
            int maxCol = mpRegGrid->getColumnIndexByFieldName("almax");
            mpRegGrid->item(row, minCol)->setText(limits.at(0));
            mpRegGrid->item(row, maxCol)->setText(limits.at(1));
        }
    }
}

void BsRegWin::clickToolAlarmRemove()
{
    int row = mpRegGrid->currentRow();
    if ( row < 0 ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_need_pick_one_grid_row"));
        return;
    }
    QString cargo = mpRegGrid->item(mpRegGrid->currentRow(), 0)->text();
    BsAlarmSetting::removeData(cargo);
    int minCol = mpRegGrid->getColumnIndexByFieldName("almin");
    int maxCol = mpRegGrid->getColumnIndexByFieldName("almax");
    mpRegGrid->item(row, minCol)->setText(QString());
    mpRegGrid->item(row, maxCol)->setText(QString());
}

void BsRegWin::clickToolCreateColorType()
{
    int row = mpRegGrid->currentRow();
    if ( row < 0 ) return;

    QString cargo = mpRegGrid->item(row, 0)->text();
    if ( cargo.isEmpty() ) return;

    int colorColIdx = mpGrid->getColumnIndexByFieldName(QStringLiteral("colortype"));
    if ( colorColIdx < 0 ) return;

    BsCreateColorTypeDlg dlg(cargo, this);
    if ( dlg.exec() == QDialog::Accepted ) {
        QString typeName = dlg.getColorTypeName();
        QString namelist = dlg.getPickeds().join(QChar(','));

        mpRegGrid->item(row, colorColIdx)->setText(typeName);

        QString sql = QStringLiteral("insert or replace into colortype(tname, namelist) "
                                     "values('%1', '%2');").arg(typeName).arg(namelist);
        QSqlDatabase db = QSqlDatabase::database();
        db.exec(sql);
        if ( db.lastError().isValid() ) qDebug() << db.lastError().text() << sql;

        sql = QStringLiteral("update cargo set colortype='%1' where hpcode='%2';")
                .arg(typeName).arg(cargo);
        db.exec(sql);
        if ( db.lastError().isValid() ) qDebug() << db.lastError().text() << sql;
    }
}

void BsRegWin::clickAmGeo()
{
    if ( mpGrid->currentRow() < 0 ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_please_pick_a_row"));
        return;
    }

    int geoIdx = mpGrid->getColumnIndexByFieldName("amgeo");
    int addrIdx = mpGrid->getColumnIndexByFieldName("regaddr");
    int teleIdx = mpGrid->getColumnIndexByFieldName("regtele");
    int respIdx = mpGrid->getColumnIndexByFieldName("regman");
    Q_ASSERT(geoIdx >= 0 && addrIdx >= 0 && teleIdx >= 0 && respIdx >= 0);

    QString backer = dogNetName;
    QString shop = mpGrid->item(mpGrid->currentRow(), 0)->text();
    QString addr = mpGrid->item(mpGrid->currentRow(), addrIdx)->text();
    QString tele = mpGrid->item(mpGrid->currentRow(), teleIdx)->text();
    QString resp = mpGrid->item(mpGrid->currentRow(), respIdx)->text();

    QString loc = mpGrid->item(mpGrid->currentRow(), geoIdx)->text();
    if ( !loc.isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("%1已经登记定位，无法更改。您可使用工具箱中"
                                                                 "删除坐标功能，先删除该店坐标。").arg(shop));
        return;
    }

    if ( addr.isEmpty() || tele.isEmpty() || resp.isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("提交坐标上线必须提供完整的地址、电话、店长信息。"));
        return;
    }

    BsShopLocDlg dlg(shop, this);
    if ( dlg.exec() != QDialog::Accepted ) {
        return;
    }

    //确保末位不变
    qint64 x = 1000000 * (dlg.mpLng->text().toDouble() + 0.00000001);
    qint64 y = 1000000 * (dlg.mpLat->text().toDouble() + 0.00000001);

    QStringList params;
    params << QStringLiteral("backer\x01%1").arg(backer)
           << QStringLiteral("shop\x01%1").arg(shop)
           << QStringLiteral("x\x01%1").arg(x)
           << QStringLiteral("y\x01%1").arg(y)
           << QStringLiteral("addr\x01%1").arg(addr)
           << QStringLiteral("tele\x01%1").arg(tele)
           << QStringLiteral("resp\x01%1").arg(resp);

    QNetworkReply* reply = httpRequest(QStringLiteral("/pc/putshop"), params.join(QChar(2)), QString(),
                                       httpUserName, httpPassHash);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();
    if ( reply->error() ) {
        int sttCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QMessageBox::information(this, QString(), QStringLiteral("网络错误%1，状态码%2").arg(reply->error()).arg(sttCode));
        return;
    }
    QString response = QString::fromUtf8(reply->readAll());
    if ( response == QStringLiteral("OK") ) {
        //经度在前，纬度在后，中间逗号。这个约定已有多处使用，不要随便动。
        QString sx = QString::number(dlg.mpLng->text().toDouble() + 0.00000001, 'f', 6);
        QString sy = QString::number(dlg.mpLat->text().toDouble() + 0.00000001, 'f', 6);
        QString geoText = QStringLiteral("%1,%2").arg(sx).arg(sy);
        int geoIdx = mpGrid->getColumnIndexByFieldName("amgeo");
        mpGrid->item(mpGrid->currentRow(), geoIdx)->setText(geoText);
        QString sql = QStringLiteral("update shop set amgeo='%1' where kname='%2';").arg(geoText).arg(shop);
        QSqlQuery qry;
        qry.exec(sql);
        if ( qry.lastError().isValid() ) {
            qDebug() << sql << "\n" << qry.lastError().text();
            return;
        }
        QMessageBox::information(this, QString(), QStringLiteral("定坐标成功。你可到登记货品窗口给相关货品“打标签”，\n"
                                                                 "打好标签后，系统会自动监控该店库存变动，以确保平台\n"
                                                                 "用户能搜索到。"));
    }
    else if ( response == QStringLiteral("EX") ) {
        QMessageBox::information(this, QString(), QStringLiteral("该坐标已有门店登记！"));
    }
    else {
        QMessageBox::information(this, QString(), QStringLiteral("定坐标失败，请稍后重试。"));
    }
}

void BsRegWin::clickToolAmUnGeo()
{
    if ( mpGrid->currentRow() < 0 ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_please_pick_a_row"));
        return;
    }

    int geoIdx = mpGrid->getColumnIndexByFieldName("amgeo");
    Q_ASSERT(geoIdx >= 0);
    qint64 x, y;
    QString amgeo = mpGrid->item(mpGrid->currentRow(), geoIdx)->text();
    if ( amgeo.isEmpty() ) {
        return;
    }
    else {
        QStringList pairs = amgeo.split(QChar(','));
        Q_ASSERT( pairs.length() == 2 );
        double fx = QString(pairs.at(0)).toDouble();
        double fy = QString(pairs.at(1)).toDouble();
        x = 1000000 * (fx + 0.00000001);
        y = 1000000 * (fy + 0.00000001);
    }

    QString backer = dogNetName;
    QString shop = mpGrid->item(mpGrid->currentRow(), 0)->text();
    QString tipMsg = QStringLiteral("删除门店坐标后该店所有货品将不再被“爱美无价”用户搜索到。");
    QString askMsg = QStringLiteral("确定要删除“%1”坐标定位吗？").arg(shop);
    if ( ! confirmDialog(this, tipMsg, askMsg,
                         mapMsg.value("btn_ok"),
                         mapMsg.value("btn_cancel"),
                         QMessageBox::Warning) ) {
        return;
    }

    QStringList params;
    params << QStringLiteral("backer\x01%1").arg(backer)
           << QStringLiteral("shop\x01%1").arg(shop)
           << QStringLiteral("x\x01%1").arg(x)
           << QStringLiteral("y\x01%1").arg(y);

    QNetworkReply* reply = httpRequest(QStringLiteral("/pc/delshop"), params.join(QChar(2)), QString(),
                                       httpUserName, httpPassHash);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();
    if ( reply->error() ) {
        int sttCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QMessageBox::information(this, QString(), QStringLiteral("网络错误%1，状态码%2").arg(reply->error()).arg(sttCode));
        return;
    }
    QString response = QString::fromUtf8(reply->readAll());
    if ( response == QStringLiteral("OK") ) {
        int geoIdx = mpGrid->getColumnIndexByFieldName("amgeo");
        mpGrid->item(mpGrid->currentRow(), geoIdx)->setText(QString());
        QString sql = QStringLiteral("update shop set amgeo='' where kname='%1';").arg(shop);
        QSqlQuery qry;
        qry.exec(sql);
        if ( qry.lastError().isValid() ) {
            qDebug() << sql << "\n" << qry.lastError().text();
            return;
        }
    }
    else {
        QMessageBox::information(this, QString(), QStringLiteral("删除门店坐标失败，请稍后重试。"));
    }
}

void BsRegWin::clickAmTag()
{
    if ( mpGrid->currentRow() < 0 ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_please_pick_a_row"));
        return;
    }

    int tagIdx = mpGrid->getColumnIndexByFieldName("amtag");
    Q_ASSERT(tagIdx >= 0);

    //基本信息
    QString hpcode = mpGrid->item(mpGrid->currentRow(), 0)->text();
    QString taged = mpGrid->item(mpGrid->currentRow(), tagIdx)->text();

    //检查获取可用标签
    if ( ! checkGetTags() ) {
        return;  //无需提示
    }

    //用户选择
    BsTagSelectDlg dlg(this, hpcode, mHttpTags, taged);
    if ( dlg.exec() != QDialog::Accepted ) {
        return;
    }
    QString cargoTag = dlg.mpResultLabel->text();
    QString uploadFile = dlg.mResultImage;

    //处理小图
    QImage thumbImg = QImage(uploadFile).scaled(QSize(300, 390), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QByteArray thumbData;
    QBuffer buffer(&thumbData);
    buffer.open(QIODevice::WriteOnly);
    thumbImg.save(&buffer, "jpg");
    buffer.close();

    //库存变动系统自动上下架,非客户手动。此处仅需上传图片
    QStringList params;
    params << QStringLiteral("backer\x01%1").arg(dogNetName)
           << QStringLiteral("hpcode\x01%1").arg(hpcode)
           << QStringLiteral("thumb\x01%1").arg(QString(thumbData.toBase64()));

    QNetworkReply* reply = httpRequest(QStringLiteral("/pc/putimage"), params.join(QChar(2)), uploadFile,
                                       httpUserName, httpPassHash);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();
    if ( reply->error() ) {
        int sttCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QMessageBox::information(this, QString(), QStringLiteral("网络错误%1，状态码%2").arg(reply->error()).arg(sttCode));
        return;
    }
    QString response = QString::fromUtf8(reply->readAll());
    if ( response == QStringLiteral("OK") ) {
        //库存变动系统自动上下架,非客户手动。此处仅作本地标记，但必须，用于库存同步。
        int tagIdx = mpGrid->getColumnIndexByFieldName("amtag");
        mpGrid->item(mpGrid->currentRow(), tagIdx)->setText(cargoTag);
        QString sql = QStringLiteral("update cargo set amtag='%1' where hpcode='%2';").arg(cargoTag).arg(hpcode);
        QSqlQuery qry;
        qry.exec(sql);
        if ( qry.lastError().isValid() ) {
            qDebug() << sql << "\n" << qry.lastError().text();
            return;
        }
        QMessageBox::information(this, QString(), QStringLiteral("打标签成功。请多关注和推广本平台。"));
    }
    else {
        QMessageBox::information(this, QString(), QStringLiteral("打标签失败，请稍候重试。"));
    }
}

void BsRegWin::clickToolAmUnTag()
{
    if ( mpGrid->currentRow() < 0 ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_please_pick_a_row"));
        return;
    }

    QString hpcode = mpGrid->item(mpGrid->currentRow(), 0)->text();
    int tagIdx = mpGrid->getColumnIndexByFieldName("amtag");
    Q_ASSERT(tagIdx >= 0);

    QString tipMsg = QStringLiteral("删除标签后该货品将不再被“爱美无价”用户搜索到。");
    QString askMsg = QStringLiteral("确定要删除“%1”平台标签吗？").arg(hpcode);
    if ( ! confirmDialog(this, tipMsg, askMsg,
                         mapMsg.value("btn_ok"),
                         mapMsg.value("btn_cancel"),
                         QMessageBox::Warning) ) {
        return;
    }

    //库存变动系统自动上下架,非客户手动。此处仅作本地标记，但必须，用于库存同步。
    QString sql = QStringLiteral("update cargo set amtag='' where hpcode='%1';").arg(hpcode);
    QSqlQuery qry;
    qry.exec(sql);
    if ( qry.lastError().isValid() ) {
        qDebug() << sql << "\n" << qry.lastError().text();
        QMessageBox::information(this, QString(), QStringLiteral("删除平台标签失败。"));
        return;
    }
    mpGrid->item(mpGrid->currentRow(), tagIdx)->setText(QString());
}

bool BsRegWin::checkGetTags()
{
    if ( mHttpTags.isEmpty() ) {
       QNetworkReply* reply = httpRequest(QStringLiteral("/web/tags"), QString(), QString());
       QEventLoop loop;
       connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
       loop.exec();

       reply->deleteLater();
       if ( reply->error() ) {
           int sttCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
           QMessageBox::information(this, QString(), QStringLiteral("网络错误%1，状态码%2").arg(reply->error()).arg(sttCode));
           return false;
       }
       QString response = QString::fromUtf8(reply->readAll());
       mHttpTags = response.split(QChar(9));
       if ( mHttpTags.length() < 3 ) {
           return false;
       }
       mHttpTags.sort();
    }

    return true;
}


// BsAbstractSheetWin
BsAbstractSheetWin::BsAbstractSheetWin(QWidget *parent, const QString &name, const QStringList &fields)
    : BsAbstractFormWin(parent, name, fields, bswtSheet), mAllowPriceMoney(true), mCurrentSheetId(0)
{
    //用于权限判断
    mRightWinName = name;
    //Q_ASSERT(lstSheetWinTableNames.indexOf(mRightWinName) >= 0);
    //由于szd也是用的BsAbstractSheetWin但没有权限细设，所以不能用此断言。
    //此断言主要怕vi类命名太多没搞一致，这儿用不用没关系。

    //具体化mFields（包括列名、帮助提示、特性、……）
    BsField *fldShop = getFieldByName("shop");
    fldShop->mFldCnName = mapMsg.value(QStringLiteral("fldcname_%1_shop").arg(name));

    BsField *fldTrader = getFieldByName("trader");
    fldTrader->mFldCnName = mapMsg.value(QStringLiteral("fldcname_%1_trader").arg(name));

    BsField *fldActPay = getFieldByName("actpay");
    if ( fldActPay )
        fldActPay->mFldCnName = mapMsg.value(QStringLiteral("fldcname_%1_actpay").arg(name));

    //窗口
    mGuideObjectTip = mapMsg.value(QStringLiteral("win_%1").arg(name)).split(QChar(9)).at(1);
    mGuideClassTip = mapMsg.value("win_<sheet>").split(QChar(9)).at(1);
    mpGuide->setText(mGuideClassTip);

    //主按钮
    mpAcMainOpen = mpToolBar->addAction(QIcon(":/icon/find.png"), mapMsg.value("btn_sheet_open").split(QChar(9)).at(0),
                                    this, SLOT(clickOpenFind()));

    mpAcMainCheck = mpToolBar->addAction(QIcon(":/icon/check.png"), mapMsg.value("btn_sheet_check").split(QChar(9)).at(0),
                                     this, SLOT(clickCheck()));

    mpAcMainPrint = mpToolBar->addAction(QIcon(":/icon/print.png"), mapMsg.value("btn_print").split(QChar(9)).at(0),
                                     this, SLOT(clickPrint()));

    mpAcMainPrint->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));

    mpToolBar->insertAction(mpAcMainNew, mpAcMainOpen);
    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainCheck);
    mpToolBar->insertAction(mpAcToolSeprator, mpAcMainPrint);

    mpAcMainOpen->setProperty(BSACFLAGS, bsacfClean);
    mpAcMainCheck->setProperty(BSACFLAGS, bsacfClean | bsacfPlusId | bsacfNotChk);
    mpAcMainPrint->setProperty(BSACFLAGS, bsacfClean | bsacfPlusId);

    mpAcMainOpen->setProperty(BSACRIGHT, true);
    mpAcMainNew->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsNew));
    mpAcMainEdit->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsUpd));
    mpAcMainDel->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsDel));
    mpAcMainCheck->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsCheck));
    mpAcMainPrint->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsPrint));
    mpAcOptHideDropRow->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsUpd));
    mpAcToolImport->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsNew) || canDo(mRightWinName, bsrsUpd));
    mpAcToolExport->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsExport));

    //工具箱
    mpToolPrintSetting = mpMenuToolCase->addAction(mapMsg.value("tool_print_setting"),
                                                   this, SLOT(clickToolPrintSetting()));
    mpToolPrintSetting->setProperty(BSACFLAGS,  0);
    mpToolPrintSetting->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsPrint));
    mpMenuToolCase->insertAction(mpAcToolExport, mpToolPrintSetting);

    mpToolUnCheck = mpMenuToolCase->addAction(mapMsg.value("tool_sheet_uncheck"),
                                                   this, SLOT(clickToolUnCheck()));
    mpToolUnCheck->setProperty(BSACFLAGS, bsacfClean | bsacfPlusId | bsacfChecked);
    mpToolUnCheck->setProperty(BSACRIGHT, loginAsBoss);
    mpMenuToolCase->insertAction(mpAcToolExport, mpToolUnCheck);

    mpToolAdjustCurrentRowPosition = mpMenuToolCase->addAction(mapMsg.value("tool_adjust_current_row_position"),
                                                   this, SLOT(clickToolAdjustCurrentRowPosition()));
    mpToolAdjustCurrentRowPosition->setProperty(BSACFLAGS,  bsacfDirty);
    mpToolAdjustCurrentRowPosition->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsUpd) || canDo(mRightWinName, bsrsNew));

    //开关盒
    mpAcOptSortBeforePrint = mpMenuOptionBox->addAction(QIcon(), mapMsg.value("opt_sort_befor_print"));
    mpAcOptSortBeforePrint->setProperty("optname", "opt_sort_befor_print");
    mpAcOptSortBeforePrint->setProperty(BSACFLAGS, 0);
    mpAcOptSortBeforePrint->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsPrint));

    mpAcOptHideNoQtySizerColWhenOpen = mpMenuOptionBox->addAction(QIcon(), mapMsg.value("opt_hide_noqty_sizercol_when_open"));
    mpAcOptHideNoQtySizerColWhenOpen->setProperty("optname", "opt_hide_noqty_sizercol_when_open");
    mpAcOptHideNoQtySizerColWhenOpen->setProperty(BSACFLAGS, 0);
    mpAcOptHideNoQtySizerColWhenOpen->setProperty(BSACRIGHT, true);
    mpAcOptHideNoQtySizerColWhenOpen->setVisible(false);  //后代BsSheetCargoWin构造函数中才显示

    mpAcOptHideNoQtySizerColWhenPrint = mpMenuOptionBox->addAction(QIcon(), mapMsg.value("opt_hide_noqty_sizercol_when_print"));
    mpAcOptHideNoQtySizerColWhenPrint->setProperty("optname", "opt_hide_noqty_sizercol_when_print");
    mpAcOptHideNoQtySizerColWhenPrint->setProperty(BSACFLAGS, 0);
    mpAcOptHideNoQtySizerColWhenPrint->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsPrint));
    mpAcOptHideNoQtySizerColWhenPrint->setVisible(false); //后代BsSheetCargoWin构造函数中才显示

    //具体化help_tip
    mpAcMainOpen->setStatusTip(mapMsg.value("btn_sheet_open").split(QChar(9)).at(1));
    mpAcMainCheck->setStatusTip(mapMsg.value("btn_sheet_check").split(QChar(9)).at(1));
    mpAcMainPrint->setStatusTip(mapMsg.value("btn_print").split(QChar(9)).at(1));

    mpAcMainNew->setText(mapMsg.value("btn_sheet_new").split(QChar(9)).at(0));
    mpAcMainEdit->setText(mapMsg.value("btn_sheet_edit").split(QChar(9)).at(0));
    mpAcMainDel->setText(mapMsg.value("btn_sheet_del").split(QChar(9)).at(0));
    mpAcMainSave->setText(mapMsg.value("btn_sheet_save").split(QChar(9)).at(0));
    mpAcMainCancel->setText(mapMsg.value("btn_sheet_cancel").split(QChar(9)).at(0));

    mpAcMainNew->setStatusTip(mapMsg.value("btn_sheet_new").split(QChar(9)).at(1));
    mpAcMainEdit->setStatusTip(mapMsg.value("btn_sheet_edit").split(QChar(9)).at(1));
    mpAcMainDel->setStatusTip(mapMsg.value("btn_sheet_del").split(QChar(9)).at(1));
    mpAcMainSave->setStatusTip(mapMsg.value("btn_sheet_save").split(QChar(9)).at(1));
    mpAcMainCancel->setStatusTip(mapMsg.value("btn_sheet_cancel").split(QChar(9)).at(1));

    //打开查询内容===========================BEGIN
    mpBtnOpenBack  = new QToolButton(this);
    mpBtnOpenBack->setText(mapMsg.value("word_backward"));
    mpBtnOpenBack->setIcon(QIcon(":/icon/cancel.png"));
    mpBtnOpenBack->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpBtnOpenBack->setIconSize(QSize(24, 24));

    QMenu *mnPeriod = new QMenu(this);
    mnPeriod->addAction(mapMsg.value("menu_today"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_yesterday"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addSeparator();
    mnPeriod->addAction(mapMsg.value("menu_this_week"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_last_week"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addSeparator();
    mnPeriod->addAction(mapMsg.value("menu_this_month"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_last_month"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addSeparator();
    mnPeriod->addAction(mapMsg.value("menu_this_year"), this, SLOT(clickQuickPeriod()));
    mnPeriod->addAction(mapMsg.value("menu_last_year"), this, SLOT(clickQuickPeriod()));
    mnPeriod->setStyleSheet("QMenu::item {padding-left:16px;}");
    mpBtnPeriod  = new QToolButton(this);
    mpBtnPeriod->setIcon(QIcon(":/icon/calendar.png"));
    mpBtnPeriod->setText(mapMsg.value("i_period_quick_select"));
    mpBtnPeriod->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mpBtnPeriod->setIconSize(QSize(12, 12));
    mpBtnPeriod->setMenu(mnPeriod);
    mpBtnPeriod->setPopupMode(QToolButton::InstantPopup);

    mpConCheck = new BsConCheck(this, true);
    mpConDateB  = new BsFldBox(this, getFieldByName("dated"), nullptr, true);
    mpConDateE  = new BsFldBox(this, getFieldByName("dated"), nullptr, true);
    mpConShop   = new BsFldBox(this, fldShop, dsShop, true);
    mpConStype  = new BsFldBox(this, getFieldByName("stype"), mpDsStype, true);
    mpConStaff  = new BsFldBox(this, getFieldByName("staff"), mpDsStaff, true);
    mpConTrader = new BsFldBox(this, fldTrader, mpDsTrader, true);

    mpConDateB->mpEditor->setMyPlaceText(mapMsg.value("i_date_begin_text"));
    mpConDateE->mpEditor->setMyPlaceText(mapMsg.value("i_date_end_text"));
    mpConShop->mpEditor->setMyPlaceText(fldShop->mFldCnName);
    mpConStype->mpEditor->setMyPlaceText(getFieldByName("stype")->mFldCnName);
    mpConStaff->mpEditor->setMyPlaceText(getFieldByName("staff")->mFldCnName);
    mpConTrader->mpEditor->setMyPlaceText(fldTrader->mFldCnName);

    mpConDateB->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConDateE->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConShop->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConStype->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConStaff->mpEditor->setMyPlaceColor(QColor(Qt::gray));
    mpConTrader->mpEditor->setMyPlaceColor(QColor(Qt::gray));

    mpConDateB->mpLabel->setText(mapMsg.value("i_date_begin_label"));
    mpConDateE->mpLabel->setText(mapMsg.value("i_date_end_label"));

    mpBtnQuery  = new QToolButton(this);
    mpBtnQuery->setText(mapMsg.value("word_query"));
    mpBtnQuery->setIcon(QIcon(":/icon/zoomlens.png"));
    mpBtnQuery->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpBtnQuery->setIconSize(QSize(24, 24));

    mpPnlOpenCon = new QWidget(this);
    QGridLayout *layCon = new QGridLayout(mpPnlOpenCon);
    layCon->setContentsMargins(15, 0, 15, 0);
    layCon->setHorizontalSpacing(20);
    layCon->addWidget(mpBtnOpenBack,    0, 0, 2, 1);
    layCon->addWidget(mpBtnPeriod,      0, 1);
    layCon->addWidget(mpConDateB,       0, 2);
    layCon->addWidget(mpConDateE,       0, 3);
    layCon->addWidget(mpConShop,        0, 4);
    layCon->addWidget(mpBtnQuery,       0, 5, 2, 1);
    layCon->addWidget(mpConCheck,      1, 1, Qt::AlignCenter);
    layCon->addWidget(mpConStype,       1, 2);
    layCon->addWidget(mpConStaff,       1, 3);
    layCon->addWidget(mpConTrader,      1, 4);
    layCon->setColumnStretch(0, 0);
    layCon->setColumnStretch(1, 0);
    layCon->setColumnStretch(2, 2);
    layCon->setColumnStretch(3, 2);
    layCon->setColumnStretch(4, 3);
    layCon->setColumnStretch(5, 0);
    int size = 2 * mpConShop->sizeHint().height() + layCon->verticalSpacing();
    mpBtnOpenBack->setFixedSize(size, size);
    mpBtnQuery->setFixedSize(size, size);

    mpFindGrid = new BsQueryGrid(this);
    mpFindGrid->setStatusTip(mapMsg.value("i_sheet_query_open_tip"));
    connect(mpFindGrid, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(doubleClickOpenSheet(QTableWidgetItem*)));

    mpPnlOpener = new QWidget(this);
    mpPnlOpener->setStyleSheet(QLatin1String(".QWidget{background-color:#eeeeeb;}"));
    QVBoxLayout *layOpener = new QVBoxLayout(mpPnlOpener);
    layOpener->setContentsMargins(3, 10, 3, 0);
    layOpener->addWidget(mpPnlOpenCon);
    layOpener->addWidget(mpFindGrid, 1);

    mpLayBody = new QVBoxLayout(mpBody);
    mpLayBody->setContentsMargins(1, 0, 1, 8);
    mpLayBody->setSpacing(0);
    mpLayBody->addWidget(mpPnlOpener, 1);

    mpConDateB->mpEditor->setDataValue(QDateTime(QDate::currentDate()).toMSecsSinceEpoch() / 1000);
    mpConDateE->mpEditor->setDataValue(QDateTime(QDate::currentDate()).toMSecsSinceEpoch() / 1000);
    mpPnlOpener->hide();

    //绑店条件
    if ( ! loginShop.isEmpty() ) {
        mpConShop->mpEditor->setDataValue(loginShop);
        mpConShop->mpEditor->setEnabled(false);
    }

    connect(mpBtnOpenBack, SIGNAL(clicked(bool)), this, SLOT(clickCancelOpenPage()));
    connect(mpBtnQuery, SIGNAL(clicked(bool)), this, SLOT(clickExecuteQuery()));
    //打开查询内容===========================END

    //表头
    QString sheetName = mapMsg.value(QStringLiteral("win_%1").arg(mMainTable)).split(QChar(9)).at(0);
    mpSheetName = new QLabel(sheetName, this);
    mpSheetName->setStyleSheet("font-size:12pt;");
    mpSheetId = new BsSheetIdLabel(this, name);
    mpCheckMark  = new BsSheetCheckLabel(this);

    mpLayTitleBox = new QHBoxLayout();
    mpLayTitleBox->setContentsMargins(0, 0, 0, 0);
    mpLayTitleBox->setSpacing(100);
    mpLayTitleBox->addWidget(mpSheetName);
    mpLayTitleBox->addWidget(mpSheetId);
    mpLayTitleBox->addWidget(mpCheckMark);
    mpLayTitleBox->addStretch();

    mpDated  = new BsFldBox(this, getFieldByName("dated"), nullptr);
    mpStype  = new BsFldBox(this, getFieldByName("stype"), mpDsStype);
    mpShop  = new BsFldBox(this, fldShop, dsShop);
    mpProof  = new BsFldBox(this, getFieldByName("proof"), nullptr);
    mpStaff  = new BsFldBox(this, getFieldByName("staff"), mpDsStaff);
    mpTrader  = new BsFldBox(this, fldTrader, mpDsTrader);
    mpRemark  = new BsFldBox(this, getFieldByName("remark"), nullptr);

    mpLayEditBox = new QGridLayout();
    mpLayEditBox->setContentsMargins(0, 0, 0, 0);
    mpLayEditBox->setHorizontalSpacing(30);
    mpLayEditBox->addWidget(mpDated,  0, 0);
    mpLayEditBox->addWidget(mpStype,  0, 1);
    mpLayEditBox->addWidget(mpShop,   0, 2);
    mpLayEditBox->addWidget(mpProof,  1, 0);
    mpLayEditBox->addWidget(mpStaff,  1, 1);
    mpLayEditBox->addWidget(mpTrader, 1, 2);
    mpLayEditBox->addWidget(mpRemark, 2, 0, 1, 3);
    mpLayEditBox->setColumnStretch(0, 2);
    mpLayEditBox->setColumnStretch(1, 2);
    mpLayEditBox->setColumnStretch(2, 3);

    mpPnlHeader = new QWidget(this);
    QVBoxLayout *layHeader = new QVBoxLayout(mpPnlHeader);
    layHeader->setContentsMargins(10, 5, 10, 5);
    layHeader->addLayout(mpLayTitleBox);
    layHeader->addLayout(mpLayEditBox);

    //表尾
    if ( fldActPay && getFieldByName("actowe") ) {
        mpActPay = new BsFldBox(this, fldActPay, nullptr);
        mpActOwe = new BsFldBox(this, getFieldByName("actowe"), nullptr);

        mpPnlPayOwe = new QWidget(this);
        QHBoxLayout *layFooter = new QHBoxLayout(mpPnlPayOwe);
        layFooter->setContentsMargins(10, 3, 10, 0);
        layFooter->setSpacing(30);
        layFooter->addStretch(1);
        layFooter->addWidget(mpActPay, 1);
        layFooter->addWidget(mpActOwe, 1);
    }
    else {
        mpPnlPayOwe = nullptr;
        mpActPay = nullptr;
        mpPnlPayOwe = nullptr;
    }

    //状态栏增加
    mpSttLblChecker = new QLabel(mapMsg.value("fld_checker").split(QChar(9)).at(0), this);
    mpSttLblChecker->setStyleSheet("color:#666; font-weight:900;");

    mpSttValChecker = new QLabel(this);
    mpSttValChecker->setMinimumWidth(50);

    mpSttLblChkTime = new QLabel(mapMsg.value("i_status_check_time"), this);
    mpSttLblChkTime->setStyleSheet("color:#666; font-weight:900;");

    mpSttValChkTime = new QLabel(this);

    mpStatusBar->addWidget(mpSttLblChecker);
    mpStatusBar->addWidget(mpSttValChecker);
    mpStatusBar->addWidget(mpSttLblChkTime);
    mpStatusBar->addWidget(mpSttValChkTime);

    //布局
    mpLayBody->addWidget(mpPnlHeader);
    if ( mpPnlPayOwe ) {
        mpLayBody->addWidget(mpPnlPayOwe);
    }

    //打开数据源
    dsShop->reload();

    //绑店限定
    if ( ! loginShop.isEmpty() ) {
        mpShop->mpEditor->setDataValue(loginShop);
        mpShop->mpEditor->setEnabled(false);
    }

    //打印控制件
    mpPrinter = new LxPrinter(this);
}

BsAbstractSheetWin::~BsAbstractSheetWin()
{
    delete mpPrinter;
}

void BsAbstractSheetWin::setEditable(const bool editt)
{
    BsAbstractFormWin::setEditable(editt);

    for ( int i = 0, iLen = mpPnlHeader->children().count(); i < iLen; ++i ) {
        BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlHeader->children().at(i));
        if ( edt ) {
            edt->mpEditor->setReadOnly(!editt);
        }
    }
    if ( mpPnlPayOwe ) {
        for ( int i = 0, iLen = mpPnlPayOwe->children().count(); i < iLen; ++i ) {
            BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlPayOwe->children().at(i));
            if ( edt ) {
                edt->mpEditor->setReadOnly(!editt);
            }
        }
    }
    mpSheetGrid->setEditable(editt);

    //绑店检查
    if ( ! loginShop.isEmpty() ) {
        mpShop->mpEditor->setDataValue(loginShop);
        mpShop->mpEditor->setEnabled(false);
    }

    //助手
    updateTabber(editt);
}

void BsAbstractSheetWin::openSheet(const int sheetId)   //注意与doSave()的一致性
{
    //表头
    if ( sheetId <= 0 ) {
        mpSheetId->setDataValue(sheetId);
        mpCheckMark->setDataValue(false, sheetId);
        mpDated->mpEditor->setDataValue( (sheetId < 0) ? QDateTime::currentMSecsSinceEpoch() / 1000 : 0 );
        mpStype->mpEditor->setDataValue(QString());
        mpStaff->mpEditor->setDataValue(QString());
        mpProof->mpEditor->setDataValue(QString());
        mpShop->mpEditor->setDataValue(QString());
        mpTrader->mpEditor->setDataValue(QString());
        mpRemark->mpEditor->setDataValue(QString());
        if ( mpActPay && mpActOwe ) {
            mpActPay->mpEditor->setDataValue(0);
            mpActOwe->mpEditor->setDataValue(0);
        }
        mpSttValKey->clear();
        mpSttValUpman->clear();
        mpSttValUptime->clear();
        mpSttValChecker->clear();
        mpSttValChkTime->clear();

        if ( mpSheetCargoGrid ) {
            mpSheetCargoGrid->setTraderDiscount(1.0);
            mpSheetCargoGrid->setTraderName(QString());
        }
    }
    else {
        QStringList sels;
        for ( int i = 0, iLen = mFields.length(); i < iLen; ++i ) {
            BsField *fld = mFields.at(i);
            uint flags = fld->mFlags;
            if ( ((flags & bsffHead) == bsffHead || (flags & bsffHideSys) == bsffHideSys) && (flags & bsffGrid) != bsffGrid ) {
                sels << fld->mFldName;
            }
        }
        QString sql = QStringLiteral("SELECT %1 FROM %2 WHERE sheetid=%3;")
                .arg(sels.join(QChar(44))).arg(mMainTable).arg(sheetId);
        QSqlQuery qry;
        qry.setForwardOnly(true);
        qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
        qry.exec(sql);
        if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << "\n" << sql;
        QSqlRecord rec = qry.record();
        if ( qry.next() ) {
            int idxSheetId = rec.indexOf(QStringLiteral("sheetid"));
            Q_ASSERT(idxSheetId >= 0);
            mpSheetId->setDataValue(qry.value(idxSheetId));

            int idxChkTime = rec.indexOf(QStringLiteral("chktime"));
            Q_ASSERT(idxChkTime >= 0);
            mpCheckMark->setDataValue(qry.value(idxChkTime), sheetId);

            for ( int i = 0, iLen = mpPnlHeader->children().count(); i < iLen; ++i ) {
                BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlHeader->children().at(i));
                if ( edt ) {
                    int idx = rec.indexOf(edt->getFieldName());
                    Q_ASSERT(idx >= 0);
                    edt->mpEditor->setDataValue(qry.value(idx));
                    edt->setProperty(BSVALUE_OLD, edt->mpEditor->text());
                }
            }

            if ( mpPnlPayOwe ) {
                for ( int i = 0, iLen = mpPnlPayOwe->children().count(); i < iLen; ++i ) {
                    BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlPayOwe->children().at(i));
                    if ( edt ) {
                        int idx = rec.indexOf(edt->getFieldName());
                        Q_ASSERT(idx >= 0);
                        edt->mpEditor->setDataValue(qry.value(idx));
                        edt->setProperty(BSVALUE_OLD, edt->mpEditor->text());
                    }
                }
            }

            mpSttValKey->setText(QStringLiteral("%1%2").arg(mMainTable.toUpper())
                                 .arg(qry.value(idxSheetId).toInt(), 8, 10, QLatin1Char('0')));

            int idxUpman = rec.indexOf(QStringLiteral("upman"));
            Q_ASSERT(idxUpman >= 0);
            mpSttValUpman->setText(qry.value(idxUpman).toString());

            int idxUptime = rec.indexOf(QStringLiteral("uptime"));
            Q_ASSERT(idxUptime >= 0);
            qint64 uptime = qry.value(idxUptime).toLongLong();
            QDateTime dtUptime = QDateTime::fromMSecsSinceEpoch(uptime * 1000);
            QString uptimeStr = (uptime > 0) ? dtUptime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) : QString();
            mpSttValUptime->setText(uptimeStr);

            int idxChecker = rec.indexOf(QStringLiteral("checker"));
            Q_ASSERT(idxChecker >= 0);
            mpSttValChecker->setText(qry.value(idxChecker).toString());

            qint64 chktime = qry.value(idxChkTime).toLongLong();
            QDateTime dtChkTime = QDateTime::fromMSecsSinceEpoch(chktime * 1000);
            QString chktimeStr = (chktime > 0) ? dtChkTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) : QString();
            mpSttValChkTime->setText(chktimeStr);
        }
        qry.finish();
    }

    //表格
    mpSheetGrid->openBySheetId(sheetId);

    //记录当前单据号
    mCurrentSheetId = sheetId;

    //状态
    setEditable(sheetId < 0);

    //显示
    mpLayBody->setContentsMargins(1, 0, 1, 8);
    mpPnlOpener->hide();
    mpToolBar->show();
    mpPnlHeader->show();
    mpSheetGrid->show();
    mpStatusBar->show();
    if ( mpPnlPayOwe ) {
        mpPnlPayOwe->setVisible(mAllowPriceMoney && mMainTable != QStringLiteral("syd"));
    }

    //选项
    if ( mpAcOptHideNoQtySizerColWhenOpen->isVisible() && mpAcOptHideNoQtySizerColWhenOpen->isChecked())
        mpSheetCargoGrid->autoHideNoQtySizerCol();
}

QString BsAbstractSheetWin::getPrintValue(const QString &valueName) const
{
    if ( valueName == QStringLiteral("sheetid") )
        return mpSheetId->getDisplayText();

    else if ( valueName == QStringLiteral("dated") )
        return mpDated->mpEditor->text();

    else if ( valueName == QStringLiteral("proof") )
        return mpProof->mpEditor->text();

    else if ( valueName == QStringLiteral("stype") )
        return mpStype->mpEditor->text();

    else if ( valueName == QStringLiteral("staff") )
        return mpStaff->mpEditor->text();

    else if ( valueName == QStringLiteral("shop") )
        return mpShop->mpEditor->text();

    else if ( valueName == QStringLiteral("trader") )
        return mpTrader->mpEditor->text();

    else if ( valueName == QStringLiteral("remark") )
        return mpRemark->mpEditor->text();

    else if ( valueName == QStringLiteral("actpay") )
        return mpActPay->mpEditor->text();

    else if ( valueName == QStringLiteral("actowe") )
        return mpActOwe->mpEditor->text();

    else if ( valueName == QStringLiteral("upman") )
        return mpSttValUpman->text();

    else if ( valueName == QStringLiteral("uptime") )
        return mpSttValUptime->text();

    else if ( valueName == QStringLiteral("sumqty") )
        return mpGrid->getFooterValueByField(QStringLiteral("qty"));

    else if ( valueName == QStringLiteral("summoney") )
        return mpGrid->getFooterValueByField(QStringLiteral("actmoney"));

    else if ( valueName == QStringLiteral("sumdis") )
        return mpGrid->getFooterValueByField(QStringLiteral("dismoney"));

    else if ( valueName == QStringLiteral("sumowe") )
    {
        QString chkCon = (mpCheckMark->getDataCheckedValue()) ? QStringLiteral(" and chktime<>0") : QString();
        QString sql = QStringLiteral("select sum(actowe) from %1 where trader='%2'%3;")
                .arg(mMainTable).arg(mpTrader->mpEditor->getDataValue()).arg(chkCon);
        QSqlQuery qry;
        qry.exec(sql);
        if ( qry.next() ) {
            QString oweValue = bsNumForRead(qry.value(0).toLongLong(), mapOption.value("dots_of_money").toInt());
            if ( !mpCheckMark->getDataCheckedValue() ) {
                oweValue += QStringLiteral("未审");
            }
            return oweValue;
        }
        return QString();
    }

    else if ( valueName.indexOf(QChar('.')) > 0 )
    {
        QStringList vpair = valueName.split(QChar('.'));
        Q_ASSERT(vpair.length() == 2);
        QString vtbl = vpair.at(0);

        QString dbtbl;
        if ( vtbl == QStringLiteral("trader") ) {
            if ( mMainTable.startsWith(QStringLiteral("cg")) )
                dbtbl = QStringLiteral("supplier");
            else if ( mMainTable.startsWith(QStringLiteral("pf"))
                      || mMainTable.startsWith(QStringLiteral("ls")) )
                dbtbl = QStringLiteral("customer");
            else
                dbtbl = QStringLiteral("shop");
        }

        QString who = (vtbl == QStringLiteral("trader"))
                ? mpTrader->mpEditor->getDataValue() : mpShop->mpEditor->getDataValue();

        QString sql = QStringLiteral("select %1 from %2 where kname='%3';")
                .arg(vpair.at(1)).arg(dbtbl).arg(who);
        QSqlQuery qry;
        qry.exec(sql);
        if ( qry.next() )
            return qry.value(0).toString();
        return QString();
    }

    return QString();
}

QString BsAbstractSheetWin::getPrintValue(const QString &cargoTableField, const int gridRow)
{
    if ( cargoTableField == QStringLiteral("rowno") )
        return QString::number(gridRow + 1);

    if ( gridRow >= mpGrid->rowCount() )
        return mapMsg.value("word_total");

    QString svalue = dsCargo->getValue(mpGrid->item(gridRow, 0)->text(), cargoTableField);

    QStringList fDefs = mapMsg.value(QStringLiteral("fld_%1").arg(cargoTableField)).split(QChar(9));
    if ( fDefs.length() >= 5 ) {
        uint ff = QString(fDefs.at(3)).toUInt();
        uint dots = QString(fDefs.at(4)).toInt();
        if ( (ff & bsffNumeric) == bsffNumeric && dots < 5 ) {
            double fvalue = svalue.toLongLong() / 10000.0;
            return QString::number(fvalue, 'f', dots);
        }
    }

    return svalue;
}

QString BsAbstractSheetWin::getGridItemValue(const int row, const int col) const
{
    if ( row < mpGrid->rowCount() )
    {
        if (row >= 0 && col >= 0 && col < mpGrid->columnCount() )
        {
            if ( mpGrid->item(row, col) ) {
                return mpGrid->item(row, col)->text();
            }
        }
    }
    else
    {
        if (col > 0 && col < mpGrid->mpFooter->columnCount() )
        {
            if ( mpGrid->mpFooter->item(0, col) ) {
                QString txt = mpGrid->mpFooter->item(0, col)->text();
                if ( txt.contains(QChar('<')) )
                    return QString();
                else
                    return txt;
            }
        }
        else
            return mapMsg.value("word_total");
    }
    return QString();
}

QString BsAbstractSheetWin::getGridItemValue(const int row, const QString &fieldName) const
{
    int col = mpGrid->getColumnIndexByFieldName(fieldName);
    if ( row < mpGrid->rowCount() && col >= 0 )
    {
        if ( mpGrid->item(row, col) ) {
            return mpGrid->item(row, col)->text();
        }
    }
    return QString();
}

BsField *BsAbstractSheetWin::getGridFieldByName(const QString &fieldName) const
{
    return mpGrid->getFieldByName(fieldName);
}

int BsAbstractSheetWin::getGridColByField(const QString &fieldName) const
{
    return mpGrid->getColumnIndexByFieldName(fieldName);
}

int BsAbstractSheetWin::getGridRowCount() const
{
    return mpGrid->rowCount();
}

QStringList BsAbstractSheetWin::getSizerNameListForPrint() const
{
    return mpSheetCargoGrid->getSizerNameListForPrint();
}

QStringList BsAbstractSheetWin::getSizerQtysOfRowForPrint(const int row)
{
    return mpSheetCargoGrid->getSizerQtysOfRowForPrint(row, printZeroSizeQty());
}

bool BsAbstractSheetWin::isLastRow(const int row)
{
    return row == mpGrid->rowCount();
}

void BsAbstractSheetWin::closeEvent(QCloseEvent *e)
{
    mpFindGrid->saveColWidths();
    BsAbstractFormWin::closeEvent(e);
}

void BsAbstractSheetWin::doOpenFind()
{
    mpLayBody->setContentsMargins(0, 0, 0, 0);
    mpPnlOpener->show();
    mpToolBar->hide();
    mpPnlHeader->hide();
    mpGrid->hide();
    mpStatusBar->hide();
    if ( mpPnlPayOwe )
        mpPnlPayOwe->hide();
}

void BsAbstractSheetWin::doCheck()
{
    if ( ! loginAsBoss ) {
        if ( ! confirmDialog(this,
                             mapMsg.value("i_check_sheet_notice"),
                             mapMsg.value("i_check_sheet_confirm"),
                             mapMsg.value("btn_ok"),
                             mapMsg.value("btn_cancel"),
                             QMessageBox::Warning) )
            return;
    }

    qint64 chktime = QDateTime::currentMSecsSinceEpoch() / 1000;
    QString sql = QStringLiteral("update %1 set checker='%2', chktime=%3 where sheetid=%4;")
            .arg(mMainTable).arg(loginer).arg(chktime).arg(mCurrentSheetId);
    QSqlQuery qry;
    qry.exec(sql);
    if ( qry.lastError().isValid() )
        QMessageBox::information(this, QString(), mapMsg.value("i_check_sheet_failed"));
    else {
        mpAcMainEdit->setEnabled(false);
        mpAcMainDel->setEnabled(false);
        mpAcMainCheck->setEnabled(false);
        mpToolUnCheck->setEnabled(true);
        mpCheckMark->setDataValue(chktime, mCurrentSheetId);
        mpSttValChecker->setText(loginer);
        mpSttValChkTime->setText(QDateTime::fromMSecsSinceEpoch(1000 * chktime).toString("yyyy-MM-dd hh:mm:ss"));
        doSyncFindGrid();
        emit shopStockChanged(mpShop->mpEditor->getDataValue(), mMainTable, mCurrentSheetId);
    }
}

void BsAbstractSheetWin::doPrint()
{
    if ( mpAcOptSortBeforePrint->isChecked() ) {
        mpGrid->sortByRowTime();
    }

    if ( mpAcOptHideNoQtySizerColWhenPrint->isVisible() && mpAcOptHideNoQtySizerColWhenPrint->isChecked())
        mpSheetCargoGrid->autoHideNoQtySizerCol();

    QString printErr = mpPrinter->doPrint();
    if ( !printErr.isEmpty() )
        QMessageBox::information(this, QString(), printErr);
}

void BsAbstractSheetWin::doNew()
{
    openSheet(-1);
}

void BsAbstractSheetWin::doEdit()
{
    setEditable(true);

    if ( mpSheetCargoGrid ) {
        QString trader = mpTrader->mpEditor->text();
        mpSheetCargoGrid->setTraderDiscount(getTraderDisByName(trader));
        mpSheetCargoGrid->setTraderName(trader);
    }
}

void BsAbstractSheetWin::doDel()
{
    if ( ! confirmDialog(this,
                         mapMsg.value("i_delete_sheet_notice"),
                         mapMsg.value("i_delete_sheet_confirm"),
                         mapMsg.value("btn_ok"),
                         mapMsg.value("btn_cancel"),
                         QMessageBox::Warning) )
        return;

    //语句
    qint64 uptime = QDateTime::currentMSecsSinceEpoch() / 1000;
    QStringList sqls;
    sqls << QStringLiteral("delete from %1dtl where parentid=%2;")
            .arg(mMainTable).arg(mCurrentSheetId);

    sqls << QStringLiteral("update %1 set proof='', stype='', staff='', "
                           "shop='', trader='', remark='', sumqty=0, summoney=0, sumdis=0, "
                           "actpay=0, actowe=0, upman='%2', uptime=%3 where sheetid=%4")
            .arg(mMainTable).arg(loginer).arg(uptime).arg(mCurrentSheetId);

    //批执行
    QString sqlErr = sqliteCommit(sqls);
    if ( sqlErr.isEmpty() )
    {
        int keepId = mCurrentSheetId;
        openSheet(0);
        mCurrentSheetId = keepId;
        mpSheetId->setDataValue(keepId);
        mpSttValUpman->setText(loginer);
        mpSttValUptime->setText(QDateTime::fromMSecsSinceEpoch(1000 * uptime).toString("yyyy-MM-dd hh:mm:ss"));
        doSyncFindGrid();
    }
    else
        QMessageBox::information(this, QString(), sqlErr);
}

void BsAbstractSheetWin::doSave() //注意与openSheet()的一致性
{
    //存前检查
    uint ret = mpSheetGrid->saveCheck();
    if ( ret == bsccError )
    {
        QMessageBox::critical(nullptr, QString(), mapMsg.value("i_save_found_error"));
        return;
    }
    else if ( ret == bsccWarning )
    {
        if ( ! confirmDialog(this,
                             mapMsg.value("i_save_found_warning"),
                             mapMsg.value("i_save_ask_warning"),
                             mapMsg.value("btn_ok"),
                             mapMsg.value("btn_cancel"),
                             QMessageBox::Warning) )
            return;
    }

    //基础变量
    int useSheetId = mCurrentSheetId;
    qint64 uptime = QDateTime::currentMSecsSinceEpoch() / 1000;
    QString sql;

    //主表SQL
    if ( mCurrentSheetId <= 0 ) {
        //取得新sheetid
        QSqlQuery qry;
        qry.setForwardOnly(true);
        qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
        qry.exec(QStringLiteral("SELECT seq FROM sqlite_sequence WHERE name='%1';").arg(mMainTable));
        if ( qry.lastError().isValid() ) {
            QMessageBox::critical(this, QString(), QStringLiteral("Database fatal error.") + qry.lastError().text());
            qApp->quit();
            return;
        }
        if ( qry.next() )
            useSheetId = qry.value(0).toInt() + 1;
        else
            useSheetId = 1;

        //注册授权
        if ( useSheetId > 100 && !dogOk ) {
            QMessageBox::information(this, QString(), QStringLiteral("试用到期，请购买软件狗！"));
            return;
        }

        //准备值对
        QStringList flds;
        QStringList vals;

        flds << "sheetid";
        vals << QString::number(useSheetId);

        for ( int i = 0, iLen = mpPnlHeader->children().count(); i < iLen; ++i ) {
            BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlHeader->children().at(i));
            if ( edt ) {
                flds << edt->mpEditor->mpField->mFldName;
                vals << edt->mpEditor->getDataValueForSql();
            }
        }

        if ( mpPnlPayOwe ) {
            for ( int i = 0, iLen = mpPnlPayOwe->children().count(); i < iLen; ++i ) {
                BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlPayOwe->children().at(i));
                if ( edt ) {
                    flds << edt->mpEditor->mpField->mFldName;
                    vals << edt->mpEditor->getDataValueForSql();
                }
            }
        }

        //sumqty, summoney, sumdis
        if ( mMainTable != QStringLiteral("szd") ) {
            flds << "sumqty" << "summoney" << "sumdis";
            vals << bsNumForSave(mpSheetGrid->getColSumByFieldName("qty"))
                 << bsNumForSave(mpSheetGrid->getColSumByFieldName("actmoney"))
                 << bsNumForSave(mpSheetGrid->getColSumByFieldName("dismoney"));
        }

        flds << "upman" << "uptime";
        vals << QStringLiteral("'%1'").arg(loginer) << QString::number(uptime);

        //SQL
        sql = QStringLiteral("INSERT INTO %1(%2) values(%3);\n").arg(mMainTable)        //注意;\n结尾
                .arg(flds.join(QChar(44))).arg(vals.join(QChar(44)));
    }
    else {
        //值对
        QStringList exps;
        for ( int i = 0, iLen = mpPnlHeader->children().count(); i < iLen; ++i ) {
            BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlHeader->children().at(i));
            if ( edt ) {
                if ( edt->property(BSVALUE_OLD).toString() != edt->mpEditor->text() )
                    exps << QStringLiteral("%1=%2")
                            .arg(edt->mpEditor->mpField->mFldName)
                            .arg(edt->mpEditor->getDataValueForSql());
            }
        }

        if ( mpPnlPayOwe ) {
            for ( int i = 0, iLen = mpPnlPayOwe->children().count(); i < iLen; ++i ) {
                BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlPayOwe->children().at(i));
                if ( edt ) {
                    if ( edt->property(BSVALUE_OLD).toString() != edt->mpEditor->text() )
                        exps << QStringLiteral("%1=%2")
                                .arg(edt->mpEditor->mpField->mFldName)
                                .arg(edt->mpEditor->getDataValueForSql());
                }
            }
        }

        if ( mMainTable != QStringLiteral("szd") ) {
            exps << QStringLiteral("sumqty=%1").arg(bsNumForSave(mpSheetGrid->getColSumByFieldName("qty")))
                 << QStringLiteral("summoney=%1").arg(bsNumForSave(mpSheetGrid->getColSumByFieldName("actmoney")))
                 << QStringLiteral("sumdis=%1").arg(bsNumForSave(mpSheetGrid->getColSumByFieldName("dismoney")));
        }

        exps << QStringLiteral("upman='%1'").arg(loginer)
             << QStringLiteral("uptime=%1").arg(uptime);

        //SQL
        sql = QStringLiteral("UPDATE %1 SET %2 WHERE sheetid=%3;\n").arg(mMainTable)       //注意;\n结尾
                .arg(exps.join(QChar(44))).arg(useSheetId);
    }

    //从表SQL
    QString sqlDetail = mpSheetGrid->getSqliteSaveSql();
    sqlDetail.replace(mapMsg.value("app_sheetid_placeholer"), QString::number(useSheetId));
    sql += sqlDetail;

    //批执行
    QStringList sqls = sql.split(QStringLiteral(";\n"), QString::SkipEmptyParts);
    QString sqlErr = sqliteCommit(sqls);
    if ( sqlErr.isEmpty() ) {
        mpSheetGrid->savedReconcile();
        mpSheetGrid->setEditable(false);
        savedReconcile(useSheetId, uptime);
        setEditable(false);
        doSyncFindGrid();
    }
    else {
        QMessageBox::information(this, QString(), sqlErr);
    }
}

void BsAbstractSheetWin::doCancel()
{
    if ( mCurrentSheetId > 0 ) {
        mpSheetGrid->cancelRestore();
        mpSheetGrid->setEditable(false);
        cancelRestore();
        setEditable(false);
    }
    else {
        openSheet(0);
    }
}

bool BsAbstractSheetWin::mainNeedSaveDirty()
{
    if ( ! isEditing() )
        return false;

    bool actPayDirty = ( mpActPay ) ? mpActPay->mpEditor->isDirty() : false;

    return mpStype->mpEditor->isDirty() ||
            mpStaff->mpEditor->isDirty() ||
            mpShop->mpEditor->isDirty() ||
            mpTrader->mpEditor->isDirty() ||
            mpProof->mpEditor->isDirty() ||
            ( mpDated->mpEditor->isDirty() && mCurrentSheetId > 0 ) ||
            mpRemark->mpEditor->isDirty() ||
            actPayDirty;
}

double BsAbstractSheetWin::getTraderDisByName(const QString &name)
{
    if ( mMainTable.contains("cg") ) {
        if ( dsSupplier->keyExists(name) ) {
            return dsSupplier->getValue(name, QStringLiteral("regdis")).toLongLong() / 10000.0;
        }
    }
    else if ( mMainTable.contains("pf") || mMainTable.contains("xs") || mMainTable.contains("lsd") ) {
        if ( dsCustomer->keyExists(name) ) {
            return dsCustomer->getValue(name, QStringLiteral("regdis")).toLongLong() / 10000.0;
        }
    }
    else {
        if ( dsShop->keyExists(name) ) {
            return dsShop->getValue(name, QStringLiteral("regdis")).toLongLong() / 10000.0;
        }
    }
    return 1.0;
}

void BsAbstractSheetWin::clickToolPrintSetting()
{
    LxPrintSettingWin dlg(this);
    dlg.setWindowTitle(windowTitle() + mapMsg.value("tool_print_setting"));
    if ( QDialog::Accepted == dlg.exec() )
        mpPrinter->loadPrintSettings();
}

void BsAbstractSheetWin::clickToolUnCheck()
{
    if ( ! loginAsBoss ) return;

    QString sql = QStringLiteral("update %1 set checker='', chktime=0 where sheetid=%2;")
            .arg(mMainTable).arg(mCurrentSheetId);
    QSqlQuery qry;
    qry.exec(sql);
    if ( qry.lastError().isValid() )
        QMessageBox::information(this, QString(), mapMsg.value("i_uncheck_sheet_failed"));
    else {
        mpAcMainEdit->setEnabled(true);
        mpAcMainDel->setEnabled(true);
        mpAcMainCheck->setEnabled(true);
        mpToolUnCheck->setEnabled(false);
        mpCheckMark->setDataValue(0, mCurrentSheetId);
        mpSttValChecker->setText(QString());
        mpSttValChkTime->setText(QString());
        doSyncFindGrid();
        emit shopStockChanged(mpShop->mpEditor->getDataValue(), mMainTable, mCurrentSheetId);
        QMessageBox::information(this, QString(), mapMsg.value("i_uncheck_sheet_success"));
    }
}

void BsAbstractSheetWin::clickToolAdjustCurrentRowPosition()
{
    //先检查原序
    if ( !mpSheetGrid->isCleanSort() ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_adjust_rowtime_need_clean_sort"));
        return;
    }
    //调整
    mpSheetGrid->adjustCurrentRowPosition();
}

void BsAbstractSheetWin::clickQuickPeriod()
{
    QAction *act = qobject_cast<QAction*>(QObject::sender());
    Q_ASSERT(act);
    setQuickDate(act->text(), mpConDateB, mpConDateE, mpBtnPeriod);
}

void BsAbstractSheetWin::clickExecuteQuery()
{
    doOpenQuery();
}

void BsAbstractSheetWin::clickCancelOpenPage()
{
    mpLayBody->setContentsMargins(1, 0, 1, 8);
    mpPnlOpener->hide();
    mpToolBar->show();
    mpPnlHeader->show();
    mpGrid->show();
    mpStatusBar->show();
    if ( mpPnlPayOwe && mMainTable != QStringLiteral("syd") )
        mpPnlPayOwe->show();
}

void BsAbstractSheetWin::doubleClickOpenSheet(QTableWidgetItem *item)
{
    if ( item ) {
        openSheet(mpFindGrid->item(item->row(), 0)->text().toInt());
    }
}

void BsAbstractSheetWin::savedReconcile(const int sheetId, const qint64 uptime)
{
    mCurrentSheetId = sheetId;
    mpSheetId->setDataValue(sheetId);
    mpCheckMark->setDataValue(0, sheetId);
    mpSttValKey->setText(QStringLiteral("%1%2").arg(mMainTable.toUpper())
                         .arg(sheetId, 8, 10, QLatin1Char('0')));
    mpSttValUpman->setText(loginer);
    QDateTime dtUptime = QDateTime::fromMSecsSinceEpoch(uptime * 1000);
    mpSttValUptime->setText(dtUptime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    for ( int i = 0, iLen = mpPnlHeader->children().count(); i < iLen; ++i ) {
        BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlHeader->children().at(i));
        if ( edt ) {
            edt->setProperty(BSVALUE_OLD, edt->mpEditor->text());
        }
    }

    if ( mpPnlPayOwe ) {
        for ( int i = 0, iLen = mpPnlPayOwe->children().count(); i < iLen; ++i ) {
            BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlPayOwe->children().at(i));
            if ( edt ) {
                edt->setProperty(BSVALUE_OLD, edt->mpEditor->text());
            }
        }
    }
}

void BsAbstractSheetWin::cancelRestore()
{
    for ( int i = 0, iLen = mpPnlHeader->children().count(); i < iLen; ++i ) {
        BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlHeader->children().at(i));
        if ( edt ) {
            edt->mpEditor->setText( edt->property(BSVALUE_OLD).toString() );
        }
    }

    if ( mpPnlPayOwe ) {
        for ( int i = 0, iLen = mpPnlPayOwe->children().count(); i < iLen; ++i ) {
            BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlPayOwe->children().at(i));
            if ( edt ) {
                edt->mpEditor->setText( edt->property(BSVALUE_OLD).toString() );
            }
        }
    }
}


// BsSheetCargoWin
BsSheetCargoWin::BsSheetCargoWin(QWidget *parent, const QString &name, const QStringList &fields)
    : BsAbstractSheetWin(parent, name, fields)
{
    //禁权字段
    if ( (!canRett && name == QStringLiteral("lsd")) ||
         (!canLott && name.startsWith(QStringLiteral("pf"))) ||
         (!canBuyy && name.startsWith(QStringLiteral("cg"))) ||
         (!canRett && !canLott && !canBuyy) )
    {
        mAllowPriceMoney = false;
        mDenyFields << "summoney" << "sumdis" << "actpay" << "actowe" << "price" << "discount" << "actmoney" << "dismoney";
    }
    else
        mAllowPriceMoney = true;

    //表格（隐藏列必须放最后，否则表格列数有变时，首列前会出现不可控宽度的无效列的BUG）
    int hpMarkNum = mapOption.value("sheet_hpmark_define").toInt();
    QStringList cols;
    cols << QStringLiteral("cargo");

    if ( hpMarkNum > 0 && hpMarkNum < 7 ) {
        cols << QStringLiteral("hpmark");
    }

    if ( mapOption.value("show_hpname_in_sheet_grid") == QStringLiteral("是") ) {
        cols << QStringLiteral("hpname");
    }

    if ( mapOption.value("show_hpunit_in_sheet_grid") == QStringLiteral("是") ) {
        cols << QStringLiteral("unit");
    }

    if ( mapOption.value("show_hpprice_in_sheet_grid") == QStringLiteral("是") ) {
        cols << QStringLiteral("setprice");
    }

    cols << QStringLiteral("color")
         << QStringLiteral("qty") << QStringLiteral("price") << QStringLiteral("actmoney")
         << QStringLiteral("discount") << QStringLiteral("dismoney")
         << QStringLiteral("rowmark") << QStringLiteral("rowtime") << QStringLiteral("sizers");

    for ( int i = 0, iLen = cols.length(); i < iLen; ++i ) {
        QString col = cols.at(i);
        QStringList defs = mapMsg.value(QStringLiteral("fld_%1").arg(col)).split(QChar(9));
        Q_ASSERT(defs.count() > 4);
        BsField* bsCol = new BsField(col,
                                     defs.at(0),
                                     QString(defs.at(3)).toUInt(),
                                     QString(defs.at(4)).toInt(),
                                     defs.at(2));
        resetFieldDotsDefine(bsCol);
        mGridFlds << bsCol;
    }

    mpSheetFinanceGrid = nullptr;
    mpSheetCargoGrid = new BsSheetCargoGrid(this, name, mGridFlds);
    mpGrid = mpSheetCargoGrid;
    mpFormGrid = mpSheetCargoGrid;
    mpSheetGrid = mpSheetCargoGrid;

    mpSheetCargoGrid->mDenyFields << mDenyFields;
    mpLayBody->insertWidget(2, mpSheetCargoGrid, 1);

    connect(mpSheetCargoGrid, SIGNAL(shootHintMessage(QString)),  this, SLOT(displayGuideTip(QString)));
    connect(mpSheetCargoGrid, SIGNAL(shootForceMessage(QString)), this, SLOT(forceShowMessage(QString)));
    connect(mpSheetCargoGrid, SIGNAL(filterDone()), mpToolBar, SLOT(hide()));
    connect(mpSheetCargoGrid, SIGNAL(filterEmpty()), mpToolBar, SLOT(show()));
    connect(mpSheetCargoGrid, SIGNAL(focusOuted()), mpStatusBar, SLOT(clearMessage()));
    connect(mpSheetCargoGrid, SIGNAL(shootCurrentRowSysValue(QStringList)), this, SLOT(showCargoInfo(QStringList)));
    connect(mpSheetCargoGrid, SIGNAL(sheetSumMoneyChanged(QString)), this, SLOT(sumMoneyChanged(QString)));
    connect(mpToolHideCurrentCol, SIGNAL(triggered(bool)), mpSheetCargoGrid, SLOT(hideCurrentCol()));
    connect(mpToolShowAllCols, SIGNAL(triggered(bool)), mpSheetCargoGrid, SLOT(showHiddenCols()));
    connect(mpTrader, &BsFldBox::inputEditing, this, &BsSheetCargoWin::traderEditing);

    //表格底部收付款
    if ( mpPnlPayOwe ) {
        if ( name == QStringLiteral("cgd") || name == QStringLiteral("pfd") || name == QStringLiteral("syd") )
            mpPnlPayOwe->hide();
        else
            connect(mpActPay, SIGNAL(editingFinished()), this, SLOT(actPayChanged()));
    }

    //工具箱
    mpAcToolDefineName = mpMenuToolCase->addAction(QIcon(),
                                                   mapMsg.value("tool_define_name"),
                                                   this, SLOT(doToolDefineFieldName()));
    mpAcToolDefineName->setProperty(BSACFLAGS, 0);
    mpAcToolDefineName->setProperty(BSACRIGHT, loginAsAdminOrBoss);
    mpMenuToolCase->insertAction(mpToolPrintSetting, mpAcToolDefineName);

    mpAcToolImportBatchBarcodes = mpMenuToolCase->addAction(QIcon(), mapMsg.value("tool_import_batch_barcodes"),
                                                            this, SLOT(doToolImportBatchBarcodes()));
    mpAcToolImportBatchBarcodes->setProperty(BSACFLAGS, bsacfDirty);
    mpAcToolImportBatchBarcodes->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsNew) || canDo(mRightWinName, bsrsUpd));

    mpAcToolPrintCargoLabels = mpMenuToolCase->addAction(QIcon(), mapMsg.value("tool_print_cargo_labels"),
                                                            this, SLOT(doToolPrintCargoLabels()));
    mpAcToolPrintCargoLabels->setProperty(BSACFLAGS, bsacfClean | bsacfPlusId);
    mpAcToolPrintCargoLabels->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsPrint));

    mpAcToolImport->setText(mapMsg.value("tool_copy_import_sheet"));

    //开关盒
    mpAcOptHideNoQtySizerColWhenOpen->setVisible(true);
    mpAcOptHideNoQtySizerColWhenPrint->setVisible(true);

    mpAcOptPrintZeroSizeQty = mpMenuOptionBox->addAction(QIcon(), mapMsg.value("opt_print_zero_size_qty"),  //后期设为奇洛专用
                                                         this, SLOT(clickOptHideDropRow()));
    mpAcOptPrintZeroSizeQty->setProperty("optname", "opt_print_zero_size_qty");
    mpAcOptPrintZeroSizeQty->setProperty(BSACFLAGS, 0);
    mpAcOptPrintZeroSizeQty->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsPrint));

    mpAcOptAutoUseFirstColor = mpMenuOptionBox->addAction(QIcon(), mapMsg.value("opt_auto_use_first_color"));
    mpAcOptAutoUseFirstColor->setProperty("optname", "opt_auto_use_first_color");
    mpAcOptAutoUseFirstColor->setProperty(BSACFLAGS, bsacfDirty);
    mpAcOptAutoUseFirstColor->setProperty(BSACRIGHT, canDo(mRightWinName, bsrsNew) || canDo(mRightWinName, bsrsUpd));

    //窗口
    loadFldsUserNameSetting();

    if ( name == QStringLiteral("syd") ) {
        mpConTrader->hide();
        mpTrader->hide();
        mpPnlPayOwe->hide();
    }


    //扫描助手
    mpPnlScan = new QLabel;
    mpPnlScan->setText(mapMsg.value("i_scan_assistant"));
    mpPnlScan->setAlignment(Qt::AlignCenter);
    mpPnlScan->setStyleSheet("color:#999;");

    //拣货助手
    mpDsAttr1 = new BsListModel(this, QStringLiteral("select distinct attr1 from cargo order by attr1;"), false);
    mpDsAttr2 = new BsListModel(this, QStringLiteral("select distinct attr2 from cargo order by attr2;"), false);
    mpDsAttr3 = new BsListModel(this, QStringLiteral("select distinct attr3 from cargo order by attr3;"), false);
    mpDsAttr4 = new BsListModel(this, QStringLiteral("select distinct attr4 from cargo order by attr4;"), false);
    mpDsAttr5 = new BsListModel(this, QStringLiteral("select distinct attr5 from cargo order by attr5;"), false);
    mpDsAttr6 = new BsListModel(this, QStringLiteral("select distinct attr6 from cargo order by attr6;"), false);

    mpDsAttr1->reload();
    mpDsAttr2->reload();
    mpDsAttr3->reload();
    mpDsAttr4->reload();
    mpDsAttr5->reload();
    mpDsAttr6->reload();

    QStringList sizerTypeDefs = mapMsg.value(QStringLiteral("fld_sizertype")).split(QChar(9));
    Q_ASSERT(sizerTypeDefs.count() > 4);
    mPickSizerFld = new BsField("sizertype", sizerTypeDefs.at(0), QString(sizerTypeDefs.at(3)).toUInt(),
                                QString(sizerTypeDefs.at(4)).toInt(), sizerTypeDefs.at(2));

    QStringList attr1Defs = mapMsg.value(QStringLiteral("fld_attr1")).split(QChar(9));
    Q_ASSERT(attr1Defs.count() > 4);
    mPickAttr1Fld = new BsField("attr1", attr1Defs.at(0), QString(attr1Defs.at(3)).toUInt(),
                                QString(attr1Defs.at(4)).toInt(), attr1Defs.at(2));
    mPickAttr1Fld->mFldCnName = mapOption.value(QStringLiteral("cargo_attr1_name"));

    QStringList attr2Defs = mapMsg.value(QStringLiteral("fld_attr2")).split(QChar(9));
    Q_ASSERT(attr2Defs.count() > 4);
    mPickAttr2Fld = new BsField("attr2", attr2Defs.at(0), QString(attr2Defs.at(3)).toUInt(),
                                QString(attr2Defs.at(4)).toInt(), attr2Defs.at(2));
    mPickAttr2Fld->mFldCnName = mapOption.value(QStringLiteral("cargo_attr2_name"));

    QStringList attr3Defs = mapMsg.value(QStringLiteral("fld_attr3")).split(QChar(9));
    Q_ASSERT(attr3Defs.count() > 4);
    mPickAttr3Fld = new BsField("attr3", attr3Defs.at(0), QString(attr3Defs.at(3)).toUInt(),
                                QString(attr3Defs.at(4)).toInt(), attr3Defs.at(2));
    mPickAttr3Fld->mFldCnName = mapOption.value(QStringLiteral("cargo_attr3_name"));

    QStringList attr4Defs = mapMsg.value(QStringLiteral("fld_attr4")).split(QChar(9));
    Q_ASSERT(attr4Defs.count() > 4);
    mPickAttr4Fld = new BsField("attr4", attr4Defs.at(0), QString(attr4Defs.at(3)).toUInt(),
                                QString(attr4Defs.at(4)).toInt(), attr4Defs.at(2));
    mPickAttr4Fld->mFldCnName = mapOption.value(QStringLiteral("cargo_attr4_name"));

    QStringList attr5Defs = mapMsg.value(QStringLiteral("fld_attr5")).split(QChar(9));
    Q_ASSERT(attr5Defs.count() > 4);
    mPickAttr5Fld = new BsField("attr5", attr5Defs.at(0), QString(attr5Defs.at(3)).toUInt(),
                                QString(attr5Defs.at(4)).toInt(), attr5Defs.at(2));
    mPickAttr5Fld->mFldCnName = mapOption.value(QStringLiteral("cargo_attr5_name"));

    QStringList attr6Defs = mapMsg.value(QStringLiteral("fld_attr6")).split(QChar(9));
    Q_ASSERT(attr6Defs.count() > 4);
    mPickAttr6Fld = new BsField("attr6", attr6Defs.at(0), QString(attr6Defs.at(3)).toUInt(),
                                QString(attr6Defs.at(4)).toInt(), attr6Defs.at(2));
    mPickAttr6Fld->mFldCnName = mapOption.value(QStringLiteral("cargo_attr6_name"));

    mpPnlPick = new QWidget;
    QHBoxLayout *layPick = new QHBoxLayout(mpPnlPick);
    layPick->setContentsMargins(1, 0, 1, 0);
    layPick->setSpacing(0);

    mpPickGrid = new BsSheetStockPickGrid(this);

    mpPickSizerType = new BsFldEditor(this, mPickSizerFld, dsSizer);
    mpPickSizerType->setMyPlaceText(QStringLiteral("请先选择") + mPickSizerFld->mFldCnName);
    mpPickSizerType->setMyPlaceColor(QColor(255, 0, 0));

    mpPickAttr1 = new BsFldEditor(this, mPickAttr1Fld, mpDsAttr1);
    mpPickAttr1->setMyPlaceText(mPickAttr1Fld->mFldCnName);
    mpPickAttr1->setMyPlaceColor(QColor(0, 150, 0));

    mpPickAttr2 = new BsFldEditor(this, mPickAttr2Fld, mpDsAttr2);
    mpPickAttr2->setMyPlaceText(mPickAttr2Fld->mFldCnName);
    mpPickAttr2->setMyPlaceColor(QColor(0, 150, 0));

    mpPickAttr3 = new BsFldEditor(this, mPickAttr3Fld, mpDsAttr3);
    mpPickAttr3->setMyPlaceText(mPickAttr3Fld->mFldCnName);
    mpPickAttr3->setMyPlaceColor(QColor(0, 150, 0));

    mpPickAttr4 = new BsFldEditor(this, mPickAttr4Fld, mpDsAttr4);
    mpPickAttr4->setMyPlaceText(mPickAttr4Fld->mFldCnName);
    mpPickAttr4->setMyPlaceColor(QColor(0, 150, 0));

    mpPickAttr5 = new BsFldEditor(this, mPickAttr5Fld, mpDsAttr5);
    mpPickAttr5->setMyPlaceText(mPickAttr5Fld->mFldCnName);
    mpPickAttr5->setMyPlaceColor(QColor(0, 150, 0));

    mpPickAttr6 = new BsFldEditor(this, mPickAttr6Fld, mpDsAttr6);
    mpPickAttr6->setMyPlaceText(mPickAttr6Fld->mFldCnName);
    mpPickAttr6->setMyPlaceColor(QColor(0, 150, 0));

    mpPickDate = new QCheckBox(QStringLiteral("不限日期"), this);
    mpPickCheck = new QCheckBox(QStringLiteral("不限审核"), this);
    mpPickTrader = new QCheckBox(mapMsg.value("word_pick_trader_stock"), this);
    mpPickTrader->setVisible(mMainTable == QStringLiteral("dbd"));

    QPushButton *btnPickQuery = new QPushButton(QIcon(":/icon/query.png"), mapMsg.value("word_query"), this);

    mpPickCons = new QWidget(this);
    QVBoxLayout *layFilterPick = new QVBoxLayout(mpPickCons);
    layFilterPick->setContentsMargins(9, 0, 9, 0);
    layFilterPick->setSpacing(2);
    layFilterPick->addWidget(mpPickSizerType);
    layFilterPick->addWidget(mpPickAttr1);
    layFilterPick->addWidget(mpPickAttr2);
    layFilterPick->addWidget(mpPickAttr3);
    layFilterPick->addWidget(mpPickAttr4);
    layFilterPick->addWidget(mpPickAttr5);
    layFilterPick->addWidget(mpPickAttr6);
    layFilterPick->addWidget(mpPickDate);
    layFilterPick->addWidget(mpPickCheck);
    layFilterPick->addWidget(mpPickTrader);
    layFilterPick->addWidget(btnPickQuery);
    layFilterPick->addStretch();

    layPick->addWidget(mpPickGrid, 1);
    layPick->addWidget(mpPickCons);

    //助手布局
    mpTaber->addTab(mpPnlScan, QIcon(":/icon/scan.png"), mapMsg.value("word_scan_assitant"));
    mpTaber->addTab(mpPnlPick, QIcon(":/icon/shop.png"), mapMsg.value("word_pick_assitant"));

    connect(mpTaber, SIGNAL(currentChanged(int)), this, SLOT(taberIndexChanged(int)));
    connect(mpPickTrader, SIGNAL(clicked(bool)), this, SLOT(pickStockTraderChecked()));
    connect(btnPickQuery, SIGNAL(clicked(bool)), this, SLOT(loadPickStock()));
    connect(mpPickGrid, SIGNAL(pickedCell(QString,QString,QString)),
            mpSheetCargoGrid, SLOT(addOneCargo(QString,QString,QString)));
    connect(mpPickGrid, &BsSheetStockPickGrid::cargoRowSelected, mpSheetCargoGrid, &BsSheetCargoGrid::tryLocateCargoRow);
    connect(mpSheetCargoGrid, &BsSheetCargoGrid::cargoRowSelected, mpPickGrid, &BsSheetStockPickGrid::tryLocateCargoRow);

    //初始
    openSheet(0);
}

BsSheetCargoWin::~BsSheetCargoWin()
{
    delete mPickSizerFld;
    delete mPickAttr1Fld;
    delete mPickAttr2Fld;
    delete mPickAttr3Fld;
    delete mPickAttr4Fld;
    delete mPickAttr5Fld;
    delete mPickAttr6Fld;

    delete mpDsAttr1;
    delete mpDsAttr2;
    delete mpDsAttr3;
    delete mpDsAttr4;
    delete mpDsAttr5;
    delete mpDsAttr6;

    qDeleteAll(mGridFlds);
    mGridFlds.clear();
}

void BsSheetCargoWin::showEvent(QShowEvent *e)
{
    BsAbstractSheetWin::showEvent(e);

    mpSheetCargoGrid->setFontPoint(9);
    mpSheetCargoGrid->setRowHeight(22);
}

void BsSheetCargoWin::doOpenQuery()
{
    QStringList sels;
    QStringList cons;

    sels << QStringLiteral("sheetid") << QStringLiteral("dated");
    cons << QStringLiteral("(dated BETWEEN %1 AND %2)")
            .arg(mpConDateB->mpEditor->getDataValue())
            .arg(mpConDateE->mpEditor->getDataValue());

    if ( mpConStype->mpEditor->getDataValue().isEmpty() )
        sels << QStringLiteral("stype");
    else
        cons << QStringLiteral("stype='%1'").arg(mpConStype->mpEditor->getDataValue());

    if ( mpConStaff->mpEditor->getDataValue().isEmpty() )
        sels << QStringLiteral("staff");
    else
        cons << QStringLiteral("staff='%1'").arg(mpConStaff->mpEditor->getDataValue());

    if ( mpConShop->mpEditor->getDataValue().isEmpty() )
        sels << QStringLiteral("shop");
    else
        cons << QStringLiteral("shop='%1'").arg(mpConShop->mpEditor->getDataValue());

    if ( !mMainTable.startsWith(QStringLiteral("syd")) )
    {
        if ( mpConTrader->mpEditor->getDataValue().isEmpty() )
            sels << QStringLiteral("trader");
        else
            cons << QStringLiteral("trader='%1'").arg(mpConTrader->mpEditor->getDataValue());
    }

    sels << QStringLiteral("sumqty") << QStringLiteral("chktime");

    //价款权限
    if ( (canRett || mMainTable == QStringLiteral("lsd")) &&
         (canLott || !mMainTable.startsWith(QStringLiteral("pf"))) &&
         (canBuyy || !mMainTable.startsWith(QStringLiteral("cg"))) ) {
        sels << QStringLiteral("summoney");
        sels << QStringLiteral("actpay");
        sels << QStringLiteral("actowe");
    }

    if ( !mpConCheck->getConExp().isEmpty() )
        cons << mpConCheck->getConExp();
    QStringList colTitles;
    colTitles << QStringLiteral("shop\t%1").arg(getFieldByName(QStringLiteral("shop"))->mFldCnName)
         << QStringLiteral("trader\t%1").arg(getFieldByName(QStringLiteral("trader"))->mFldCnName)
         << QStringLiteral("actpay\t%1").arg(getFieldByName(QStringLiteral("actpay"))->mFldCnName);

    QString sql = QStringLiteral("SELECT %1 FROM %2 WHERE %3 ORDER BY sheetid;")
            .arg(sels.join(QChar(44)))
            .arg(mMainTable)
            .arg(cons.join(QStringLiteral(" AND ")));

    //加载数据
    mpFindGrid->loadData(sql, colTitles);

    //加载列宽
    mpFindGrid->loadColWidths();
}

void BsSheetCargoWin::doSyncFindGrid()
{
    int sheetIdCol = mpFindGrid->getColumnIndexByFieldName("sheetid");
    if ( sheetIdCol < 0 )
        return;

    int row = -1;
    for ( int i = 0, iLen = mpFindGrid->rowCount(); i < iLen; ++i ) {
        if ( mpFindGrid->item(i, sheetIdCol)->text().toInt() == mCurrentSheetId ) {
            row = i;
            break;
        }
    }
    if ( row < 0 )
        return;

    int datedCol = mpFindGrid->getColumnIndexByFieldName("dated");
    if ( datedCol > 0 )
        mpFindGrid->item(row, datedCol)->setText(getPrintValue("dated"));

    int stypeCol = mpFindGrid->getColumnIndexByFieldName("stype");
    if ( stypeCol > 0 )
        mpFindGrid->item(row, stypeCol)->setText(getPrintValue("stype"));

    int staffCol = mpFindGrid->getColumnIndexByFieldName("staff");
    if ( staffCol > 0 )
        mpFindGrid->item(row, staffCol)->setText(getPrintValue("staff"));

    int shopCol = mpFindGrid->getColumnIndexByFieldName("shop");
    if ( shopCol > 0 )
        mpFindGrid->item(row, shopCol)->setText(getPrintValue("shop"));

    int traderCol = mpFindGrid->getColumnIndexByFieldName("trader");
    if ( traderCol > 0 )
        mpFindGrid->item(row, traderCol)->setText(getPrintValue("trader"));

    int qtyCol = mpFindGrid->getColumnIndexByFieldName("sumqty");
    if ( qtyCol > 0 )
        mpFindGrid->item(row, qtyCol)->setText(getPrintValue("sumqty"));

    int mnyCol = mpFindGrid->getColumnIndexByFieldName("summoney");
    if ( mnyCol > 0 )
        mpFindGrid->item(row, mnyCol)->setText(getPrintValue("summoney"));

    int payCol = mpFindGrid->getColumnIndexByFieldName("actpay");
    if ( payCol > 0 )
        mpFindGrid->item(row, payCol)->setText(getPrintValue("actpay"));

    int oweCol = mpFindGrid->getColumnIndexByFieldName("actowe");
    if ( oweCol > 0 )
        mpFindGrid->item(row, oweCol)->setText(getPrintValue("actowe"));

    if (mpCheckMark->getDataCheckedValue()) {
        mpFindGrid->item(row, sheetIdCol)->setData(Qt::DecorationRole, QIcon(":/icon/check.png"));
    } else {
        mpFindGrid->item(row, sheetIdCol)->setData(Qt::DecorationRole, QVariant());
    }
}

void BsSheetCargoWin::updateTabber(const bool editablee)
{
    mpPickGrid->hide();
    mpPickCons->hide();
    mpTaber->setCurrentIndex(0);
    mpTaber->setVisible(editablee);
    if ( editablee )
        restoreTaberMiniHeight();
}

void BsSheetCargoWin::doToolExport()
{
    QStringList headPairs;
    headPairs << QStringLiteral("%1:,%2").arg(getFieldByName("sheetid")->mFldCnName).arg(mpSheetId->getDisplayText())
              << QStringLiteral("%1:,%2").arg(getFieldByName("dated")->mFldCnName).arg(mpDated->mpEditor->text())
              << QStringLiteral("%1:,%2").arg(getFieldByName("stype")->mFldCnName).arg(mpStype->mpEditor->text())
              << QStringLiteral("%1:,%2").arg(getFieldByName("staff")->mFldCnName).arg(mpStaff->mpEditor->text())
              << QStringLiteral("%1:,%2").arg(getFieldByName("shop")->mFldCnName).arg(mpShop->mpEditor->text())
              << QStringLiteral("%1:,%2").arg(getFieldByName("trader")->mFldCnName).arg(mpTrader->mpEditor->text())
              << QStringLiteral("%1:,%2").arg(getFieldByName("remark")->mFldCnName).arg(mpRemark->mpEditor->text())
              << QStringLiteral("%1:,%2").arg(getFieldByName("actpay")->mFldCnName).arg(mpActPay->mpEditor->text())
              << QStringLiteral("%1:,%2").arg(getFieldByName("actowe")->mFldCnName).arg(mpActOwe->mpEditor->text());
    exportGrid(mpGrid, headPairs);
}

void BsSheetCargoWin::doToolImport()
{
    BsCopyImportSheetDlg dlg(this);
    if ( dlg.exec() != QDialog::Accepted )
        return;

    QString tbl = dlg.mpCmbSheetName->currentData().toString();
    int sheetId = dlg.mpEdtSheetId->text().toInt();

    QString sql = QStringLiteral("select cargo, color, sizers from %1dtl where parentid=%2;").arg(tbl).arg(sheetId);
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    while ( qry.next() ) {
        QString cargo = qry.value(0).toString();
        QString color = qry.value(1).toString();
        QStringList sizers = qry.value(2).toString().split(QChar(10));
        for ( int i = 0, iLen = sizers.length(); i < iLen; ++i ) {
            QStringList sizerPair = QString(sizers.at(i)).split(QChar(9));
            if ( sizerPair.length() == 2 ) {
                QString sizerName = sizerPair.at(0);
                qint64 sizerQty = QString(sizerPair.at(1)).toLongLong();
                mpSheetCargoGrid->inputNewCargoRow(cargo, color, sizerName, sizerQty, false);
            }
        }
    }
    qry.finish();

    /* 因为已经做了doToolCopyImport()功能，此功能不开放，待删除。原因是当初对应的导出是特殊格式导入方便，后来改为普通表格格式，以下代码不能用。

    QString dir = QStandardPaths::locate(QStandardPaths::DesktopLocation, QString(), QStandardPaths::LocateDirectory);
    QString openData = openLoadTextFile(mapMsg.value("tool_import_data"), dir, mapMsg.value("i_formatted_csv_file"), this);
    if ( openData.isEmpty() )
        return;

    //数据
    QStringList lines = openData.split(QChar(10));

    //以下确保等长
    QStringList cargos;
    QStringList colorNames;
    QStringList sizerNames;
    QStringList qtys;

    //数据行
    int cargoIdx = -1;
    int colorIdx = -1;
    int sizersIdx = -1;
    int colCount = 0;
    bool pass = true;
    for ( int i = 0, iLen = lines.length(); i < iLen; ++i )
    {
        QStringList cols = QString(lines.at(i)).split(QChar(44));

        //首行字段名，求列序
        if ( i == 0 )
        {
            colCount = cols.length();
            for ( int j = 0, jLen = cols.length(); j < jLen; ++j )
            {
                if ( cols.at(j) == QStringLiteral("cargo") ) {
                    cargoIdx = j;
                    continue;
                }
                if ( cols.at(j) == QStringLiteral("color") ) {
                    colorIdx = j;
                    continue;
                }
                if ( cols.at(j) == QStringLiteral("sizers") ) {
                    sizersIdx = j;
                    continue;
                }
            }
            if ( cargoIdx < 0 || colorIdx < 0 || sizersIdx < 0 ) {
                pass = false;
                break;
            }
        }

        //表格货品行
        else
        {
            if ( cols.length() != colCount ) {
                pass = false;
                break;
            }

            QString cargo = cols.at(cargoIdx);
            QString color = cols.at(colorIdx);
            QString sizers = cols.at(sizersIdx);
            QStringList spairs = sizers.split(QChar(';'), QString::SkipEmptyParts);
            bool pairPass = true;
            for ( int j = 0, jLen = spairs.length(); j < jLen; ++j )
            {
                QStringList spair = QString(spairs.at(j)).split(QChar(':'), QString::SkipEmptyParts);
                if ( spair.length() == 2 ) {
                    cargos << cargo;
                    colorNames << color;
                    sizerNames << spair.at(0);
                    qtys << spair.at(1);
                }
                else {
                    pairPass = false;
                    break;
                }
            }

            if ( ! pairPass ) {
                pass = false;
                break;
            }
        }
    }

    //处理
    if ( pass ) {
        QStringList lostReports;
        for ( int i = 0, iLen = cargos.length(); i < iLen; ++i )
        {
            QString inputErr = mpSheetCargoGrid->inputNewCargoRow(cargos.at(i),
                                                      colorNames.at(i),
                                                      sizerNames.at(i),
                                                      QString(qtys.at(i)).toLongLong(),
                                                      false);
            if ( !inputErr.isEmpty() )
                lostReports << QStringLiteral("%1 %2 %3: %4件")
                               .arg(cargos.at(i))
                               .arg(colorNames.at(i))
                               .arg(sizerNames.at(i))
                               .arg(QString(qtys.at(i)).toLongLong() / 10000.0);
        }

        //报告
        if ( lostReports.isEmpty() )
            QMessageBox::information(this, QString(), mapMsg.value("i_import_sheet_finished_ok"));
        else if ( lostReports.length() > 10 )
            QMessageBox::information(this, QString(), mapMsg.value("i_import_sheet_too_many_lost"));
        else
            forceShowMessage(mapMsg.value("i_import_sheet_lost_following") + lostReports.join(QChar(10)));
    }
    else
        QMessageBox::information(this, QString(), mapMsg.value("i_invliad_baili_sheet_data"));
    */
}

void BsSheetCargoWin::traderEditing(const QString &text, const bool)
{
    mpSheetCargoGrid->setTraderDiscount(getTraderDisByName(text));
    mpSheetCargoGrid->setTraderName(text);
}

void BsSheetCargoWin::showCargoInfo(const QStringList &values)
{
    Q_ASSERT(values.length()==2);
    QString cargoInfo = dsCargo->getCargoBasicInfo(values.at(0));
    if ( !cargoInfo.isEmpty() )
        mpStatusBar->showMessage(cargoInfo);
}

void BsSheetCargoWin::sumMoneyChanged(const QString &sumValue)
{
    double actPay = mpActPay->mpEditor->text().toDouble();
    double actSum = sumValue.toDouble();
    mpActOwe->mpEditor->setText(QString::number(actSum - actPay, 'f', mMoneyDots));
}

void BsSheetCargoWin::actPayChanged()
{
    double actPay = mpActPay->mpEditor->text().toDouble();
    double actSum = mpGrid->getFooterValueByField(QStringLiteral("actmoney")).toDouble();
    mpActOwe->mpEditor->setText(QString::number(actSum - actPay, 'f', mMoneyDots));
}

void BsSheetCargoWin::taberIndexChanged(int index)
{
    if ( mpShop->mpEditor->getDataValue().isEmpty() && index == 1 ) {
        QMessageBox::information(this, QString(), mapMsg.value("word_pick_assitant") + mapMsg.value("i_please_pick_shop_first"));
        mpTaber->setCurrentIndex(0);
        return;
    }
    mpPickGrid->setVisible(index > 0);
    mpPickCons->setVisible(index > 0);
    if ( index == 0 ) {
        restoreTaberMiniHeight();
    }
    if ( index == 1 ) {
        mpSpl->setSizes(QList<int>() << height() / 2 << height() / 3);
    }
}

void BsSheetCargoWin::doToolDefineFieldName()
{
    int hpMarkNum = mapOption.value("sheet_hpmark_define").toInt();

    QList<BsField*> flds;
    flds << getFieldByName("stype")
         << getFieldByName("staff")
         << getFieldByName("shop");

    if ( mMainTable != QStringLiteral("syd") )
        flds << getFieldByName("trader")
             << getFieldByName("actpay")
             << getFieldByName("actowe");

    flds << mpSheetCargoGrid->getFieldByName("cargo")
         << mpSheetCargoGrid->getFieldByName("color");

    if ( mMainTable != QStringLiteral("syd") )
        flds << mpSheetCargoGrid->getFieldByName("price")
             << mpSheetCargoGrid->getFieldByName("actmoney")
             << mpSheetCargoGrid->getFieldByName("dismoney");

    if ( hpMarkNum > 0 && hpMarkNum < 7 )
        flds << mpSheetCargoGrid->getFieldByName("hpmark");

    flds << mpSheetCargoGrid->getFieldByName("rowmark");

    BsFieldDefineDlg dlg(this, mMainTable, flds);
    if ( dlg.exec() == QDialog::Accepted ) {

        QStringList sqls;
        sqls << QStringLiteral("delete from bailifldname where tblname='%1';").arg(mMainTable);

        QList<QLineEdit *> edts = dlg.findChildren<QLineEdit *>();
        for ( int i = 0, iLen = edts.length(); i < iLen; ++i ) {
            QLineEdit *edt = edts.at(i);
            QString fld = edt->property("field_name").toString();
            QString oldName = edt->property("field_value").toString();
            QString newName = edt->text().trimmed();
            if ( oldName != newName && !newName.isEmpty() ) {

                BsField *bsFld = getFieldByName(fld);
                if ( bsFld )
                    bsFld->mFldCnName = newName;

                bsFld = mpSheetCargoGrid->getFieldByName(fld);
                if ( bsFld )
                    bsFld->mFldCnName = newName;

                sqls << QStringLiteral("insert into bailifldname(tblname, fldname, cname) values('%1', '%2', '%3');")
                        .arg(mMainTable).arg(fld).arg(newName);
            }
        }

        QString sqlErr = sqliteCommit(sqls);

        if ( sqlErr.isEmpty() )
            loadFldsUserNameSetting();
        else
            QMessageBox::information(this, QString(), sqlErr);
    }
}

void BsSheetCargoWin::doToolImportBatchBarcodes()
{
    QString dir = QStandardPaths::locate(QStandardPaths::DesktopLocation, QString(), QStandardPaths::LocateDirectory);
    QString openData = openLoadTextFile(mapMsg.value("tool_import_batch_barcodes"), dir,
                                        mapMsg.value("i_common_text_file"), this);
    if ( openData.isEmpty() )
        return;

    //原数据分行
    QStringList lines = openData.split(QChar(10));

    //待求条码列序，和数量列序
    int idxBar = 0;
    int idxQty = -1;

    //推测分隔符
    QChar colSplittor = (openData.indexOf(QChar(9)) > 0) ? QChar(9) : QChar(44);

    //推测是否单列，不是单列，对话框让用户给出
    int testRow = int(floor(lines.length() / 2));
    QStringList testCols = QString(lines.at(testRow)).split(colSplittor);
    if ( testCols.length() > 1 )
    {
        BsBarcodesImportDlg dlg(this, openData);
        if ( QDialog::Accepted != dlg.exec() )
            return;

        idxBar = dlg.mpBarCol->text().toInt() - 1;  //人数1基
        idxQty = dlg.mpQtyCol->text().toInt() - 1;  //人数1基
    }

    //就绪
    for ( int i = 0, iLen = lines.length(); i < iLen; ++i )
    {
        QStringList cols = QString(lines.at(i)).split(colSplittor);
        if ( cols.length() > idxBar && cols.length() > idxQty )
        {
            //取出
            QString barcode = cols.at(idxBar);
            int qty = ( idxQty >= 0 ) ? QString(cols.at(idxQty)).toInt() : 1;

            //扫描
            QString cargo, colorCode, sizerCode;
            if ( mpSheetCargoGrid->scanBarcode(barcode, &cargo, &colorCode, &sizerCode) )
            {
                QString err = mpSheetCargoGrid->inputNewCargoRow(cargo, colorCode, sizerCode, qty * 10000, true);
                if ( !err.isEmpty() ) {
                    QMessageBox::information(this, QString(), err);
                    return;
                }
            }
        }
    }
}

void BsSheetCargoWin::doToolPrintCargoLabels()
{
    BsLabelPrinter::doPrintSheet(mMainTable, mCurrentSheetId, this);
}

void BsSheetCargoWin::pickStockTraderChecked()
{
    QString trader = mpTrader->mpEditor->getDataValue();
    if ( QObject::sender() == mpPickTrader && trader.isEmpty() ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_please_pick_trader_first"));
        mpPickTrader->setChecked(false);
    }
}

void BsSheetCargoWin::loadPickStock()
{
    QString shop = mpShop->mpEditor->getDataValue();
    QString trader = mpTrader->mpEditor->getDataValue();
    QString stockShop = (mpPickTrader->isVisible() && mpPickTrader->isChecked()) ? trader : shop;
    QString attr1 = mpPickAttr1->getDataValue();
    QString attr2 = mpPickAttr2->getDataValue();
    QString attr3 = mpPickAttr3->getDataValue();
    QString attr4 = mpPickAttr4->getDataValue();
    QString attr5 = mpPickAttr5->getDataValue();
    QString attr6 = mpPickAttr6->getDataValue();
    QString sizerType = mpPickSizerType->getDataValue();
    if ( sizerType.isEmpty() ) {
        QMessageBox::information(this, QString(), mapMsg.value("i_please_pick_sizertype_first"));
        return;
    }

    if ( mpPickGrid->rowCount() > 0 ) {
        mpPickGrid->saveColWidths("pick");
    }

    int pickDelta = -1;
    if ( mMainTable == QStringLiteral("cgj") || mMainTable == QStringLiteral("pft") ||
         ( mMainTable == QStringLiteral("dbd") && mpPickTrader->isChecked() ) ) {
        pickDelta = 1;
    }
    mpPickGrid->setPickDelta(pickDelta);

    mpPickGrid->cancelAllFilters();

    QStringList cons;
    if ( ! mpPickDate->isChecked() )
        cons << QStringLiteral("dated<=%1").arg(QDateTime::currentSecsSinceEpoch());
    if ( ! mpPickCheck->isChecked() )
        cons << QStringLiteral("chktime<>0");
    cons << QStringLiteral("shop='%1'").arg(stockShop);
    cons << QStringLiteral("sizertype='%1'").arg(sizerType);
    if ( !attr1.isEmpty() ) cons << QStringLiteral("attr1='%1'").arg(attr1);
    if ( !attr2.isEmpty() ) cons << QStringLiteral("attr2='%1'").arg(attr2);
    if ( !attr3.isEmpty() ) cons << QStringLiteral("attr3='%1'").arg(attr3);
    if ( !attr4.isEmpty() ) cons << QStringLiteral("attr4='%1'").arg(attr4);
    if ( !attr5.isEmpty() ) cons << QStringLiteral("attr5='%1'").arg(attr5);
    if ( !attr6.isEmpty() ) cons << QStringLiteral("attr6='%1'").arg(attr6);

    //第一列必须cargo，第二列必须hpname，最后列必须sizers，约定见BsGrid::loadData()
    QString sql = QStringLiteral("SELECT cargo, hpname, unit, setprice, color, "
                                 "SUM(qty) AS qty, GROUP_CONCAT(sizers, '') AS sizers "
                                 "FROM vi_stock_attr "
                                 "WHERE %1 "
                                 "GROUP BY cargo, hpname, unit, setprice, color "
                                 "HAVING SUM(qty)<>0 "
                                 "ORDER BY cargo, color;").arg(cons.join(" AND "));
    mpPickGrid->loadData(sql, QStringList(), sizerType, true);
    mpPickGrid->loadColWidths("pick");

    QString msgHint = (mpPickGrid->rowCount() > 10)
            ? mapMsg.value("i_pick_keypress_hint")
            : ((mpPickGrid->rowCount() == 0) ? mapMsg.value("i_no_stock_queryed") : QString());
    mpPickGrid->updateHint(msgHint);
}

void BsSheetCargoWin::restoreTaberMiniHeight()
{
    int bodyHt = mpBody->height() - mpTaber->tabBar()->height() - mpPnlScan->sizeHint().height();
    QList<int> szs;
    szs << bodyHt << mpTaber->tabBar()->height();
    mpSpl->setSizes(szs);
}

void BsSheetCargoWin::loadFldsUserNameSetting()
{
    for ( int i = 0, iLen = mpPnlHeader->children().count(); i < iLen; ++i ) {
        BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlHeader->children().at(i));
        if ( edt )
            edt->mpLabel->setText(edt->mpEditor->mpField->mFldCnName);
    }
    if ( mpPnlPayOwe ) {
        for ( int i = 0, iLen = mpPnlPayOwe->children().count(); i < iLen; ++i ) {
            BsFldBox *edt = qobject_cast<BsFldBox*>(mpPnlPayOwe->children().at(i));
            if ( edt )
                edt->mpLabel->setText(edt->mpEditor->mpField->mFldCnName);
        }
    }
    mpSheetCargoGrid->updateColTitleSetting();
}


// BsSheetFinanceWin
BsSheetFinanceWin::BsSheetFinanceWin(QWidget *parent, const QStringList &fields)
    : BsAbstractSheetWin(parent, QStringLiteral("szd"), fields)
{
    //表格（隐藏列必须放最后，否则表格列数有变时，首列前会出现不可控宽度的无效列的BUG）
    QStringList cols;
    cols << QStringLiteral("subject") << QStringLiteral("income") << QStringLiteral("expense")
         << QStringLiteral("rowmark") << QStringLiteral("rowtime");

    for ( int i = 0, iLen = cols.length(); i < iLen; ++i ) {
        QString col = cols.at(i);
        QStringList defs = mapMsg.value(QStringLiteral("fld_%1").arg(col)).split(QChar(9));
        Q_ASSERT(defs.count() > 4);
        int fldLen = QString(defs.at(4)).toInt();
        if ( col == QStringLiteral("subject") ) fldLen = 100;
        BsField* bsCol = new BsField(col,
                                     defs.at(0),
                                     QString(defs.at(3)).toUInt(),
                                     fldLen,
                                     defs.at(2));
        resetFieldDotsDefine(bsCol);
        mGridFlds << bsCol;
    }

    mpSheetCargoGrid = nullptr;
    mpSheetFinanceGrid = new BsSheetFinanceGrid(this, mGridFlds);
    mpGrid = mpSheetFinanceGrid;
    mpFormGrid = mpSheetFinanceGrid;
    mpSheetGrid = mpSheetFinanceGrid;

    connect(mpSheetFinanceGrid, SIGNAL(shootHintMessage(QString)),  this, SLOT(displayGuideTip(QString)));
    connect(mpSheetFinanceGrid, SIGNAL(shootForceMessage(QString)), this, SLOT(forceShowMessage(QString)));
    connect(mpSheetFinanceGrid, SIGNAL(filterDone()), mpToolBar, SLOT(hide()));
    connect(mpSheetFinanceGrid, SIGNAL(filterEmpty()), mpToolBar, SLOT(show()));
    connect(mpSheetFinanceGrid, SIGNAL(focusOuted()), mpStatusBar, SLOT(clearMessage()));
    connect(mpToolHideCurrentCol, SIGNAL(triggered(bool)), mpSheetFinanceGrid, SLOT(hideCurrentCol()));
    connect(mpToolShowAllCols, SIGNAL(triggered(bool)), mpSheetFinanceGrid, SLOT(showHiddenCols()));

    mpAcMainPrint->setVisible(false);
    mpAcToolExport->setVisible(false);
    mpAcToolImport->setVisible(false);

    //工具箱

    //开关盒

    //布局
    mpLayBody->insertWidget(2, mpSheetFinanceGrid, 1);

    //初始
    openSheet(0);
}

BsSheetFinanceWin::~BsSheetFinanceWin()
{
    qDeleteAll(mGridFlds);
    mGridFlds.clear();
}

void BsSheetFinanceWin::doOpenQuery()
{
    QStringList sels;
    QStringList cons;

    sels << QStringLiteral("sheetid") << QStringLiteral("dated");
    cons << QStringLiteral("(dated BETWEEN %1 AND %2)")
            .arg(mpConDateB->mpEditor->getDataValue())
            .arg(mpConDateE->mpEditor->getDataValue());

    if ( mpConStype->mpEditor->getDataValue().isEmpty() )
        sels << QStringLiteral("stype");
    else
        cons << QStringLiteral("stype='%1'").arg(mpConStype->mpEditor->getDataValue());

    if ( mpConStaff->mpEditor->getDataValue().isEmpty() )
        sels << QStringLiteral("staff");
    else
        cons << QStringLiteral("staff='%1'").arg(mpConStaff->mpEditor->getDataValue());

    if ( mpConShop->mpEditor->getDataValue().isEmpty() )
        sels << QStringLiteral("shop");
    else
        cons << QStringLiteral("shop='%1'").arg(mpConShop->mpEditor->getDataValue());

    if ( mpConTrader->mpEditor->getDataValue().isEmpty() )
        sels << QStringLiteral("trader");
    else
        cons << QStringLiteral("trader='%1'").arg(mpConTrader->mpEditor->getDataValue());

    if ( !mpConCheck->getConExp().isEmpty() )
        cons << mpConCheck->getConExp();

    QStringList colTitles;
    colTitles << QStringLiteral("shop\t%1").arg(getFieldByName(QStringLiteral("shop"))->mFldCnName)
         << QStringLiteral("trader\t%1").arg(getFieldByName(QStringLiteral("trader"))->mFldCnName);

    QString sql = QStringLiteral("SELECT %1 FROM %2 WHERE %3 ORDER BY sheetid;")
            .arg(sels.join(QChar(44)))
            .arg(mMainTable)
            .arg(cons.join(QStringLiteral(" AND ")));

    //加载数据
    mpFindGrid->loadData(sql, colTitles);

    //加载列宽
    mpFindGrid->loadColWidths();
}

void BsSheetFinanceWin::doSyncFindGrid()
{
    int sheetIdCol = mpFindGrid->getColumnIndexByFieldName("sheetid");
    if ( sheetIdCol < 0 )
        return;

    int row = -1;
    for ( int i = 0, iLen = mpFindGrid->rowCount(); i < iLen; ++i ) {
        if ( mpFindGrid->item(i, sheetIdCol)->text().toInt() == mCurrentSheetId ) {
            row = i;
            break;
        }
    }
    if ( row < 0 )
        return;

    int datedCol = mpFindGrid->getColumnIndexByFieldName("dated");
    if ( datedCol > 0 )
        mpFindGrid->item(row, datedCol)->setText(getPrintValue("dated"));

    int stypeCol = mpFindGrid->getColumnIndexByFieldName("stype");
    if ( stypeCol > 0 )
        mpFindGrid->item(row, stypeCol)->setText(getPrintValue("stype"));

    int staffCol = mpFindGrid->getColumnIndexByFieldName("staff");
    if ( staffCol > 0 )
        mpFindGrid->item(row, staffCol)->setText(getPrintValue("staff"));

    int shopCol = mpFindGrid->getColumnIndexByFieldName("shop");
    if ( shopCol > 0 )
        mpFindGrid->item(row, shopCol)->setText(getPrintValue("shop"));

    int traderCol = mpFindGrid->getColumnIndexByFieldName("trader");
    if ( traderCol > 0 )
        mpFindGrid->item(row, traderCol)->setText(getPrintValue("trader"));

    if (mpCheckMark->getDataCheckedValue()) {
        mpFindGrid->item(row, sheetIdCol)->setData(Qt::DecorationRole, QIcon(":/icon/check.png"));
    } else {
        mpFindGrid->item(row, sheetIdCol)->setData(Qt::DecorationRole, QVariant());
    }
}

}
