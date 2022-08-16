#include "bslabeldesigner.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailigrid.h"
#include "main/bailiedit.h"
#include "comm/barlabel.h"
#include "comm/qrlabel.h"

#define FONT_ALIGN_LEFT     "靠左"
#define FONT_ALIGN_RIGHT    "靠右"
#define FONT_ALIGN_CENTER   "居中"

namespace BailiSoft {

// BsLabelDesigner ===========================================================================
BsLabelDesigner::BsLabelDesigner(QWidget *parent, const QString &winName) : QWidget(parent)
{
    setProperty(BSWIN_TABLE, winName);
    setWindowTitle(mapMsg.value(QStringLiteral("menu_label_designer")));
    setMinimumSize(1000, 600);

    //屏幕分辨率
    mScreenDpi = qApp->primaryScreen()->logicalDotsPerInch();  //经实量测试，不能用physicalDotsPerInch

    //工具栏
    mpComboPatterns = new QComboBox(this);
    mpComboPatterns->setFixedWidth(200);

    QToolBar *mainBar = new QToolBar(this);
    mainBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mainBar->setIconSize(QSize(16, 16));

    mpAcSelect = mainBar->addWidget(mpComboPatterns);
    mainBar->addWidget(new QLabel(QStringLiteral("\u3000\u3000")));
    mpAcNew     = mainBar->addAction(QIcon(":/icon/add.png"), QStringLiteral("新建样式"), this, &BsLabelDesigner::clickPatternNew);
    mpAcDel     = mainBar->addAction(QIcon(":/icon/del.png"), QStringLiteral("删除样式"), this, &BsLabelDesigner::clickPatternDel);
    mainBar->addWidget(new QLabel(QStringLiteral("\u3000\u3000")));
    mpAcSave    = mainBar->addAction(QIcon(":/icon/ok.png"), QStringLiteral("保存"), this, &BsLabelDesigner::clickEditSave);
    mpAcCancel  = mainBar->addAction(QIcon(":/icon/cancel.png"), QStringLiteral("取消"), this, &BsLabelDesigner::clickEditCancel);
    mainBar->addWidget(new QLabel(QStringLiteral("\u3000\u3000")));
    mpAcTest    = mainBar->addAction(QIcon(":/icon/print.png"), QStringLiteral("测试"), this, &BsLabelDesigner::clickEditTest);

    //主参数设置栏
    mpFldLabelName = new QLineEdit(this);
    mpFldLabelName->setPlaceholderText(QStringLiteral("用于支持多样式管理"));
    mpFldLabelName->setMaxLength(20);

    mpFldPrinterName = new QComboBox(this);

    mpFldFromX = new QSpinBox(this);
    mpFldFromX->setRange(0, 100);
    mpFldFromX->setSuffix(QStringLiteral(" 毫米"));

    mpFldFromY = new QSpinBox(this);
    mpFldFromY->setRange(0, 100);
    mpFldFromY->setSuffix(QStringLiteral(" 毫米"));

    mpFldUnitCount = new QSpinBox(this);
    mpFldUnitCount->setRange(1, 5);
    mpFldUnitCount->setSuffix(QStringLiteral(" 只"));

    mpFldUnitWidth = new QSpinBox(this);
    mpFldUnitWidth->setRange(10, 100);
    mpFldUnitWidth->setSuffix(QStringLiteral(" 毫米"));

    mpFldUnitHeight = new QSpinBox(this);
    mpFldUnitHeight->setRange(10, 100);
    mpFldUnitHeight->setSuffix(QStringLiteral(" 毫米"));

    mpFldSpaceX = new QSpinBox(this);
    mpFldSpaceX->setRange(0, 100);
    mpFldSpaceX->setSuffix(QStringLiteral(" 毫米"));

    mpFldSpaceY = new QSpinBox(this);
    mpFldSpaceY->setRange(0, 100);
    mpFldSpaceY->setSuffix(QStringLiteral(" 毫米"));

    mpFldFlowNum = new QLineEdit(this);
    mpFldFlowNum->setEnabled(false);

    mpFldCargoExp = new QLineEdit(this);
    mpFldCargoExp->setMaxLength(300);
    mpFldCargoExp->setPlaceholderText(QStringLiteral("正则表达式"));

    mpPnlParams = new QWidget(this);
    mpPnlParams->setFixedWidth(200);
    mpPnlParams->setStyleSheet(QLatin1String("QLabel{color:#888;}"));
    QVBoxLayout *layForm = new QVBoxLayout(mpPnlParams);
    layForm->setSpacing(0);
    layForm->addWidget(new QLabel(QStringLiteral("样式名："), this));
    layForm->addWidget(mpFldLabelName);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("使用打印机："), this));
    layForm->addWidget(mpFldPrinterName);



    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("基点偏移横向："), this));
    layForm->addWidget(mpFldFromX);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("基点偏移纵向："), this));
    layForm->addWidget(mpFldFromY);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("横向排列标签数："), this));
    layForm->addWidget(mpFldUnitCount);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("单标签宽："), this));
    layForm->addWidget(mpFldUnitWidth);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("单标签高："), this));
    layForm->addWidget(mpFldUnitHeight);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("标签之间横向间距："), this));
    layForm->addWidget(mpFldSpaceX);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("标签之间纵向间距："), this));
    layForm->addWidget(mpFldSpaceY);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("当前流水号："), this));
    layForm->addWidget(mpFldFlowNum);
    layForm->addSpacing(9);
    layForm->addWidget(new QLabel(QStringLiteral("筛选打印货号特征（选填）："), this));
    layForm->addWidget(mpFldCargoExp);
    layForm->addStretch();

    //样式设计区
    mpPnlPrinterPad = new QWidget(this);
    QPalette pal(palette());
    pal.setBrush(QPalette::Window, QColor("#999"));
    mpPnlPrinterPad->setAutoFillBackground(true);
    mpPnlPrinterPad->setPalette(pal);

    mpLayPrinterPad = new QVBoxLayout(mpPnlPrinterPad);
    mpLayPrinterPad->setContentsMargins(0, 0, 0, 0);

    mpLabelPage = new BsLabelPage(this);
    mpLayPrinterPad->addWidget(mpLabelPage, 1);

    //总布局
    QWidget* pnlWork = new QWidget(this);
    QHBoxLayout *layWork = new QHBoxLayout(pnlWork);
    layWork->setContentsMargins(0, 0, 0, 0);
    layWork->addWidget(mpPnlParams);
    layWork->addWidget(mpPnlPrinterPad, 1);

    QVBoxLayout *layWin = new QVBoxLayout(this);
    layWin->addWidget(mainBar);
    layWin->addWidget(pnlWork, 1);

    //非布局明细按钮
    QToolButton* btnAddText = new QToolButton(this);
    btnAddText->setIconSize(QSize(32, 32));
    btnAddText->setFixedSize(48, 48);
    btnAddText->setIcon(QIcon(":/icon/text.png"));
    btnAddText->setToolTip(QStringLiteral("添加静态文字"));
    connect(btnAddText, &QToolButton::clicked, mpLabelPage, &BsLabelPage::clickAddText);

    QToolButton* btnAddField = new QToolButton(this);
    btnAddField->setIconSize(QSize(32, 32));
    btnAddField->setFixedSize(48, 48);
    btnAddField->setIcon(QIcon(":/icon/field.png"));
    btnAddField->setToolTip(QStringLiteral("添加动态字段"));
    connect(btnAddField, &QToolButton::clicked, mpLabelPage, &BsLabelPage::clickAddField);

    QToolButton* btnAddBarcode = new QToolButton(this);
    btnAddBarcode->setIconSize(QSize(32, 32));
    btnAddBarcode->setFixedSize(48, 48);
    btnAddBarcode->setIcon(QIcon(":/icon/barcode.png"));
    btnAddBarcode->setToolTip(QStringLiteral("添加条形码"));
    connect(btnAddBarcode, &QToolButton::clicked, mpLabelPage, &BsLabelPage::clickAddBarcode);

    QToolButton* btnAddQrcode = new QToolButton(this);
    btnAddQrcode->setIconSize(QSize(32, 32));
    btnAddQrcode->setFixedSize(48, 48);
    btnAddQrcode->setIcon(QIcon(":/icon/qrcode.png"));
    btnAddQrcode->setToolTip(QStringLiteral("添加二维码"));
    connect(btnAddQrcode, &QToolButton::clicked, mpLabelPage, &BsLabelPage::clickAddQrcode);

    mpPnlButtons = new QWidget(this);
    QVBoxLayout *layButtons = new QVBoxLayout(mpPnlButtons);
    layButtons->addWidget(btnAddText);
    layButtons->addWidget(btnAddField);
    layButtons->addWidget(btnAddBarcode);
    layButtons->addWidget(btnAddQrcode);
    layButtons->addStretch();
    mpPnlButtons->hide();

    //初始化
    reloadPatternSelections();
    reloadPrinterSelections();
    openPattern(0);

    dsCargo->reload();
    dsColorList->reload();
    dsSizer->reload();

    connect(mpComboPatterns,    SIGNAL(currentIndexChanged(int)), this, SLOT(onPatternSelected(int)));
    connect(mpFldPrinterName,   SIGNAL(currentIndexChanged(int)), this, SLOT(onMainPrinterChanged(int)));
    connect(mpFldFromX,         SIGNAL(valueChanged(int)), this, SLOT(onMainValueChanged(int)));
    connect(mpFldFromY,         SIGNAL(valueChanged(int)), this, SLOT(onMainValueChanged(int)));
    connect(mpFldUnitCount,     SIGNAL(valueChanged(int)), this, SLOT(onMainValueChanged(int)));
    connect(mpFldUnitWidth,     SIGNAL(valueChanged(int)), this, SLOT(onMainValueChanged(int)));
    connect(mpFldUnitHeight,    SIGNAL(valueChanged(int)), this, SLOT(onMainValueChanged(int)));
    connect(mpFldSpaceX,        SIGNAL(valueChanged(int)), this, SLOT(onMainValueChanged(int)));
    connect(mpFldSpaceY,        SIGNAL(valueChanged(int)), this, SLOT(onMainValueChanged(int)));

    connect(mpFldLabelName, &QLineEdit::textEdited, this, &BsLabelDesigner::onMainTextChanged);
    connect(mpFldCargoExp,  &QLineEdit::textEdited, this, &BsLabelDesigner::onMainTextChanged);

    connect(mpLabelPage, &BsLabelPage::mouseClicked, this, &BsLabelDesigner::layoutDefineButtons);
}

void BsLabelDesigner::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    layoutDefineButtons();
}

void BsLabelDesigner::onPatternSelected(int idx)
{
    if ( isVisible() && idx >= 0 ) {
        openPattern(mpComboPatterns->itemData(idx).toInt());
    }
}

void BsLabelDesigner::onMainValueChanged(int val)
{
    Q_UNUSED(val);
    if ( mpPnlParams && mpPnlParams->isVisible() ) {
        mpLayPrinterPad->setContentsMargins(getPixelsByMMs(mpFldFromX->value()),
                                           getPixelsByMMs(mpFldFromY->value()),
                                           0,
                                           0);
        mpLabelPage->updateUnits(mCurrentId, sender() == mpFldUnitCount);
    }
    mIsDirty = true;
    updateActions();
}

void BsLabelDesigner::onMainPrinterChanged(int)
{
    mIsDirty = true;
    updateActions();
}

void BsLabelDesigner::onMainTextChanged()
{
    mIsDirty = true;
    updateActions();
}

void BsLabelDesigner::reloadPrinterSelections()
{
    mpFldPrinterName->clear();
    QList<QPrinterInfo> lstPrinterInfo = QPrinterInfo::availablePrinters();
    if ( lstPrinterInfo.size() > 0 ) {
        for ( int i = 0; i < lstPrinterInfo.size(); ++i ) {
            mpFldPrinterName->addItem(lstPrinterInfo.at(i).printerName());
        }
    }
    mpFldPrinterName->setCurrentIndex(-1);
}

void BsLabelDesigner::reloadPatternSelections()
{
    QObject::disconnect(mpComboPatterns, SIGNAL(currentIndexChanged(int)), 0, 0);

    mpComboPatterns->clear();
    QString sql = QStringLiteral("select labelid, labelname from lxlabel order by labelid;");
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    while ( qry.next() ) {
        mpComboPatterns->addItem(qry.value(1).toString(), qry.value(0));
    }
    qry.finish();
    mpComboPatterns->setCurrentIndex(-1);

    QObject::connect(mpComboPatterns,    SIGNAL(currentIndexChanged(int)), this, SLOT(onPatternSelected(int)));
}

void BsLabelDesigner::openPattern(const int labelId)
{
    //打开
    if ( labelId > 0 ) {
        QString sql = QStringLiteral("select labelname, printername, fromx, fromy, "
                                     "unitxcount, unitwidth, unitheight, "
                                     "spacex, spacey, flownum, cargoexp "
                                     "from lxlabel where labelid=%1;").arg(labelId);
        QSqlQuery qry;
        qry.setForwardOnly(true);
        qry.exec(sql);
        if ( qry.next() ) {
            mpFldLabelName->setText(qry.value(0).toString());
            mpFldPrinterName->setCurrentText(qry.value(1).toString());
            mpFldFromX->setValue(qry.value(2).toInt());
            mpFldFromY->setValue(qry.value(3).toInt());
            mpFldUnitCount->setValue(qry.value(4).toInt());
            mpFldUnitWidth->setValue(qry.value(5).toInt());
            mpFldUnitHeight->setValue(qry.value(6).toInt());
            mpFldSpaceX->setValue(qry.value(7).toInt());
            mpFldSpaceY->setValue(qry.value(8).toInt());
            mpFldFlowNum->setText(qry.value(9).toString());
            mpFldCargoExp->setText(qry.value(10).toString());
        }
        else {
            qDebug() << "BAD sql err:" << qry.lastError().text();
            openPattern(0);
        }
        qry.finish();
    }
    //新建
    else if ( labelId < 0 ) {
        mpFldLabelName->setText(QStringLiteral("新样式%1").arg(mpComboPatterns->count() + 1));
        mpFldPrinterName->setCurrentIndex(-1);
        mpFldFromX->setValue(0);
        mpFldFromY->setValue(0);
        mpFldUnitCount->setValue(1);
        mpFldUnitWidth->setValue(40);
        mpFldUnitHeight->setValue(30);
        mpFldSpaceX->setValue(5);
        mpFldSpaceY->setValue(5);
        mpFldFlowNum->setText(0);
        mpFldCargoExp->setText(QString());
    }
    //关闭
    else {
        mpFldLabelName->setText(QString());
        mpFldPrinterName->setCurrentIndex(-1);
        mpFldFromX->setValue(0);
        mpFldFromY->setValue(0);
        mpFldUnitCount->setValue(0);
        mpFldUnitWidth->setValue(0);
        mpFldUnitHeight->setValue(0);
        mpFldSpaceX->setValue(0);
        mpFldSpaceY->setValue(0);
        mpFldFlowNum->setText(0);
        mpFldCargoExp->setText(QString());

        mpComboPatterns->setCurrentIndex(-1);
    }

    //设计页
    mpLabelPage->openPattern(labelId);

    //状态特征
    mCurrentId = labelId;
    mIsDirty = (labelId < 0);

    //界面更新
    mpAcSelect->setVisible(mpComboPatterns->count() > 0 && mCurrentId >= 0);
    mpPnlParams->setVisible(labelId);
    mpPnlButtons->setVisible(labelId);
    updateActions();
}

void BsLabelDesigner::layoutDefineButtons()
{
    if ( mCurrentId ) {
        QPoint p = mpPnlPrinterPad->mapTo(this, QPoint(mpPnlPrinterPad->width(), 0));
        int w = mpPnlButtons->sizeHint().width();
        int h = mpPnlButtons->sizeHint().height();
        int x = p.x() - w;
        int y = p.y();
        mpPnlButtons->setGeometry(x, y, w, h);
    }
}

void BsLabelDesigner::updateActions()
{
    mpAcNew->setEnabled(!mIsDirty);
    mpAcDel->setEnabled(!mIsDirty && mCurrentId > 0);
    mpAcSave->setEnabled(mIsDirty);
    mpAcCancel->setEnabled(mIsDirty);
    mpAcTest->setEnabled(mpPnlParams->isVisible() && mpLabelPage->mDefines.length() > 0);
    layoutDefineButtons();
}

void BsLabelDesigner::resetPatternComboWithCurrent()
{
    int idx = -1;
    for ( int i = 0, iLen = mpComboPatterns->count(); i < iLen; ++i ) {
        if ( mpComboPatterns->itemData(i).toInt() == mCurrentId ) {
            idx = i;
            break;
        }
    }
    mpComboPatterns->setCurrentIndex(idx);
}

void BsLabelDesigner::clickPatternNew()
{
    openPattern(-1);
}

void BsLabelDesigner::clickPatternDel()
{
    QStringList sqls;
    sqls << QStringLiteral("delete from lxlabel where labelid=%1;").arg(mCurrentId);
    sqls << QStringLiteral("delete from lxlabelobj where nparentid=%1;").arg(mCurrentId);

    sqliteCommit(sqls);

    openPattern(0);

    mpComboPatterns->removeItem(mpComboPatterns->currentIndex());
}

void BsLabelDesigner::clickEditSave()
{
    QStringList sqls; 

    int savingId = mCurrentId;

    if ( mCurrentId > 0 ) {
        QStringList updExps;
        updExps << QStringLiteral("labelname='%1'").arg(mpFldLabelName->text())
                << QStringLiteral("printername='%1'").arg(mpFldPrinterName->currentText())
                << QStringLiteral("fromx=%1").arg(mpFldFromX->value())
                << QStringLiteral("fromy=%1").arg(mpFldFromY->value())
                << QStringLiteral("unitxcount=%1").arg(mpFldUnitCount->value())
                << QStringLiteral("unitwidth=%1").arg(mpFldUnitWidth->value())
                << QStringLiteral("unitheight=%1").arg(mpFldUnitHeight->value())
                << QStringLiteral("spacex=%1").arg(mpFldSpaceX->value())
                << QStringLiteral("spacey=%1").arg(mpFldSpaceY->value())
                << QStringLiteral("cargoexp='%1'").arg(mpFldCargoExp->text());
        QString updateSql = QStringLiteral("update lxlabel set %1 where labelid=%2")
                .arg(updExps.join(QChar(','))).arg(mCurrentId);

        sqls << updateSql;
        sqls << mpLabelPage->getSaveSqls(mCurrentId);
    }
    else {
        int newLabelId;
        QSqlQuery qry;
        qry.exec(QStringLiteral("SELECT seq FROM sqlite_sequence WHERE name='lxlabel';"));
        if ( qry.next() )
            newLabelId = qry.value(0).toInt() + 1;
        else
            newLabelId = 1;
        qry.finish();

        QString insertSql = QStringLiteral("insert into lxlabel(labelid, labelname, printername,"
                                           "fromx, fromy, unitxcount, unitwidth, unitheight,"
                                           "spacex, spacey, cargoexp) values(");
        insertSql += QStringLiteral("%1, '%2', '%3'")
                .arg(newLabelId)
                .arg(mpFldLabelName->text())
                .arg(mpFldPrinterName->currentText());
        insertSql += QStringLiteral(", %1, %2, %3, %4, %5, %6, %7, '%8');")
                .arg(mpFldFromX->value())
                .arg(mpFldFromY->value())
                .arg(mpFldUnitCount->value())
                .arg(mpFldUnitWidth->value())
                .arg(mpFldUnitHeight->value())
                .arg(mpFldSpaceX->value())
                .arg(mpFldSpaceY->value())
                .arg(mpFldCargoExp->text());

        sqls << insertSql;
        sqls << mpLabelPage->getSaveSqls(newLabelId);

        savingId = newLabelId;
    }

    QString err = sqliteCommit(sqls);
    if ( err.isEmpty() ) {
        mIsDirty = false;
        mCurrentId = savingId;
        updateActions();
        reloadPatternSelections();
        resetPatternComboWithCurrent();
    }
    else {
        QMessageBox::information(this, QString(), QStringLiteral("保存失败！"));
    }
}

void BsLabelDesigner::clickEditCancel()
{
    if ( mCurrentId > 0 ) {
        openPattern(mCurrentId);
    } else {
        openPattern(0);
    }
}

void BsLabelDesigner::clickEditTest()
{
    QDialog dlg(this);

    //标题
    QFont ft(font());
    ft.setPointSize(3 * ft.pointSize() / 2);
    ft.setBold(true);
    QLabel *lblTitle = new QLabel(QStringLiteral("选择测试数据"), &dlg);
    lblTitle->setFont(ft);

    //货号
    QStringList cargoDefs = mapMsg.value(QStringLiteral("fld_cargo")).split(QChar(9));
    Q_ASSERT(cargoDefs.count() > 4);
    BsField *fldCargo = new BsField(QStringLiteral("cargo"),
                                 cargoDefs.at(0),
                                 QString(cargoDefs.at(3)).toUInt(),
                                 QString(cargoDefs.at(4)).toInt(),
                                 cargoDefs.at(2));
    BsFldEditor *conCargo = new BsFldEditor(&dlg, fldCargo, dsCargo);
    conCargo->setMyPlaceText(fldCargo->mFldCnName);
    conCargo->setMyPlaceColor(QColor(Qt::gray));
    conCargo->setStyleSheet(QLatin1String("QLineEdit{color:#290;}"));
    conCargo->setMinimumWidth(240);

    //色号
    QComboBox *conColor = new QComboBox(this);

    //尺码
    QComboBox *conSizer = new QComboBox(this);

    //打印几排
    QString rowCheckText = (mpFldUnitCount->value() > 1)
            ? QStringLiteral("打印两行测试间距")
            : QStringLiteral("打印两只测试间距");
    QCheckBox *testTwoRow = new QCheckBox(rowCheckText, &dlg);

    //左区
    QVBoxLayout *layForm = new QVBoxLayout;
    layForm->setContentsMargins(0, 0, 0, 0);
    layForm->setSpacing(0);
    layForm->addWidget(lblTitle, 0, Qt::AlignCenter);
    layForm->addSpacing(20);
    layForm->addWidget(new QLabel(QStringLiteral("货号："), &dlg));
    layForm->addWidget(conCargo);
    layForm->addSpacing(10);
    layForm->addWidget(new QLabel(QStringLiteral("色号："), &dlg));
    layForm->addWidget(conColor);
    layForm->addSpacing(10);
    layForm->addWidget(new QLabel(QStringLiteral("尺码："), &dlg));
    layForm->addWidget(conSizer);
    layForm->addSpacing(10);
    layForm->addWidget(testTwoRow);
    layForm->addSpacing(20);

    //右区
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->setOrientation(Qt::Vertical);
    buttonBox->button(QDialogButtonBox::Ok)->setText(mapMsg.value("btn_ok"));
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    buttonBox->button(QDialogButtonBox::Cancel)->setText(mapMsg.value("btn_cancel"));
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    //总布局
    QHBoxLayout *layWin = new QHBoxLayout(&dlg);
    layWin->setContentsMargins(50, 20, 50, 20);
    layWin->setSpacing(50);
    layWin->addLayout(layForm, 1);
    layWin->addWidget(buttonBox);

    //事件
    QObject::connect(conCargo, QOverload<const QString&, const bool>::of(&BsFldEditor::inputEditing),
            [&](const QString &inputing, const bool validInPicks) {

        conColor->clear();
        conSizer->clear();

        if ( validInPicks ) {
            QString colorType = dsCargo->getValue(inputing, "colortype");
            QStringList colorList = dsColorList->getColorListByType(colorType);
            conColor->addItems(colorList);

            QString sizerType = dsCargo->getValue(inputing, QStringLiteral("sizertype"));
            QStringList sizerList = dsSizer->getSizerList(sizerType);
            conSizer->addItems(sizerList);
        }

        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(validInPicks && !inputing.isEmpty());
    });

    QObject::connect(&dlg, &QDialog::destroyed, [&] {
        delete fldCargo;
    });

    //执行
    dlg.setMinimumSize(dlg.sizeHint());
    dlg.setWindowFlags(dlg.windowFlags() &~ Qt::WindowContextHelpButtonHint);
    if ( dlg.exec() != QDialog::Accepted ) {
        return;
    }

    //测试输出
    BsLabelPrinter::doPrintTest(
                mpFldPrinterName->currentText(),
                mpFldFromX->value(),
                mpFldFromY->value(),
                mpFldUnitCount->value(),
                mpFldUnitWidth->value(),
                mpFldUnitHeight->value(),
                mpFldSpaceX->value(),
                mpFldSpaceY->value(),
                mpFldFlowNum->text().toInt(),
                mpLabelPage->mDefines,
                conCargo->getDataValue(),
                conColor->currentText(),
                conSizer->currentText(),
                testTwoRow->isChecked(),
                this
                );
}


// BsLabelPage ================================================================================
BsLabelPage::BsLabelPage(BsLabelDesigner *parent) : QWidget(parent), mppDesigner(parent)
{
    QPalette pal(palette());
    pal.setBrush(QPalette::Window, QColor("#cef"));
    setAutoFillBackground(true);
    setPalette(pal);

    mpLayRow = new QHBoxLayout;
    mpLayRow->setContentsMargins(0, 0, 0, 0);
    mpLayRow->setSpacing(0);
    mpLayRow->addStretch();

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addLayout(mpLayRow, 0);
    lay->addStretch();
}

void BsLabelPage::openPattern(const int labelId)
{
    qDeleteAll(mDefines);
    mDefines.clear();

    //打开
    if ( labelId > 0 ) {
        QString sql = QStringLiteral("select nid, nobjtype, svalue, sexp, nposx, nposy, "
                                     "nwidth, nheight, sfontname, nfontpoint, nfontalign "
                                     "from lxlabelobj where nparentid=%1 "
                                     "order by nposy, nposx;").arg(labelId);
        QSqlQuery qry;
        qry.setForwardOnly(true);
        qry.exec(sql);
        if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << sql;
        while ( qry.next() ) {
            BsLabelDef *def = new BsLabelDef(
                        qry.value(0).toInt(),
                        qry.value(1).toInt(),
                        qry.value(2).toString(),
                        qry.value(3).toString(),
                        qry.value(4).toInt(),
                        qry.value(5).toInt(),
                        qry.value(6).toInt(),
                        qry.value(7).toInt(),
                        qry.value(8).toString(),
                        qry.value(9).toInt(),
                        qry.value(10).toInt()
                        );
            mDefines << def;
        }
        qry.finish();
    }

    //更新内容
    updateUnits(labelId, true);

    //状态特征
    mCurrentId = labelId;
    mIsDirty = (labelId < 0);
}

QStringList BsLabelPage::getSaveSqls(const int saveId)
{
    QStringList sqls;

    sqls << QStringLiteral("delete from lxlabelobj where nparentid=%1;").arg(saveId);

    for ( int i = 0, iLen = mDefines.length(); i < iLen; ++i ) {

        BsLabelDef *def = mDefines.at(i);

        QString sql = QStringLiteral("insert into lxlabelobj(nparentid, nobjtype, svalue, sexp, "
                                     "nposx, nposy, nwidth, nheight, sfontname, nfontpoint, nfontalign) "
                                     "values(");

        sql += QStringLiteral("%1, %2, '%3', '%4'")
                .arg(saveId)
                .arg(def->nObjType)
                .arg(def->sValue)
                .arg(def->sExp);

        sql += QStringLiteral(", %1, %2, %3, %4, '%5', %6, %7);")
                .arg(def->nPosX)
                .arg(def->nPosY)
                .arg(def->nWidth)
                .arg(def->nHeight)
                .arg(def->sFontName)
                .arg(def->nFontPoint)
                .arg(def->nFontAlign);

        sqls << sql;
    }

    return sqls;
}

void BsLabelPage::updateUnits(const int labelId, const bool recreate)  //BY: openPattern()  onMainValueChanged()
{
    //清空重建
    if ( recreate ) {
        qDeleteAll(mUnits);
        mUnits.clear();

        if ( labelId ) {
            for ( int i = 0, iLen = mppDesigner->mpFldUnitCount->value(); i < iLen; ++i ) {
                BsLabelWinUnit *unit = new BsLabelWinUnit(this);
                unit->createParts();
                mUnits << unit;
                mpLayRow->insertWidget( mpLayRow->count() - 1, unit);
            }
        }
    }

    //布局
    mpLayRow->setSpacing(mppDesigner->getPixelsByMMs(mppDesigner->mpFldSpaceX->value()));
    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->setFixedSize(mppDesigner->getPixelsByMMs(mppDesigner->mpFldUnitWidth->value()),
                           mppDesigner->getPixelsByMMs(mppDesigner->mpFldUnitHeight->value()));
    }
}

void BsLabelPage::clickAddText()
{
    BsLabelDefineDlg dlg(this, 0, nullptr);
    if ( dlg.exec() != QDialog::Accepted ) return;

    BsLabelDef *def = new BsLabelDef(
                -1,                                 //nid
                0,                                  //nobjtype
                dlg.mpEdtValue->currentText(),      //svalue
                dlg.getExp(),                       //sexp
                dlg.mpEdtPosX->value(),             //nposx
                dlg.mpEdtPosY->value(),             //nposy
                dlg.mpEdtWidth->value(),            //nwidth
                0,                                  //nheight
                dlg.mpEdtFontName->currentText(),   //sfontname
                dlg.mpEdtFontPoint->value(),        //nfontpoint
                dlg.mpEdtFontAlign->currentData().toInt()   //nfontalign
                );
    mDefines << def;

    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->addPartByDef(def);
    }
    mppDesigner->setDirty(true);
}

void BsLabelPage::clickAddField()
{
    BsLabelDefineDlg dlg(this, 1, nullptr);
    if ( dlg.exec() != QDialog::Accepted ) return;

    BsLabelDef *def = new BsLabelDef(
                -1,                                 //nid
                1,                                  //nobjtype
                dlg.mpEdtValue->currentText(),      //svalue
                dlg.getExp(),                       //sexp
                dlg.mpEdtPosX->value(),             //nposx
                dlg.mpEdtPosY->value(),             //nposy
                dlg.mpEdtWidth->value(),            //nwidth
                0,                                  //nheight
                dlg.mpEdtFontName->currentText(),   //sfontname
                dlg.mpEdtFontPoint->value(),        //nfontpoint
                dlg.mpEdtFontAlign->currentData().toInt()   //nfontalign
                );
    mDefines << def;

    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->addPartByDef(def);
    }
    mppDesigner->setDirty(true);
}

void BsLabelPage::clickAddBarcode()
{
    bool hasAdded = false;
    for ( int i = 0, iLen = mDefines.length(); i < iLen; ++i ) {
        if ( mDefines.at(i)->nObjType == 2 ) {
            hasAdded = true;
            break;
        }
    }
    if ( hasAdded ) {
        QMessageBox::information(this, QString(), QStringLiteral("标签内已有条形码！"));
        return;
    }

    BsLabelDefineDlg dlg(this, 2, nullptr);
    if ( dlg.exec() != QDialog::Accepted ) return;

    BsLabelDef *def = new BsLabelDef(
                -1,                                 //nid
                2,                                  //nobjtype
                dlg.mpEdtValue->currentText(),      //svalue
                dlg.getExp(),                       //sexp
                dlg.mpEdtPosX->value(),             //nposx
                dlg.mpEdtPosY->value(),             //nposy
                dlg.mpEdtWidth->value(),            //nwidth
                dlg.mpEdtHeight->value(),           //nheight
                QString(),                          //sfontname
                dlg.mpEdtFontPoint->value(),        //nfontpoint  借用为流水号长
                0                                   //nfontalign
                );
    mDefines << def;

    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->addPartByDef(def);
    }
    mppDesigner->setDirty(true);
}

void BsLabelPage::clickAddQrcode()
{
    bool hasAdded = false;
    for ( int i = 0, iLen = mDefines.length(); i < iLen; ++i ) {
        if ( mDefines.at(i)->nObjType == 3 ) {
            hasAdded = true;
            break;
        }
    }
    if ( hasAdded ) {
        QMessageBox::information(this, QString(), QStringLiteral("标签内已有二维码！"));
        return;
    }

    BsLabelDefineDlg dlg(this, 3, nullptr);
    if ( dlg.exec() != QDialog::Accepted ) return;

    BsLabelDef *def = new BsLabelDef(
                -1,                                 //nid
                3,                                  //nobjtype
                dlg.mpEdtValue->currentText(),      //svalue
                dlg.getExp(),                       //sexp
                dlg.mpEdtPosX->value(),             //nposx
                dlg.mpEdtPosY->value(),             //nposy
                dlg.mpEdtWidth->value(),            //nwidth
                0,                                  //nheight
                QString(),                          //sfontname
                0,                                  //nfontpoint
                0                                   //nfontalign
                );
    mDefines << def;

    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->addPartByDef(def);
    }
    mppDesigner->setDirty(true);
}

void BsLabelPage::onDeleteDef()
{
    BsLabelPart *part = qobject_cast<BsLabelPart*>(sender());
    if ( !part ) return;

    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->delPartByDef(part->mppDefine);
    }

    BsLabelDef *def = part->mppDefine;
    mDefines.removeOne(def);
    delete def;
    mppDesigner->setDirty(true);
}

void BsLabelPage::onUpdateDef()
{
    BsLabelPart *part = qobject_cast<BsLabelPart*>(sender());
    if ( !part ) return;

    BsLabelDef *def = part->mppDefine;

    //对话框
    BsLabelDefineDlg dlg(this, part->mppDefine->nObjType, part->mppDefine);
    if ( dlg.exec() != QDialog::Accepted ) return;

    def->sValue = dlg.mpEdtValue->currentText();
    def->sExp = dlg.getExp();
    def->nPosX = dlg.mpEdtPosX->value();
    def->nPosY = dlg.mpEdtPosY->value();
    def->nWidth = dlg.mpEdtWidth->value();

    if ( def->nObjType < 2 ) {
        def->sFontName = dlg.mpEdtFontName->currentText();
        def->nFontPoint = dlg.mpEdtFontPoint->value();
        def->nFontAlign = dlg.mpEdtFontAlign->currentData().toInt();
    }

    if ( def->nObjType == 2 ) {
        def->nHeight = dlg.mpEdtHeight->value();
        def->nFontPoint = dlg.mpEdtFontPoint->value();          //借用为流水号长
    }

    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->updPartByDef(def);
    }
    mppDesigner->setDirty(true);
}

void BsLabelPage::onDraggedDef()
{
    for ( int i = 0, iLen = mUnits.length(); i < iLen; ++i ) {
        BsLabelWinUnit *unit = mUnits.at(i);
        unit->layoutParts();
    }
    mppDesigner->setDirty(true);
}


// BsLabelWinUnit ================================================================================
BsLabelWinUnit::BsLabelWinUnit(BsLabelPage *parent) : QWidget(parent), mppPage(parent)
{
    QPalette pal(palette());
    pal.setBrush(QPalette::Window, QColor("#fff"));
    setAutoFillBackground(true);
    setPalette(pal);
}

BsLabelWinUnit::~BsLabelWinUnit()
{
    qDeleteAll(mParts);
    mParts.clear();
}

void BsLabelWinUnit::createParts()  //BY: updateUnits()
{
    //清空
    qDeleteAll(mParts);
    mParts.clear();

    //重建
    for ( int i = 0, iLen = mppPage->mDefines.length(); i < iLen; ++i ) {
        createOnePart(mppPage->mDefines.at(i));
    }

    //排版
    layoutParts();
}

void BsLabelWinUnit::layoutParts()
{
    for ( int i = 0, iLen = mParts.length(); i < iLen; ++i ) {
        BsLabelPart *part = mParts.at(i);
        int x = mppPage->mppDesigner->getPixelsByMMs(part->mppDefine->nPosX);
        int y = mppPage->mppDesigner->getPixelsByMMs(part->mppDefine->nPosY);
        int w = mppPage->mppDesigner->getPixelsByMMs(part->mppDefine->nWidth);
        int h = ( part->mppDefine->nObjType > 1 )
                ? mppPage->mppDesigner->getPixelsByMMs(part->mppDefine->nHeight)
                : part->sizeHint().height();
        part->setGeometry(x, y, w, h);
    }
}

void BsLabelWinUnit::addPartByDef(BsLabelDef *def)
{
    createOnePart(def);
    layoutParts();
}

void BsLabelWinUnit::delPartByDef(BsLabelDef *def)
{
    for ( int i = mParts.length() - 1; i >= 0; i-- ) {
        BsLabelPart *part = mParts.at(i);
        if ( part->mppDefine == def ) {
            mParts.removeOne(part);
            delete part;
        }
    }
}

void BsLabelWinUnit::updPartByDef(BsLabelDef *def)
{
    for ( int i = 0, iLen = mParts.length(); i < iLen; ++i ) {
        BsLabelPart *part = mParts.at(i);
        if ( part->mppDefine == def ) {
            part->setContent(def);
        }
    }
    layoutParts();
}

void BsLabelWinUnit::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    layoutParts();
}

void BsLabelWinUnit::enterEvent(QEvent *e)
{
    QWidget::enterEvent(e);
    for ( int i = 0, iLen = mParts.length(); i < iLen; ++i ) {
        BsLabelPart *part = mParts.at(i);
        part->setStyleSheet(QLatin1String("background-color:#ffa;"));
    }
}

void BsLabelWinUnit::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    for ( int i = 0, iLen = mParts.length(); i < iLen; ++i ) {
        BsLabelPart *part = mParts.at(i);
        part->setStyleSheet(QString());
    }
}

void BsLabelWinUnit::createOnePart(BsLabelDef *def)
{
    BsLabelPart *part = new BsLabelPart(this, def);
    connect(part, &BsLabelPart::reqDeleteDef,   mppPage, &BsLabelPage::onDeleteDef);
    connect(part, &BsLabelPart::reqUpdateDef,   mppPage, &BsLabelPage::onUpdateDef);
    connect(part, &BsLabelPart::reqDragMoveDef, mppPage, &BsLabelPage::onDraggedDef);
    mParts << part;

    part->raise();
    part->show();
}

/*
"    nId           integer primary key autoincrement, "
"    nParentid     integer not null, "
"    nObjType      integer, "  //0固定文字         1动态字段       2条形码        3二维码
"    sValue        text, "     //固定文字TEXT      =字段中文       [条形码]       [字段]文字...
"    sExp			text, "    //NULL              fieldName      NULL           [fields]text...
"    nPosX			integer, "
"    nPosY			integer, "
"    nWidth        integer, "
"    nHeight       integer, "
"    sFontName		text, "
"    nFontPoint    integer, "
"    nFontAlign    integer "
*/

// BsLabelDefineDlg ==============================================================================
BsLabelDefineDlg::BsLabelDefineDlg(QWidget *parent, const int defType, BsLabelDef *editDef)
    : QDialog(parent), mDefType(defType), mppDefine(editDef)
{
    //系统字体
    QFontDatabase fdb;
    QStringList sysFonts = fdb.families(QFontDatabase::Any);
    int fontIndex = sysFonts.indexOf(font().family());

    //定义名称
    QString dtypeName;

    //内容
    mpEdtValue = new QComboBox;  //后面分别设置

    //距左
    mpEdtPosX = new QSpinBox;
    mpEdtPosX->setSuffix(QStringLiteral("毫米"));
    mpEdtPosX->setMinimum(0);
    mpEdtPosX->setMaximum(200);
    mpEdtPosX->setValue(0);
    if ( mppDefine ) {
        mpEdtPosX->setValue(mppDefine->nPosX);
    }

    //距顶
    mpEdtPosY = new QSpinBox;
    mpEdtPosY->setSuffix(QStringLiteral("毫米"));
    mpEdtPosY->setMinimum(0);
    mpEdtPosY->setMaximum(200);
    mpEdtPosY->setValue(0);
    if ( mppDefine ) {
        mpEdtPosY->setValue(mppDefine->nPosY);
    }

    //宽度
    mpEdtWidth = new QSpinBox;
    mpEdtWidth->setSuffix(QStringLiteral("毫米"));
    mpEdtWidth->setMinimum(1);
    mpEdtWidth->setMaximum(200);
    mpEdtWidth->setValue((defType > 1) ? 40 : 20);
    if ( mppDefine ) {
        mpEdtWidth->setValue(mppDefine->nWidth);
    }

    //布局
    mpLayForm = new QFormLayout;
    mpLayForm->addRow(QStringLiteral("内容"), mpEdtValue);
    mpLayForm->addRow(QStringLiteral("距左"), mpEdtPosX);
    mpLayForm->addRow(QStringLiteral("距顶"), mpEdtPosY);
    mpLayForm->addRow(QStringLiteral("宽度"), mpEdtWidth);

    //----静态文字
    if ( defType == 0 ) {
        dtypeName = QStringLiteral("静态文字");

        mpEdtValue->setEditable(true);
        mpEdtValue->lineEdit()->setPlaceholderText(QStringLiteral("直接填写"));
        if ( mppDefine ) {
            mpEdtValue->setCurrentText(mppDefine->sValue);
            mpEdtValue->lineEdit()->setText(mppDefine->sValue);
        }
        connect(mpEdtValue->lineEdit(), &QLineEdit::textEdited, this, &BsLabelDefineDlg::onValueTextEdited);

        mpEdtFontName = new QComboBox;
        mpEdtFontName->addItems(sysFonts);
        mpEdtFontName->setCurrentIndex(fontIndex);
        mpLayForm->addRow(QStringLiteral("字体"), mpEdtFontName);
        if ( mppDefine ) {
            mpEdtFontName->setCurrentText(mppDefine->sFontName);
        } else {
            mpEdtFontName->setCurrentIndex(fontIndex);
        }

        mpEdtFontPoint = new QSpinBox;
        mpEdtFontPoint->setSuffix(QStringLiteral("点"));
        mpEdtFontPoint->setMinimum(5);
        mpEdtFontPoint->setMaximum(60);
        mpEdtFontPoint->setValue(10);
        mpLayForm->addRow(QStringLiteral("字大"), mpEdtFontPoint);
        if ( mppDefine ) {
            mpEdtFontPoint->setValue(mppDefine->nFontPoint);
        }

        mpEdtFontAlign = new QComboBox;
        mpEdtFontAlign->addItem(QStringLiteral(FONT_ALIGN_LEFT), Qt::AlignLeft);
        mpEdtFontAlign->addItem(QStringLiteral(FONT_ALIGN_RIGHT), Qt::AlignRight);
        mpEdtFontAlign->addItem(QStringLiteral(FONT_ALIGN_CENTER), Qt::AlignHCenter);
        mpEdtFontAlign->setCurrentIndex(0);
        mpLayForm->addRow(QStringLiteral("字齐"), mpEdtFontAlign);
        if ( mppDefine ) {
            if ( Qt::AlignRight == mppDefine->nFontAlign) {
                mpEdtFontAlign->setCurrentIndex(1);
            } else if ( Qt::AlignHCenter == mppDefine->nFontAlign) {
                mpEdtFontAlign->setCurrentIndex(2);
            }  else {
                mpEdtFontAlign->setCurrentIndex(0);
            }
        }
    }

    //----动态字段
    if ( defType == 1 ) {
        dtypeName = QStringLiteral("动态字段");

        mpEdtValue->setEditable(false);
        QList<QPair<QString, QString> > flds = BsLabelPrinter::getCargoFieldsList();
        for ( int i = 0, iLen = flds.length(); i < iLen; ++i ) {
            mpEdtValue->addItem(QStringLiteral("=%1").arg(flds.at(i).first), flds.at(i).second);
        }
        if ( mppDefine ) {
            mpEdtValue->setCurrentText(mppDefine->sValue);
        } else {
            mpEdtValue->setCurrentIndex(-1);
        }
        connect(mpEdtValue, SIGNAL(currentIndexChanged(int)), this, SLOT(onValueIndexChanged(int)));

        mpEdtFontName = new QComboBox;
        mpEdtFontName->addItems(sysFonts);
        mpEdtFontName->setCurrentIndex(fontIndex);
        mpLayForm->addRow(QStringLiteral("字体"), mpEdtFontName);
        if ( mppDefine ) {
            mpEdtFontName->setCurrentText(mppDefine->sFontName);
        } else {
            mpEdtFontName->setCurrentIndex(fontIndex);
        }

        mpEdtFontPoint = new QSpinBox;
        mpEdtFontPoint->setSuffix(QStringLiteral("点"));
        mpEdtFontPoint->setMinimum(5);
        mpEdtFontPoint->setMaximum(60);
        mpEdtFontPoint->setValue(10);
        mpLayForm->addRow(QStringLiteral("字大"), mpEdtFontPoint);
        if ( mppDefine ) {
            mpEdtFontPoint->setValue(mppDefine->nFontPoint);
        }

        mpEdtFontAlign = new QComboBox;
        mpEdtFontAlign->addItem(QStringLiteral(FONT_ALIGN_LEFT), Qt::AlignLeft);
        mpEdtFontAlign->addItem(QStringLiteral(FONT_ALIGN_RIGHT), Qt::AlignRight);
        mpEdtFontAlign->addItem(QStringLiteral(FONT_ALIGN_CENTER), Qt::AlignHCenter);
        mpEdtFontAlign->setCurrentIndex(0);
        mpLayForm->addRow(QStringLiteral("字齐"), mpEdtFontAlign);
        if ( mppDefine ) {
            if ( Qt::AlignRight == mppDefine->nFontAlign) {
                mpEdtFontAlign->setCurrentIndex(1);
            } else if ( Qt::AlignHCenter == mppDefine->nFontAlign) {
                mpEdtFontAlign->setCurrentIndex(2);
            }  else {
                mpEdtFontAlign->setCurrentIndex(0);
            }
        }
    }

    //----条形码
    if ( defType == 2 ) {
        dtypeName = QStringLiteral("条形码");

        mpEdtValue->setEditable(true);
        mpEdtValue->lineEdit()->setPlaceholderText(QStringLiteral("鼠标停留看提示"));
        mpEdtValue->setToolTip(QStringLiteral("[K]表示货号\n"
                                              "[S]表示色号\n"
                                              "[M]表示尺码\n"
                                              "[J]表示价格\n"
                                              "[L]表示流水号\n"
                                              "注意包括方括号\n"
                                              "分隔符与固定前后缀直接写"));
        if ( mppDefine ) {
            mpEdtValue->setCurrentText(mppDefine->sValue);
        }
        connect(mpEdtValue->lineEdit(), &QLineEdit::textEdited, this, &BsLabelDefineDlg::onValueTextEdited);

        mpEdtHeight = new QSpinBox;
        mpEdtHeight->setSuffix(QStringLiteral("毫米"));
        mpEdtHeight->setMinimum(5);
        mpEdtHeight->setMaximum(100);
        mpEdtHeight->setValue(20);
        mpLayForm->addRow(QStringLiteral("高度"), mpEdtHeight);
        if ( mppDefine ) {
            mpEdtHeight->setValue(mppDefine->nHeight);
        }

        mpEdtFontPoint = new QSpinBox;
        mpEdtFontPoint->setSuffix(QStringLiteral("位"));
        mpEdtFontPoint->setMinimum(0);
        mpEdtFontPoint->setMaximum(6);
        mpEdtFontPoint->setValue(0);
        mpLayForm->addRow(QStringLiteral("流水号长"), mpEdtFontPoint);  //借用
        if ( mppDefine ) {
            mpEdtFontPoint->setValue(mppDefine->nFontPoint);
        }
    }

    //----二维码
    if ( defType == 3 ) {
        dtypeName = QStringLiteral("二维码");

        mpEdtValue->setEditable(true);
        mpEdtValue->lineEdit()->setPlaceholderText(QStringLiteral("鼠标停留看提示"));
        mpEdtValue->setToolTip(QStringLiteral("动态货品字段使用半角方括号括起来，具体有：\n"
                                              "[货号]、[色号]、[尺码]、[品名]、[单位]、\n"
                                              "[标牌价]、[进货价]、[批发价]、[零售价]"
                                              "静态文字直接写。"));
        if ( mppDefine ) {
            mpEdtValue->setCurrentText(mppDefine->sValue);
            mpEdtValue->lineEdit()->setText(mppDefine->sValue);
        }
        connect(mpEdtValue->lineEdit(), &QLineEdit::textEdited, this, &BsLabelDefineDlg::onValueTextEdited);

        mpEdtWidth->setValue(40);

        QLabel *label = qobject_cast<QLabel*>(mpLayForm->labelForField(mpEdtWidth));
        if ( label ) label->setText(QStringLiteral("大小"));
    }

    //确定取消
    QDialogButtonBox *pBox = new QDialogButtonBox(this);
    pBox->setOrientation(Qt::Horizontal);

    mpBtnOk = pBox->addButton(mapMsg.value("btn_ok"), QDialogButtonBox::AcceptRole);
    mpBtnOk->setIcon(QIcon(":/icon/ok.png"));
    mpBtnOk->setIconSize(QSize(20, 20));
    connect(mpBtnOk, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *pBtnCancel = pBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setIcon(QIcon(":/icon/cancel.png"));
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    //标题
    QFont ft(font());
    ft.setPointSize( 3 * ft.pointSize() / 2);
    ft.setBold(true);
    QString actName = ( mppDefine ) ? QStringLiteral("设置改动") : QStringLiteral("添加");
    QLabel* lblTitle = new QLabel(this);
    lblTitle->setFont(ft);
    lblTitle->setText(QStringLiteral("%1 %2").arg(actName).arg(dtypeName));

    //总布局
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(lblTitle, 0, Qt::AlignCenter);
    lay->addLayout(mpLayForm, 1);
    lay->addWidget(pBox, 0, Qt::AlignCenter);

    //编辑态禁止改动内容（用户可删除后重新创建）
    if ( mppDefine ) {
        mpEdtValue->setEnabled(false);
    }
    //创建态按钮须验证激活
    else {
        mpBtnOk->setEnabled(defType == 2);
    }

    setMinimumSize(sizeHint());
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
}

QString BsLabelDefineDlg::getExp() const
{
    if ( mDefType == 1 ) {
        return BsLabelPrinter::getFieldOf(mpEdtValue->currentText().mid(1));
    }

    if ( mDefType == 3 ) {
        QString textValue = mpEdtValue->currentText();

        QList<QPair<QString, QString> > flds = BsLabelPrinter::getCargoFieldsList();
        for ( int i = 0, iLen = flds.length(); i < iLen; ++i ) {
            QString namer = QStringLiteral("[%1]").arg(flds.at(i).first);
            QString exper = QStringLiteral("[%1]").arg(flds.at(i).second);
            textValue.replace(namer, exper);
        }

        return textValue;
    }

    return QString();
}

void BsLabelDefineDlg::onValueIndexChanged(int index)
{
    mpBtnOk->setEnabled(index >= 0);
}

void BsLabelDefineDlg::onValueTextEdited(const QString &text)
{
    mpBtnOk->setEnabled(!text.trimmed().isEmpty());
}


// BsLabelPart ===================================================================================
BsLabelPart::BsLabelPart(QWidget *parent, BsLabelDef *def)
    : QLabel(parent), mppParent(parent), mppDefine(def)
{
    Q_ASSERT(def != nullptr);

    setContent(def);

    mpMenu = new QMenu(this);
    mpMenu->addAction(QStringLiteral("修改"), this, &BsLabelPart::clickUpdate);
    mpMenu->addAction(QStringLiteral("删除"), this, &BsLabelPart::clickDelete);
}

void BsLabelPart::setContent(const BsLabelDef *def)
{
    qreal screenDpi = qApp->primaryScreen()->logicalDotsPerInch();
    int w = int((def->nWidth * screenDpi) / 25.4);

    if ( def->nObjType < 2 ) {
        setText(def->sValue);
        QFont ft;
        ft.setFamily(def->sFontName);
        ft.setPointSize(def->nFontPoint);
        setFont(ft);
        setAlignment(Qt::Alignment(def->nFontAlign));
        setFixedWidth(w);
    }
    else if ( def->nObjType == 2 ) {
        int h = int((def->nHeight * screenDpi) / 25.4);
        //QString example = def->sValue;
        //example.replace(QStringLiteral("["), QString()).replace(QStringLiteral("]"), QString());
        QString example = ( def->nWidth >= 50 )
                ? QStringLiteral("bailisoft.com")
                : QStringLiteral("bailisoft");
        setPixmap(BarLabel::generatePixmap(example, w, h, true));
        setFixedSize(w + 10, h + 10);     //留5像素空边，用于焦点识别
        setAlignment(Qt::AlignCenter);
    }
    else {
        setPixmap(QrLabel::generatePixmap(def->sValue, w));
        setFixedSize(w, w);  //二维码图一般不太可能正好与此尺寸一样的，故而会有空边
        setAlignment(Qt::AlignCenter);
    }
}

void BsLabelPart::mousePressEvent(QMouseEvent *event)
{
    mMoved = false;
    if ( event->button() == Qt::LeftButton )
    {
        mPtStart = event->pos();
    }
    QLabel::mousePressEvent(event);
}

void BsLabelPart::mouseMoveEvent(QMouseEvent *event)
{
    if ( event->buttons() & Qt::LeftButton )
    {
        if ( (event->pos() - mPtStart).manhattanLength() > 3 )
        {
            QPoint ptMove = mapToParent(event->pos() - mPtStart);
            int leftTo  = rect().topLeft().x() + ptMove.x();
            int rightTo = rect().topRight().x() + ptMove.x();
            int topTo   = rect().topLeft().y() + ptMove.y();
            int botTo   = rect().bottomLeft().y() + ptMove.y();

            QRect rThis = QRect( mapToParent(rect().topLeft()), mapToParent(rect().bottomRight()) );
            QRect rOuter = mppParent->rect().united(rThis);
            if ( leftTo >= rOuter.left() && topTo >= rOuter.top() &&
                 rightTo <= rOuter.right() && botTo <= rOuter.bottom() )
            {
                if ( abs(ptMove.x()) < 8 ) ptMove.setX(0);
                if ( abs(ptMove.y()) < 8 ) ptMove.setY(0);

                move(ptMove);
            }

            mMoved = true;
            return;
        }
    }
    QLabel::mouseMoveEvent(event);
}

void BsLabelPart::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->button() == Qt::LeftButton )
    {
        if ( mMoved ) {
            qreal screenDpi = qApp->primaryScreen()->logicalDotsPerInch();  //经实量测试，不能用physicalDotsPerInch
            mppDefine->nPosX = int(25.4 * (geometry().left() / screenDpi));
            mppDefine->nPosY = int(25.4 * (geometry().top() / screenDpi));
            emit reqDragMoveDef();
        } else {
            mpMenu->popup( mapToGlobal(event->pos()) );
        }
    }
    mMoved = false;
    QLabel::mouseReleaseEvent(event);
}

void BsLabelPart::clickDelete()
{
    emit reqDeleteDef();
}

void BsLabelPart::clickUpdate()
{
    emit reqUpdateDef();
}

}
