#include "baililabel.h"
#include "bailicode.h"
#include "bailifunc.h"
#include "bailidata.h"
#include "bailiedit.h"
#include "bailigrid.h"
#include "comm/barlabel.h"
#include "comm/qrlabel.h"

#define PRINTER_LOST        "没找到指定打印机："
#define PAPER_LOST          "没找到指定类型纸张："
#define FOUNDNOT_SETTINGS   "本单尚未进行打印设置！"

namespace BailiSoft {

BsLabelPrinter* BsLabelPrinter::instance = nullptr;

BsLabelPrinter::BsLabelPrinter()
{
    mCargoFields << qMakePair(mapMsg.value("fld_cargo").split(QChar('\t')).at(0), QStringLiteral("cargo"))
                 << qMakePair(mapMsg.value("fld_color").split(QChar('\t')).at(0), QStringLiteral("color"))
                 << qMakePair(mapMsg.value("fld_sizer").split(QChar('\t')).at(0), QStringLiteral("sizer"))
                 << qMakePair(mapMsg.value("fld_hpname").split(QChar('\t')).at(0), QStringLiteral("hpname"))
                 << qMakePair(mapMsg.value("fld_unit").split(QChar('\t')).at(0), QStringLiteral("unit"))
                 << qMakePair(mapMsg.value("fld_setprice").split(QChar('\t')).at(0), QStringLiteral("setprice"))
                 << qMakePair(mapMsg.value("fld_lotprice").split(QChar('\t')).at(0), QStringLiteral("lotprice"))
                 << qMakePair(mapMsg.value("fld_retprice").split(QChar('\t')).at(0), QStringLiteral("retprice"))
                 << qMakePair(mapMsg.value("fld_buyprice").split(QChar('\t')).at(0), QStringLiteral("buyprice"));

    for ( int i = 0, iLen = mCargoFields.length(); i < iLen; ++i ) {
        QPair<QString, QString> fld = mCargoFields.at(i);
        mCargoFieldMap.insert(fld.first, fld.second);
    }
}

BsLabelPrinter::~BsLabelPrinter()
{
    qDeleteAll(mLoadDefines);
}

void BsLabelPrinter::doPrintTest(const QString &printerName,
                                 const int fromX,
                                 const int fromY,
                                 const int unitxCount,
                                 const int unitWidth,
                                 const int unitHeight,
                                 const int spaceX,
                                 const int spaceY,
                                 const int flowNum,
                                 const QList<BsLabelDef *> &defines,
                                 const QString &cargo,
                                 const QString &color,
                                 const QString &sizer,
                                 const bool testTwoRow,
                                 QWidget *dlgParent)
{
    BsLabelPrinter &inst = BsLabelPrinter::getInstance();

    inst.mPrinterName = printerName;
    inst.mFromX = fromX;
    inst.mFromY = fromY;
    inst.mUnitxCount = unitxCount;
    inst.mUnitWidth = unitWidth;
    inst.mUnitHeight = unitHeight;
    inst.mSpaceX = spaceX;
    inst.mSpaceY = spaceY;
    inst.mFlowNum = flowNum;
    inst.mTestingCargo = cargo;
    inst.mTestingColor = color;
    inst.mTestingSizer = sizer;
    inst.mTestingTwoRow = testTwoRow;

    inst.mTestDefines.clear();
    inst.mTestDefines << defines;
    inst.mppUsingDefines = &inst.mTestDefines;

    inst.mSheetPattern = QString();     //用于其他函数判断测试

    inst.mppBarcodeDef = nullptr;
    inst.mppQrcodeDef = nullptr;
    for ( int i = 0, iLen = defines.length(); i < iLen; ++i ) {
        BsLabelDef *def = defines.at(i);
        if ( def->nObjType == 2 ) {
            inst.mppBarcodeDef = def;
        }
        if ( def->nObjType == 3 ) {
            inst.mppQrcodeDef = def;
        }
    }

    //准备SKU数据
    QStringList reasons = inst.readyTestSkuMaps();

    if ( reasons.isEmpty() ) {
        QString err = inst.paintDrawOutput();
        if ( !err.isEmpty() ) {
            QMessageBox::information(dlgParent, QString(), err);
        }
    }
    else {
        reasons.prepend(QStringLiteral("所选货号色系或码类信息不全，无法生成条码！"));
        QMessageBox::information(dlgParent, QString(), reasons.join(QChar('\n')));
    }
}

void BsLabelPrinter::doPrintSheet(const QString &sheetTable, const int &sheetId, QWidget *dlgParent)
{
    BsLabelPrinter &inst = BsLabelPrinter::getInstance();

    inst.mSheetTable = sheetTable;
    inst.mSheetId = sheetId;

    qDeleteAll(inst.mLoadDefines);
    inst.mLoadDefines.clear();
    inst.mppUsingDefines = &inst.mLoadDefines;

    //样式与特定货色码确认
    if ( inst.confirmPattern(dlgParent) ) {

        //准备定义参数
        if ( inst.readySheetLabelDefine() ) {

            //准备SKU数据
            QStringList reasons = inst.readySheetSkuMaps();

            if ( reasons.isEmpty() ) {

                //数量确认
                QString qtyHint = QStringLiteral("本次打印操作将输出 %1 张标签。").arg(inst.mSkus.length());
                if ( ! confirmDialog(dlgParent,
                                     qtyHint,
                                     QStringLiteral("确定要打印吗？"),
                                     mapMsg.value("btn_ok"),
                                     mapMsg.value("btn_cancel"),
                                     QMessageBox::Question) ) {
                    return;
                }

                //执行打印
                QString err = inst.paintDrawOutput();
                if ( !err.isEmpty() ) {
                    QMessageBox::information(dlgParent, QString(), err);
                }
            }
            else {
                reasons.prepend(QStringLiteral("以下货号色系或码类信息不全，无法生成条码！"));
                QStringList shows;
                if ( reasons.length() > 11 ) {
                    shows << reasons.mid(0, 11) << QStringLiteral("...");
                } else {
                    shows << reasons;
                }
                QMessageBox::information(dlgParent, QString(), shows.join(QChar('\n')));
            }
        }
        else {
            QMessageBox::information(dlgParent, QString(), QStringLiteral("该样式没有设计标签内容！"));
        }
    }
}

QString BsLabelPrinter::getFieldOf(const QString &cname)
{
    BsLabelPrinter &inst = BsLabelPrinter::getInstance();
    return inst.mCargoFieldMap.value(cname);
}

QList<QPair<QString, QString> > BsLabelPrinter::getCargoFieldsList()
{
    BsLabelPrinter &inst = BsLabelPrinter::getInstance();
    return inst.mCargoFields;
}

bool BsLabelPrinter::confirmPattern(QWidget *dlgParent)
{
    QDialog dlg(dlgParent);

    //样式
    QComboBox *usePattern = new QComboBox(&dlg);

    QString sql = QStringLiteral("select labelname from lxlabel order by labelid;");
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    while ( qry.next() ) {
        usePattern->addItem(qry.value(0).toString());
    }
    qry.finish();
    if ( usePattern->count() == 1 ) {
        usePattern->setCurrentIndex(0);
    } else {
        usePattern->setCurrentIndex(-1);
    }

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
    QComboBox *conColor = new QComboBox(&dlg);

    //尺码
    QComboBox *conSizer = new QComboBox(&dlg);

    //左区
    QVBoxLayout *layForm = new QVBoxLayout;
    layForm->setContentsMargins(0, 0, 0, 0);
    layForm->setSpacing(0);
    layForm->addWidget(new QLabel(QStringLiteral("选择标签样式："), &dlg));
    layForm->addWidget(usePattern);
    layForm->addSpacing(20);
    layForm->addWidget(new QLabel(QStringLiteral("限打货号："), &dlg));
    layForm->addWidget(conCargo);
    layForm->addSpacing(10);
    layForm->addWidget(new QLabel(QStringLiteral("限打色号："), &dlg));
    layForm->addWidget(conColor);
    layForm->addSpacing(10);
    layForm->addWidget(new QLabel(QStringLiteral("限打尺码："), &dlg));
    layForm->addWidget(conSizer);
    layForm->addSpacing(20);

    //右区
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->setOrientation(Qt::Vertical);
    buttonBox->button(QDialogButtonBox::Ok)->setText(mapMsg.value("btn_ok"));
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(usePattern->count() == 1);
    buttonBox->button(QDialogButtonBox::Cancel)->setText(mapMsg.value("btn_cancel"));
    QDialog::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QDialog::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

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
        conColor->addItem(QStringLiteral("全部"));

        conSizer->clear();
        conSizer->addItem(QStringLiteral("全部"));

        if ( validInPicks ) {
            QString colorType = dsCargo->getValue(inputing, "colortype");
            QStringList colorList = dsColorList->getColorListByType(colorType);
            conColor->addItems(colorList);

            QString sizerType = dsCargo->getValue(inputing, QStringLiteral("sizertype"));
            QStringList sizerList = dsSizer->getSizerList(sizerType);
            conSizer->addItems(sizerList);
        }

        conColor->setCurrentIndex(0);
        conSizer->setCurrentIndex(0);
    });

    QObject::connect(usePattern, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(index >= 0);
    });

    QObject::connect(&dlg, &QDialog::destroyed, [&] {
        delete fldCargo;
    });

    //执行
    dlg.setMinimumSize(dlg.sizeHint());
    dlg.setWindowFlags(dlg.windowFlags() &~ Qt::WindowContextHelpButtonHint);
    if ( dlg.exec() != QDialog::Accepted ) {
        return false;
    }

    mSheetPattern = usePattern->currentText();
    mSheetLimCargo = conCargo->getDataValue();
    mSheetLimColor = conColor->currentText();
    mSheetLimSizer = conSizer->currentText();

    return true;
}

bool BsLabelPrinter::readySheetLabelDefine()
{
    int labelId = 0;

    //页面定义
    QString sql = QStringLiteral("select labelid, printername, fromx, fromy, "
                                 "unitxcount, unitwidth, unitheight, "
                                 "spacex, spacey, flownum, cargoexp "
                                 "from lxlabel where labelname='%1';").arg(mSheetPattern);
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    if ( qry.next() ) {
        labelId = qry.value(0).toInt();
        mPrinterName = qry.value(1).toString();
        mFromX = qry.value(2).toInt();
        mFromY = qry.value(3).toInt();
        mUnitxCount = qry.value(4).toInt();
        mUnitWidth = qry.value(5).toInt();
        mUnitHeight = qry.value(6).toInt();
        mSpaceX = qry.value(7).toInt();
        mSpaceY = qry.value(8).toInt();
        mFlowNum = qry.value(9).toInt();
        mCargoExp = qry.value(10).toString();
    }
    qry.finish();

    //检测初始化
    mppBarcodeDef = nullptr;
    mppQrcodeDef = nullptr;

    //内容定义
    sql = QStringLiteral("select nid, nobjtype, svalue, sexp, nposx, nposy, "
                         "nwidth, nheight, sfontname, nfontpoint, nfontalign "
                         "from lxlabelobj where nparentid=%1 "
                         "order by nposy, nposx;").arg(labelId);
    qry.exec(sql);
    while ( qry.next() ) {
        int nObjType = qry.value(1).toInt();
        QString sValue = qry.value(2).toString();
        BsLabelDef *def = new BsLabelDef(
                    qry.value(0).toInt(),
                    nObjType,
                    sValue,
                    qry.value(3).toString(),
                    qry.value(4).toInt(),
                    qry.value(5).toInt(),
                    qry.value(6).toInt(),
                    qry.value(7).toInt(),
                    qry.value(8).toString(),
                    qry.value(9).toInt(),
                    qry.value(10).toInt()
                    );

        if ( nObjType == 2 ) {
            mppBarcodeDef = def;
        }
        if ( nObjType == 3 ) {
            mppQrcodeDef = def;
        }

        mLoadDefines << def;  //doPrintSheet()中已经清空，并且共用指针已经改指
    }
    qry.finish();

    return mLoadDefines.length() > 0;
}

QStringList BsLabelPrinter::readyTestSkuMaps()
{
    QStringList errs;
    mSkus.clear();

    int qty = ( mTestingTwoRow ) ? 2 * mUnitxCount : mUnitxCount;

    int priceDots = mapOption.value("dots_of_price").toInt();

    QString err = readySku(mTestingCargo, mTestingColor, mTestingSizer, qty, priceDots);
    if ( !err.isEmpty() ) {
        errs << err;
    }

    return errs;
}

QStringList BsLabelPrinter::readySheetSkuMaps()
{
    QStringList errs;
    mSkus.clear();

    int priceDots = mapOption.value("dots_of_price").toInt();

    QString sql = QStringLiteral("select cargo, color, sizers "
                                 "from %1dtl where parentid=%2 "
                                 "order by cargo, color;")
            .arg(mSheetTable).arg(mSheetId);
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(sql);
    while ( qry.next() ) {      //一while一color

        //货号
        QString cargo = qry.value(0).toString();

        //判断是否筛选
        bool regexpFiltered = true;
        if ( ! mCargoExp.isEmpty() ) {
            QRegularExpression reg(QStringLiteral("^%1$").arg(mCargoExp));
            QRegularExpressionMatch match = reg.match(cargo);
            regexpFiltered = match.hasMatch();
        }

        //是否有本单确认筛选
        bool cargoWithoutLimit = (mSheetLimCargo.isEmpty() || mSheetLimCargo == cargo);

        //只打印符合条件的
        if ( regexpFiltered && cargoWithoutLimit ) {

            QString color = qry.value(1).toString();

            if ( mSheetLimColor.isEmpty() || mSheetLimColor == color ) {

                QStringList qtys = qry.value(2).toString().split(QChar(10));

                for ( int i = 0, iLen = qtys.length(); i < iLen; ++i ) {            //一for一sizer

                    QStringList qtyPair = QString(qtys.at(i)).split(QChar(9));

                    if ( qtyPair.length() == 2 ) {

                        QString sizer = qtyPair.at(0);

                        if ( mSheetLimSizer.isEmpty() || mSheetLimSizer == sizer ) {

                            int qty = QString(qtyPair.at(1)).toLongLong() / 10000;

                            QString err = readySku(cargo, color, sizer, qty, priceDots);

                            if ( !err.isEmpty() ) {
                                errs << err;
                            }
                        }
                    }
                }
            }
        }
    }

    return errs;
}

QString BsLabelPrinter::readySku(const QString &cargo, const QString &color, const QString &sizer,
                            const int qty, const int priceDots)
{
    //取码准备
    QString colorCode;
    QString sizerCode;

    //取得颜色编码colorCode
    if ( mppBarcodeDef && mppBarcodeDef->sValue.contains(QStringLiteral("[S]")) ) {

        QString colorType = dsCargo->getValue(cargo, "colortype");
        QStringList colorList = dsColorList->getColorListByType(colorType);
        int colorIndex = colorList.indexOf(color);

        if ( colorIndex >= 0 ) {
            QStringList typeCodes = dsColorList->getCodeListByType(colorType);

            if ( typeCodes.length() == colorList.length() ) {
                colorCode = typeCodes.at(colorIndex);
            }
            else if ( typeCodes.join(QString()).trimmed().isEmpty() ) {

                colorCode = fetchAsiicPrefix(color);
                if ( colorCode.isEmpty() ) {
                    return QStringLiteral("%1 色号 %2 无编码").arg(cargo).arg(color);
                }
            }
            else {
                return QStringLiteral("%1 色号 %2 无对应编码").arg(cargo).arg(color);
            }
        }
        else {
            return QStringLiteral("%1 色号 %2 在色系 %3 中未登记").arg(cargo).arg(color).arg(colorType);
        }
    }

    //取得尺码编码sizerCode
    if ( mppBarcodeDef && mppBarcodeDef->sValue.contains(QStringLiteral("[M]")) ) {

        QString sizerType = dsCargo->getValue(cargo, QStringLiteral("sizertype"));
        QStringList sizerList = dsSizer->getSizerList(sizerType);
        int sizerIndex = sizerList.indexOf(sizer);

        if ( sizerIndex >= 0 ) {
            QStringList typeCodes = dsSizer->getCodeList(sizerType);
            if ( typeCodes.length() == sizerList.length() ) {
                sizerCode = typeCodes.at(sizerIndex);
            }
            else if ( typeCodes.join(QString()).trimmed().isEmpty() ) {
                sizerCode = sizer;
            }
            else {
                return QStringLiteral("%1 尺码 %2 无对应编码").arg(cargo).arg(sizer);
            }
        }
        else {
            return QStringLiteral("%1 尺码 %2 在码类 %3 中未登记").arg(cargo).arg(sizer).arg(sizerType);
        }

    }

    //取得条码
    QString barcode;
    if ( mppBarcodeDef ) {

        barcode = mppBarcodeDef->sValue;
        barcode.replace(QStringLiteral("[K]"), cargo);
        barcode.replace(QStringLiteral("[S]"), colorCode);
        barcode.replace(QStringLiteral("[M]"), sizerCode);

        if ( barcode.contains(QStringLiteral("[J]")) ) {
            qint64 setPrice = dsCargo->getValue(cargo, QStringLiteral("setprice")).toLongLong() / 10000;
            barcode.replace(QStringLiteral("[J]"), QString::number(setPrice));
        }
    }

    //取得二维码
    QString qrcode;
    if ( mppQrcodeDef ) {

        qrcode = mppQrcodeDef->sValue;

        QString holder;

        holder = QStringLiteral("[货号]");
        if ( qrcode.contains(holder) ) {
            qrcode.replace(holder, cargo);
        }

        holder = QStringLiteral("[色号]");
        if ( qrcode.contains(holder) ) {
            qrcode.replace(holder, color);
        }

        holder = QStringLiteral("[尺码]");
        if ( qrcode.contains(holder) ) {
            qrcode.replace(holder, sizer);
        }

        holder = QStringLiteral("[%1]").arg(mapMsg.value("fld_hpname").split(QChar('\t')).at(0));
        if ( qrcode.contains(holder) ) {
            QString val = dsCargo->getValue(cargo, QStringLiteral("hpname"));
            qrcode.replace(holder, val);
        }

        holder = QStringLiteral("[%1]").arg(mapMsg.value("fld_unit").split(QChar('\t')).at(0));
        if ( qrcode.contains(holder) ) {
            QString val = dsCargo->getValue(cargo, QStringLiteral("unit"));
            qrcode.replace(holder, val);
        }

        holder = QStringLiteral("[%1]").arg(mapMsg.value("fld_setprice").split(QChar('\t')).at(0));
        if ( qrcode.contains(holder) ) {
            double  v = dsCargo->getValue(cargo, QStringLiteral("setprice")).toLongLong() / 10000.0;
            QString s = QString::number(v, 'f', priceDots);
            qrcode.replace(holder, s);
        }

        holder = QStringLiteral("[%1]").arg(mapMsg.value("fld_buyprice").split(QChar('\t')).at(0));
        if ( qrcode.contains(holder) ) {
            double  v = dsCargo->getValue(cargo, QStringLiteral("buyprice")).toLongLong() / 10000.0;
            QString s = QString::number(v, 'f', priceDots);
            qrcode.replace(holder, s);
        }

        holder = QStringLiteral("[%1]").arg(mapMsg.value("fld_lotprice").split(QChar('\t')).at(0));
        if ( qrcode.contains(holder) ) {
            double  v = dsCargo->getValue(cargo, QStringLiteral("lotprice")).toLongLong() / 10000.0;
            QString s = QString::number(v, 'f', priceDots);
            qrcode.replace(holder, s);
        }

        holder = QStringLiteral("[%1]").arg(mapMsg.value("fld_retprice").split(QChar('\t')).at(0));
        if ( qrcode.contains(holder) ) {
            double  v = dsCargo->getValue(cargo, QStringLiteral("retprice")).toLongLong() / 10000.0;
            QString s = QString::number(v, 'f', priceDots);
            qrcode.replace(holder, s);
        }

        //测试附加信息
        if ( mSheetPattern.isEmpty() ) {
            qrcode += QStringLiteral(" 测试打印附加信息www.bailisoft.com百利软件真敬业！");
        }
    }

    //添加到SKU池
    for ( int j = 0; j < qty; ++j ) {

        bool flowIncreased = false;
        if ( mppBarcodeDef && barcode.contains(QStringLiteral("[L]"))) {
            int flowLen = mppBarcodeDef->nFontPoint;  //借用值
            mFlowNum++;
            if ( QString::number(mFlowNum).length() > flowLen ) {
                mFlowNum = 1;
            }
            flowIncreased = true;
            QString flowCode = QStringLiteral("%1").arg(mFlowNum, flowLen, 10, QLatin1Char('0'));
            barcode.replace(QStringLiteral("[L]"), flowCode);
        }

        if ( mppQrcodeDef && qrcode.contains(QStringLiteral("[流水号]"))) {
            if ( !flowIncreased ) mFlowNum++;
            qrcode.replace(QStringLiteral("[流水号]"), QString::number(mFlowNum));
        }

        QStringList secs;
        secs << cargo << color << sizer << barcode << qrcode;
        mSkus << secs.join(QChar(9));
    }

    //返回
    return QString();
}

QString BsLabelPrinter::paintDrawOutput()
{
    //检测打印机
    if ( mPrinterName.isEmpty() )
        return QStringLiteral("未指定打印机！");

    QPrinterInfo prnInfo = QPrinterInfo::printerInfo(mPrinterName);
    if ( prnInfo.isNull() )
        return QStringLiteral("找不到指定打印机：") + mPrinterName;

    //打印机实例
    QPrinter printer(prnInfo);
    mPritnerDpi = printer.resolution();

    //画布
    QPainter painter(&printer);

    //基础变量
    int x = mmUnits(mFromX);
    int y = mmUnits(mFromY);
    int unitX = mmUnits(mUnitWidth + mSpaceX);
    int unitY = mmUnits(mUnitHeight + mSpaceY);

    //遍历SKU
    for ( int i = 0, iLen = mSkus.length(); i < iLen; ++i ) {

        drawSku(x, y, mSkus.at(i), &painter);

        if ( i % mUnitxCount == mUnitxCount - 1 ) {
            x = mmUnits(mFromX);
            y += unitY;
        } else {
            x += unitX;
        }
    }

    //记录流水号
    if ( !mSheetPattern.isEmpty() && mFlowNum > 0 ) {
        QString sql = QStringLiteral("update lxlabel set flownum=%1 where labelname='%2';")
                .arg(mFlowNum).arg(mSheetPattern);
        QSqlDatabase db = QSqlDatabase::database();
        db.exec(sql);
    }

    //无错成功
    return QString();
}

void BsLabelPrinter::drawSku(const int xOff, const int yOff, const QString &sku,
                             QPainter *painter)
{
    //系统参数
    int priceDots = mapOption.value("dots_of_price").toInt();

    //打印内容
    QStringList secs = sku.split(QChar(9));
    QString cargo = secs.at(0);
    QString color = secs.at(1);
    QString sizer = secs.at(2);
    QString barcode = secs.at(3);
    QString qrcode = secs.at(4);

    int x = xOff;
    int y = yOff;

    //遍历打印定义
    for ( int j = 0, jLen = mppUsingDefines->length(); j < jLen; ++j ) {

        BsLabelDef *def = mppUsingDefines->at(j);

        //文字
        if ( def->nObjType < 2 ) {

            QString text = def->sValue;

            //重新计算text
            if ( def->nObjType == 1 ) {

                QString exp = def->sExp;

                if ( QStringLiteral("cargo") == exp ) {
                    text = cargo;
                }

                if ( QStringLiteral("color") == exp ) {
                    text = color;
                }

                if ( QStringLiteral("sizer") == exp ) {
                    text = sizer;
                }

                if ( QStringLiteral("hpname") == exp ) {
                    text = dsCargo->getValue(cargo, QStringLiteral("hpname"));
                }

                if ( QStringLiteral("unit") == exp ) {
                    text = dsCargo->getValue(cargo, QStringLiteral("unit"));
                }

                if ( QStringLiteral("setprice") == exp ) {
                    double  v = dsCargo->getValue(cargo, QStringLiteral("setprice")).toLongLong() / 10000.0;
                    text = QString::number(v, 'f', priceDots);
                }

                if ( QStringLiteral("buyprice") == exp ) {
                    double  v = dsCargo->getValue(cargo, QStringLiteral("buyprice")).toLongLong() / 10000.0;
                    text = QString::number(v, 'f', priceDots);
                }

                if ( QStringLiteral("lotprice") == exp ) {
                    double  v = dsCargo->getValue(cargo, QStringLiteral("lotprice")).toLongLong() / 10000.0;
                    text = QString::number(v, 'f', priceDots);
                }

                if ( QStringLiteral("retprice") == exp ) {
                    double  v = dsCargo->getValue(cargo, QStringLiteral("retprice")).toLongLong() / 10000.0;
                    text = QString::number(v, 'f', priceDots);
                }
            }

            int left        = x + mmUnits(def->nPosX);
            int top         = y + mmUnits(def->nPosY);
            int w           = mmUnits(def->nWidth);
            int h           = 3 * painter->fontMetrics().height() / 2;
            QString ftName  = def->sFontName;
            int ftPoint     = def->nFontPoint;
            int ftAlign     = def->nFontAlign | Qt::AlignTop;

            QFont font(ftName);
            font.setPointSize(ftPoint);
            painter->setFont(font);

            painter->drawText(left, top, w, h, ftAlign, text);
        }

        //条形码
        if ( def->nObjType == 2 ) {
            int left        = x + mmUnits(def->nPosX);
            int top         = y + mmUnits(def->nPosY);
            int w           = mmUnits(def->nWidth);
            int h           = mmUnits(def->nHeight);

            QPixmap pixmap = BarLabel::generatePixmap(barcode, w, h, true);

            painter->drawImage(left, top, pixmap.toImage());
        }

        //二维码
        if ( def->nObjType == 3 ) {
            int left        = x + mmUnits(def->nPosX);
            int top         = y + mmUnits(def->nPosY);
            int w           = mmUnits(def->nWidth);

            QPixmap pixmap = QrLabel::generatePixmap(qrcode, w);

            painter->drawImage(left, top, pixmap.toImage());
        }
    }
}

BsLabelPrinter &BsLabelPrinter::getInstance()
{
    if (nullptr == instance) {
        instance = new BsLabelPrinter();
    }
    return *instance;
}

}
