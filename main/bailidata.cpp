#include "bailidata.h"
#include "bailicode.h"
#include "bailifunc.h"
#include "bailigrid.h"
#include "bailicustom.h"
#include "comm/pinyincode.h"
#include "dialog/bsrefsheetdlg.h"


// BsSizerModel
namespace BailiSoft {

BsSizerModel::BsSizerModel() : BsAbstractModel(nullptr)
{
    mMaxCount = 0;
}

int BsSizerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    //仅用于货号登记窗口“尺码品类”列的下拉选择（程序中扫描以及动态列名获取，用另外定义的getXXXX函数）
    return lstTypeName.length();
}

QVariant BsSizerModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
        if ( role == Qt::DisplayRole ) {
            return lstTypeName.at(index.row());
        }
        else {
            return lstTypePinyin.at(index.row());
        }
    }
    return QVariant();
}

void BsSizerModel::reload()
{
    mMaxCount = 0;
    lstTypeName.clear();
    lstTypePinyin.clear();
    mapScan.clear();
    mapName.clear();
    mapCode.clear();

    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec("select tname, namelist, codelist from sizertype;");
    while ( qry.next() ) {
        QString key = qry.value(0).toString();
        lstTypeName << key;
        lstTypePinyin << (QChar(32) + LxSoft::ChineseConvertor::GetFirstLetter(key));

        QStringList names = qry.value(1).toString().replace(QChar(32), QString()).split(QChar(44), QString::SkipEmptyParts);
        QStringList codes = qry.value(2).toString().replace(QChar(32), QString()).split(QChar(44), QString::SkipEmptyParts);

        mapScan.insert(key, names.length() == codes.length());
        mapName.insert(key, names);
        mapCode.insert(key, codes);

        if ( names.length() > mMaxCount )
        {
            mMaxCount = names.length();
        }
    }
    qry.finish();

    //升级版本改增
    QStringList wumaNames, wumaCodes;
    wumaNames << mapMsg.value("mix_size_name");
    wumaCodes << QStringLiteral("*");
    mapScan.insert(QString(), true);
    mapName.insert(QString(), wumaNames);
    mapCode.insert(QString(), wumaCodes);
}

QStringList BsSizerModel::getSizerList(const QString &sizerType)
{
    return mapName.value(sizerType);
}

QStringList BsSizerModel::getCodeList(const QString &sizerType)
{
    return mapCode.value(sizerType);
}

QString BsSizerModel::getSizerNameByIndex(const QString &sizerType, const int idx)
{
    QStringList names = mapName.value(sizerType);
    if ( idx >= 0 && idx < names.length() ) {
        return names.at(idx);
    }
    return QString();
}

QString BsSizerModel::getSizerNameByCode(const QString &sizerType, const QString &code)
{
    QStringList codes = mapCode.value(sizerType);
    QStringList names = mapName.value(sizerType);
    int idx = codes.indexOf(code);
    if ( idx >= 0 && idx < names.length() ) {
        return names.at(idx);
    }
    return QString();
}

int BsSizerModel::getColIndexBySizerCode(const QString &sizerType, const QString &code)
{
    return mapCode.value(sizerType).indexOf(code);
}

int BsSizerModel::getColIndexBySizerName(const QString &sizerType, const QString &name)
{
    return mapName.value(sizerType).indexOf(name);
}

QStringList BsSizerModel::getWholeUniqueNames()
{
    QSet<QString> sizers;
    for ( int i = 0, iLen = lstTypeName.length(); i < iLen; ++i ) {
        QStringList names = mapName.value(lstTypeName.at(i));
        foreach (QString s, names) {
            sizers.insert(s);
        }
    }

    QStringList wholeNames;
    QSetIterator<QString> it(sizers);
    while ( it.hasNext() ) {
        wholeNames << it.next();
    }

    return wholeNames;
}

QString checkCargoImageFile(const QString &hpcode)
{
    QString imgFile = QDir(imageDir).absoluteFilePath(QStringLiteral("%1.JPG").arg(hpcode));
    if ( ! QFile(imgFile).exists() )
        imgFile = QDir(imageDir).absoluteFilePath(QStringLiteral("%1.jpg").arg(hpcode));
    if ( ! QFile(imgFile).exists() )
        imgFile = QDir(imageDir).absoluteFilePath(QStringLiteral("%1.JPEG").arg(hpcode));
    if ( ! QFile(imgFile).exists() )
        imgFile = QDir(imageDir).absoluteFilePath(QStringLiteral("%1.jpeg").arg(hpcode));
    if ( ! QFile(imgFile).exists() ) {
        return QString();
    }
    return imgFile;
}

}


// BsColorTypeModel
namespace BailiSoft {

BsColorTypeModel::BsColorTypeModel() : BsAbstractModel(nullptr)
{
    //Nothing
}

int BsColorTypeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    //做为货号登记窗口“颜色系列”列的下拉选择
    return lstTypeName.length();
}

QVariant BsColorTypeModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
        if ( role == Qt::DisplayRole ) {
            return lstTypeName.at(index.row());
        }
        else {
            return lstTypePinyin.at(index.row());
        }
    }
    return QVariant();
}

void BsColorTypeModel::reload()
{
    lstTypeName.clear();
    lstTypePinyin.clear();

    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec("select tname from colortype;");
    while ( qry.next() ) {
        QString key = qry.value(0).toString();
        lstTypeName << key;
        lstTypePinyin << (QChar(32) + LxSoft::ChineseConvertor::GetFirstLetter(key));
    }
    qry.finish();
}

}


// BsColorListModel
namespace BailiSoft {

BsColorListModel::BsColorListModel() : BsAbstractModel(nullptr)
{
    //Nothing
}

int BsColorListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QStringList names = mapName.value(mFilteringType);
    return names.length();
}

QVariant BsColorListModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
        return mapName.value(mFilteringType).at(index.row());
    }
    return QVariant();
}

void BsColorListModel::reload()
{
    mapName.clear();
    mapCode.clear();
    mapScan.clear();

    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec("select tname, namelist, codelist from colortype;");
    while ( qry.next() ) {
        QString key = qry.value(0).toString();

        QStringList names = qry.value(1).toString().replace(QChar(32), QString()).split(QChar(44), QString::SkipEmptyParts);
        QStringList codes = qry.value(2).toString().replace(QChar(32), QString()).split(QChar(44), QString::SkipEmptyParts);

        bool useScan = names.length() == codes.length();
        mapScan.insert(key, useScan);
        mapName.insert(key, names);

        QStringList scanCodes;
        for ( int i = 0, iLen = names.length(); i < iLen; ++i ) {
            QString scanCode = ( useScan ) ? codes.at(i) : "";
            scanCodes << scanCode;
        }
        mapCode.insert(key, scanCodes);
    }

    qry.exec("select colortype, hpcode from cargo;");
    while ( qry.next() ) {
        QString hptype = qry.value(0).toString().trimmed()
                .replace(QStringLiteral("，"), QStringLiteral(","))
                .replace(QChar(32), QString());

        if ( ! hptype.isEmpty() && ! mapName.contains(hptype) ) {
            QStringList parts = hptype.split(QChar(','));
            QStringList names, codes;
            bool codeAll = true;
            for ( int i = 0, iLen = parts.length(); i < iLen; ++i ) {
                QString part = QString(parts.at(i));
                QString code = fetchCodePrefix(part);
                names << part;
                codes << code;
                if ( code.isEmpty() ) codeAll = false;
            }
            int codeLen = QString(codes.at(0)).length();
            for ( int i = 1, iLen = codes.length(); i < iLen; ++i ) {
                QString code = codes.at(i);
                if ( code.length() != codeLen ) codeAll = false;  //codeAll此处表示canScan
            }

            mapScan.insert(hptype, codeAll);
            mapName.insert(hptype, names);
            mapCode.insert(hptype, codes);
        }
    }
    qry.finish();
}

void BsColorListModel::setFilterByCargoType(const QString &colorType)
{
    beginResetModel();
    mFilteringType = colorType;
    endResetModel();
}

bool BsColorListModel::foundColorInType(const QString &color, const QString &colorType)
{
    QStringList names = mapName.value(colorType);
    return names.indexOf(color) >= 0;
}

QString BsColorListModel::getColorByCodeInType(const QString &code, const QString &colorType)
{
    QStringList codes = mapCode.value(colorType);
    int idx = codes.indexOf(code);
    if ( idx < 0 ) return QString();

    QStringList names = mapName.value(colorType);
    return names.at(idx);
}

QStringList BsColorListModel::getColorListByType(const QString &colorType)
{
    return mapName.value(colorType);
}

QStringList BsColorListModel::getCodeListByType(const QString &colorType)
{
    return mapCode.value(colorType);
}

QString BsColorListModel::getFirstColorByType(const QString &colorType)
{
    QStringList colors = mapName.value(colorType);
    if ( colors.isEmpty() )
        return QString();
    else
        return colors.at(0);
}

}


// BsSqlListModel
namespace BailiSoft {

BsSqlListModel::BsSqlListModel(QWidget *parent, const QString &sql, const bool commaList)
    : BsAbstractModel(parent), mSql(sql), mCommaList(commaList)
{
    //Nothing required here
    //NOTICE:
    //sql必须是返回一行一列的逗号分隔字符串，参见reload()
}

int BsSqlListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mValues.count();
}

QVariant BsSqlListModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
        //popup show
        if ( role == Qt::DisplayRole ) {
            return mValues.at(index.row());
        }
        //pinyin
        else {
            return mPinyins.at(index.row());
        }
    }
    return QVariant();
}

void BsSqlListModel::reload()
{
    beginResetModel();

    mValues.clear();
    mPinyins.clear();

    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec(mSql);
    while ( qry.next() ) {
        if ( mCommaList ) {
            mValues = qry.value(0).toString().split(QChar(44));
            break;
        }
        else {
            QString v = qry.value(0).toString();
            if ( !v.isEmpty() ) mValues << qry.value(0).toString();
        }
    }
    qry.finish();

    for ( int i = 0, iLen = mValues.length(); i < iLen; ++i ) {
        QString pinyin = (QChar(32) + LxSoft::ChineseConvertor::GetFirstLetter(mValues.at(i)));
        mPinyins << pinyin;
    }

    endResetModel();
}


}


// BsRegModel
namespace BailiSoft {

BsRegModel::BsRegModel(const QString &table, const QStringList &fields)
    : BsAbstractModel(nullptr), mTable(table), mFields(fields), mReloadEpochSecs(0)
{
    mUseCode = QString(mFields.at(0)).toLower().contains("code");
    mCargoSizeIdx = fields.indexOf(QStringLiteral("sizertype"));
    mCargoColorIdx = fields.indexOf(QStringLiteral("colortype"));
    mCargoNameIdx = fields.indexOf(QStringLiteral("hpname"));
    mCargoSetPriceIdx = fields.indexOf(QStringLiteral("setprice"));
}

int BsRegModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mRecIndex.count();
}

//注意：mRecords行内各fieldIndex与mFields偏移一位！前面少了主键字段（hpcode/kname），但后面加了pinyin列。注意别错位。
QVariant BsRegModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
        //popup show
        if ( role == Qt::DisplayRole ) {
            //cargo
            if ( mCargoSizeIdx > 0 && mCargoColorIdx > 0 && mCargoSetPriceIdx > 0 ) {
                QString key = mRecIndex.at(index.row());
                QStringList vals = mRecords.value(key);
                qint64 setPrice = QString(vals.at(mCargoSetPriceIdx - 1)).toLongLong();
                return QStringLiteral("%1 %2 %3").arg(key).arg(vals.at(1 - 1)).arg(bsNumForRead(setPrice, mPriceDots));
            }
            //staff, shop, customer, supplier, subject
            else {
                return mRecIndex.at(index.row());
            }
        }
        //pinyin
        else {
            QString key = mRecIndex.at(index.row());
            return mRecords.value(key).at(mFields.length() - 1);
        }
    }

    return QVariant();
}

void BsRegModel::switchBookLogin()
{
    mReloadEpochSecs = 0;
    mRecords.clear();
    mRecIndex.clear();
}

//通过时间记录机制，保证每次只传输有变动的数据
//注意：mRecords行内各fieldIndex与mFields偏移一位！前面少了主键字段（hpcode/kname），但后面加了pinyin列。注意别错位。
void BsRegModel::reload()
{
    mPriceDots = mapOption.value("dots_of_price").toInt();

    //begin
    beginResetModel();

    if ( mReloadEpochSecs > 0 ) {
        mReloadEpochSecs = QDateTime::currentMSecsSinceEpoch() / 1000 - 60;
    }

    //数据
    QStringList sels;
    for (int i = 0, iLen = mFields.length(); i < iLen; ++i ) {
        sels << mFields.at(i);
    }
    QString sql = QStringLiteral("SELECT %1 FROM %2").arg(sels.join(QChar(44))).arg(mTable);
    QStringList limits;
    if ( mTable == QStringLiteral("subject") && !loginAsBoss )
        limits << QStringLiteral("adminboss=0");
    if ( mReloadEpochSecs > 0 )
        limits << QStringLiteral("uptime>%1;").arg(mReloadEpochSecs);
    if ( limits.length() > 0 )
        sql += QStringLiteral(" WHERE %1").arg(limits.join(QStringLiteral(" and ")));
    sql += QChar(';');

    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << "\n" << sql;
    while ( qry.next() ) {
        QString recKey = qry.value(0).toString().trimmed();
        QStringList vals;
        for (int i = 1, iLen = mFields.length(); i < iLen; ++i ) {
            vals << qry.value(i).toString().trimmed();
        }
        QString pinyin = (mUseCode)
                ? (QChar(32) + recKey + LxSoft::ChineseConvertor::GetFirstLetter(qry.value(1).toString()))
                : (QChar(32) + LxSoft::ChineseConvertor::GetFirstLetter(recKey));
        vals << pinyin;
        mRecords.insert(recKey, vals);

        if ( mRecIndex.indexOf(recKey) < 0 ) {
            mRecIndex << recKey;
        }
    }
    qry.finish();

    //重新排序，重建
    std::sort(mRecIndex.begin(), mRecIndex.end());

    //end
    endResetModel();
}

QString BsRegModel::getValue(const QString &keyValue, const QString &fldName)
{
    QStringList vals = mRecords.value(keyValue);
    if ( vals.length() == mFields.length() ) {
        int fldIdx = mFields.indexOf(fldName);
        if ( fldIdx > 0 ) {
            return vals.at(fldIdx - 1);
        }
    }
    return QString();
}

QString BsRegModel::getCargoBasicInfo(const QString &keyValue)
{
    if ( mCargoNameIdx > 0 && mCargoSetPriceIdx > 0 ) {
        QStringList vals = mRecords.value(keyValue);
        if ( vals.isEmpty() )
            return QString();
        Q_ASSERT(vals.length() > mCargoNameIdx - 1);
        Q_ASSERT(vals.length() > mCargoSetPriceIdx - 1);
        qint64 setPrice = QString(vals.at(mCargoSetPriceIdx - 1)).toLongLong();
        return vals.at(mCargoNameIdx - 1) + QChar(32) + bsNumForRead(setPrice, mPriceDots);
    }
    return QString();
}


}


// BsSqlModel
namespace BailiSoft {

BsSqlModel::BsSqlModel(QWidget *parent, const QString &sql) : BsAbstractModel(parent), mSql(sql)
{
}

int BsSqlModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mRecIndex.count();
}

//注意：mRecords行内各fieldIndex与mFields偏移一位！前面少了主键字段，但后面加了pinyin列。注意别错位。
QVariant BsSqlModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DisplayRole || role == Qt::EditRole ) {
        //popup show
        if ( role == Qt::DisplayRole ) {
            return mRecIndex.at(index.row());
        }
        //pinyin
        else {
            QString key = mRecIndex.at(index.row());
            return mRecords.value(key).at(mFields.length() - 1);
        }
    }

    return QVariant();
}

void BsSqlModel::switchBookLogin()
{
    mRecords.clear();
    mRecIndex.clear();
}

//注意：mRecords行内各fieldIndex与mFields偏移一位！前面少了主键字段，但后面加了pinyin列。注意别错位。
void BsSqlModel::reload()
{
    //begin
    beginResetModel();

    //数据
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec(mSql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << "\n" << mSql;
    QSqlRecord rec = qry.record();
    for ( int i = 0, iLen = rec.count(); i < iLen; ++i )
        mFields << rec.fieldName(i);

    while ( qry.next() ) {
        QString recKey = qry.value(0).toString();
        QStringList vals;
        for (int i = 1, iLen = mFields.length(); i < iLen; ++i ) {
            vals << qry.value(i).toString();
        }
        QString pinyin = (QChar(32) + LxSoft::ChineseConvertor::GetFirstLetter(recKey));
        vals << pinyin;
        mRecords.insert(recKey, vals);

        if ( mRecIndex.indexOf(recKey) < 0 ) {
            mRecIndex << recKey;
        }
    }
    qry.finish();

    //重新排序，重建
    std::sort(mRecIndex.begin(), mRecIndex.end());

    //end
    endResetModel();
}

QString BsSqlModel::getValue(const QString &keyValue, const QString &fldName)
{
    QStringList vals = mRecords.value(keyValue);
    if ( vals.length() == mFields.length() ) {
        int fldIdx = mFields.indexOf(fldName);
        if ( fldIdx > 0 ) {
            return vals.at(fldIdx - 1);
        }
    }
    return QString();
}

}


// BsFinRel
namespace BailiSoft {

BsFinRel::BsFinRel()
{
    reload();
}

void BsFinRel::reload()
{
    mapDefine.clear();

    QString sql;
    QSqlQuery qry;
    qry.setForwardOnly(true);

    for ( int i = 0, iLen = lstSheetWinTableNames.length(); i < iLen; ++i ) {

        QString sheet = lstSheetWinTableNames.at(i);
        QStringList ins, exs;

        sql = QStringLiteral("select kname from subject where refsheetin='%1' order by kname;").arg(sheet);
        qry.exec(sql);
        while (qry.next()) {
            ins << qry.value(0).toString();
        }

        sql = QStringLiteral("select kname from subject where refsheetex='%1' order by kname;").arg(sheet);
        qry.exec(sql);
        while (qry.next()) {
            exs << qry.value(0).toString();
        }

        mapDefine.insert(sheet, qMakePair(ins, exs));
    }

    sql = QStringLiteral("select vsetting from bailioption where optcode='set_sheet_subject_divchar';");
    qry.exec(sql);
    if (qry.next()) {
        QString optValue = qry.value(0).toString();
        mDivChar = (optValue.length() > 0) ? optValue.at(0) : QChar('-');
    } else {
        mDivChar = QChar('-');
    }
}

bool BsFinRel::linkDefineNothing(const QString &sheet)
{
    QPair<QStringList, QStringList> p = mapDefine.value(sheet);
    return p.first.isEmpty() && p.second.isEmpty();
}

bool BsFinRel::linkDefineInvalidd(const QString &sheet)
{
    QPair<QStringList, QStringList> p = mapDefine.value(sheet);
    return (p.first.length() > 1 && p.second.length() > 1) || p.first.isEmpty() || p.second.isEmpty();
}

bool BsFinRel::getDialogAssign(const QString &sheet, const qint64 amount, QWidget *winParent)
{
    mInVals.clear();
    mExVals.clear();

    QPair<QStringList, QStringList> p = mapDefine.value(sheet);

    if (p.first.length() > 1 || p.second.length() > 1) {

        QStringList longNames = (p.first.length() > 1) ? p.first : p.second;
        QStringList shortNames;
        for ( int i = 0, iLen = longNames.length(); i < iLen; ++i ) {
            QStringList secs = QString(longNames.at(i)).split(mDivChar);
            shortNames << secs.at(secs.length() - 1);
        }

        BsRefSheetDlg dlg(amount, shortNames, winParent);
        if (dlg.exec() == QDialog::Accepted) {
            if (p.first.length() > 1) {
                mInVals << dlg.mAssigns;
            } else {
                mExVals << dlg.mAssigns;
            }
        } else {
            return false;
        }
    }

    if (p.first.length() == 1) {
        mInVals << amount;
    }

    if (p.second.length() == 1) {
        mExVals << amount;
    }

    return true;
}

bool BsFinRel::checkValuesAssign(const QString &sheet, QList<qint64> inValues, QList<qint64> exValues)
{
    mInVals.clear();
    mExVals.clear();

    QPair<QStringList, QStringList> p = mapDefine.value(sheet);

    if ( p.first.length() != inValues.length() || p.second.length() != exValues.length() ) {
        return false;
    }

    mInVals << inValues;
    mExVals << exValues;

    return true;
}

QStringList BsFinRel::qryBatchSqls(const QString &sheet, const qint64 dated, const QString &proof,
                                   const QString &shop, const QString &trader)
{
    QPair<QStringList, QStringList> p = mapDefine.value(sheet);

    //新单据号
    int sheetId = 1;
    QSqlQuery qry;
    qry.exec(QStringLiteral("SELECT seq FROM sqlite_sequence WHERE name='szd';"));
    if ( qry.next() ) sheetId = qry.value(0).toInt() + 1;
    qry.finish();

    //sqls
    QStringList batches;

    int rowTime = QDateTime::currentMSecsSinceEpoch();
    for ( int i = 0, iLen = p.first.length(); i < iLen; ++i ) {
        if ( mInVals.at(i) != 0 ) {
            rowTime++;
            QString sql = QStringLiteral("insert into szddtl(parentid, rowtime, subject, income, expense) "
                                         "values(%1, %2, '%3', %4, 0);")
                    .arg(sheetId)
                    .arg(rowTime)
                    .arg(p.first.at(i))
                    .arg(mInVals.at(i));
            batches << sql;
        }
    }

    for ( int i = 0, iLen = p.second.length(); i < iLen; ++i ) {
        if ( mExVals.at(i) != 0 ) {
            rowTime++;
            QString sql = QStringLiteral("insert into szddtl(parentid, rowtime, subject, income, expense) "
                                         "values(%1, %2, '%3', 0, %4);")
                    .arg(sheetId)
                    .arg(rowTime)
                    .arg(p.second.at(i))
                    .arg(mExVals.at(i));
            batches << sql;
        }
    }

    QString sql = QStringLiteral("insert into szd(sheetid, proof, dated, shop, trader, upman, uptime) "
                                 "values(%1, '%2', %3, '%4', '%5', 'system', %6);")
            .arg(sheetId)
            .arg(proof)
            .arg(dated)
            .arg(shop)
            .arg(trader)
            .arg(QDateTime::currentSecsSinceEpoch());
    batches << sql;

    return batches;
}


}


// init
namespace BailiSoft {

int checkDataDir()
{
    //账册文件夹一律放用户家目录。
    QDir currentDir(QDir::homePath());
    QString dataDirName = QStringLiteral("BailiR17_Data");
    if ( !currentDir.mkpath(dataDirName) ) {
        QMessageBox::warning(nullptr, BailiSoft::mapMsg.value("app_name"),
                             BailiSoft::mapMsg.value("i_app_make_datadir_fail"));
        return 1;
    }
    currentDir.cd(dataDirName);
    dataDir = currentDir.absolutePath();

    //图片文件夹，默认放在数据文件夹images子目录
    if ( !currentDir.mkpath(QStringLiteral("images")) ) {
        QMessageBox::warning(nullptr, BailiSoft::mapMsg.value("app_name"),
                             BailiSoft::mapMsg.value("i_app_make_imagedir_fail"));
        return 1;
    }
    currentDir.cd(QStringLiteral("images"));
    imageDir = currentDir.absolutePath();

    //备份文件夹，一律放在程序目录的BAK子文件夹下
    QDir bDir( qApp->applicationDirPath() );
    QString bakDirName = QStringLiteral("BAK");
    if ( !bDir.mkpath(bakDirName) ) {
        QMessageBox::warning(nullptr, BailiSoft::mapMsg.value("app_name"),
                             BailiSoft::mapMsg.value("i_app_make_bakdir_fail"));
        return 1;
    }
    bDir.cd(bakDirName);
    backupDir = bDir.absolutePath();

    //复制例子数据
    currentDir.cdUp();
    QDir binDir(qApp->applicationDirPath());
    QFile::copy(binDir.absoluteFilePath("demo.dat"), currentDir.absoluteFilePath(QStringLiteral("百利样例")));
    copyDir(binDir.absoluteFilePath("images"), currentDir.absoluteFilePath(QStringLiteral("images")), false);

    return 0;
}

void copyDemoBook(){
    QDir fromDir(BailiSoft::backupDir);
    QString fromFile = fromDir.absoluteFilePath(BailiSoft::mapMsg.value("app_sample_file_name"));

    QDir toDir(BailiSoft::dataDir);
    QString toFile = toDir.absoluteFilePath(BailiSoft::mapMsg.value("app_sample_file_name"));
    if ( QFile::exists(fromFile) && !QFile::exists(toFile) )
        QFile::copy(fromFile, toFile);
}

//仅用于测试驱动环境
int openDefaultSqliteConn() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setConnectOptions("QSQLITE_ENABLE_REGEXP");
    if ( db.isValid() ) {
        db.setDatabaseName(":memory:");
        db.open();
        db.close();
    } else {
        QMessageBox::warning(nullptr, BailiSoft::mapMsg.value("app_name"),
                             BailiSoft::mapMsg.value("i_app_sqlite_driver_drror"));
        return 2;
    }
    return 0;
}



int loginLoadOptions()
{
    //以下为简单数据结构，直接加载
    QSqlQuery qry;
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);

    //bailioption
    mapOption.clear();
    qry.exec(QStringLiteral("select optcode, vsetting from bailioption;"));
    while ( qry.next() ) {
        QString optCode = qry.value(0).toString();
        QString vsetting = qry.value(1).toString();
        if ( optCode.startsWith(QStringLiteral("stypes_")) || optCode.endsWith(QStringLiteral("_list")) ) {
            vsetting.replace(QChar(32), QString()).replace(QStringLiteral("，"), QStringLiteral(","));
        }
        mapOption.insert(optCode, vsetting);
    }

    mapFldUserSetName.clear();
    qry.exec(QStringLiteral("select tblname, fldname, cname from bailifldname;"));
    while ( qry.next() ) {
        QString defKey = QStringLiteral("%1_%2").arg(qry.value(0).toString()).arg(qry.value(1).toString());
        mapFldUserSetName.insert(defKey, qry.value(2).toString());
    }

    //barcodeRule
    vecBarcodeRule.clear();
    qry.exec(QStringLiteral("select barcodexp, sizermiddlee from barcoderule;"));
    while ( qry.next() ) {
        vecBarcodeRule << qMakePair(qry.value(0).toString(), qry.value(1).toBool());
    }

    qry.finish();

    //字段结构（需要根据mapOption）
    cargoSheetCommonFields.clear();
    cargoSheetCommonFields << QStringLiteral("sheetid")
                           << QStringLiteral("dated")
                           << QStringLiteral("stype")
                           << QStringLiteral("shop")
                           << QStringLiteral("proof")
                           << QStringLiteral("staff")
                           << QStringLiteral("trader")
                           << QStringLiteral("remark")
                           << QStringLiteral("sumqty")
                           << QStringLiteral("summoney")
                           << QStringLiteral("sumdis")
                           << QStringLiteral("actpay")
                           << QStringLiteral("actowe")
                           << QStringLiteral("upman")
                           << QStringLiteral("uptime")
                           << QStringLiteral("checker")
                           << QStringLiteral("chktime");

    financeSheetCommonFields.clear();
    financeSheetCommonFields << QStringLiteral("sheetid")
                             << QStringLiteral("dated")
                             << QStringLiteral("stype")
                             << QStringLiteral("shop")
                             << QStringLiteral("proof")
                             << QStringLiteral("staff")
                             << QStringLiteral("trader")
                             << QStringLiteral("remark")
                             << QStringLiteral("upman")
                             << QStringLiteral("uptime")
                             << QStringLiteral("checker")
                             << QStringLiteral("chktime");

    //此顺序决定窗口内排列顺序
    cargoQueryCommonFields.clear();
    cargoQueryCommonFields << QStringLiteral("yeard")
                           << QStringLiteral("monthd")
                           << QStringLiteral("dated")
                           << QStringLiteral("sheetid")
                           << QStringLiteral("proof")
                           << QStringLiteral("stype")
                           << QStringLiteral("staff")
                           << QStringLiteral("shop")
                           << QStringLiteral("trader")
                           << QStringLiteral("cargo")
                           << QStringLiteral("hpname")
                           << QStringLiteral("color")
                           << QStringLiteral("sizers")                           
                           << QStringLiteral("setprice")
                           << QStringLiteral("unit");
    for ( int i = 1; i <= 6; ++i ) {
        QString attrx = QStringLiteral("attr%1").arg(i);
        QString optkey = QStringLiteral("cargo_attr%1_name").arg(i);
        if ( !mapOption.value(optkey).trimmed().isEmpty() ) {
            cargoQueryCommonFields << attrx;
        }
    }
    cargoQueryCommonFields << QStringLiteral("colortype")
                           << QStringLiteral("sizertype")
                           << QStringLiteral("sumqty")
                           << QStringLiteral("summoney")
                           << QStringLiteral("sumdis")
                           << QStringLiteral("actpay")
                           << QStringLiteral("actowe"); //不加入qty、actmoney、dismoney是因为只需sumxxx代表即可。

    //此顺序决定窗口内排列顺序
    financeQueryCommonFields.clear();
    financeQueryCommonFields << QStringLiteral("yeard")
                           << QStringLiteral("monthd")
                           << QStringLiteral("dated")
                           << QStringLiteral("sheetid")
                           << QStringLiteral("proof")
                           << QStringLiteral("stype")
                           << QStringLiteral("staff")
                           << QStringLiteral("shop")
                           << QStringLiteral("trader")
                           << QStringLiteral("refsheetin")
                           << QStringLiteral("refsheetex")
                           << QStringLiteral("subject");
    for ( int i = 1; i <= 6; ++i ) {
        QString attrx = QStringLiteral("attr%1").arg(i);
        QString optkey = QStringLiteral("subject_attr%1_name").arg(i);
        if ( !mapOption.value(optkey).trimmed().isEmpty() ) {
            financeQueryCommonFields << attrx;
        }
    }
    financeQueryCommonFields << QStringLiteral("income")
                             << QStringLiteral("expense");

    //return
    return 0;
}

//加载权限数据
extern QMap<QString, uint>     mapRights;
QMap<QString, uint>     mapRights;
int loginLoadRights()
{
    mapRights.clear();

    QSqlQuery qry;
    QString sql = QStringLiteral("select retprice, lotprice, buyprice, %1, %2, %3 "
                                 "from baililoginer where loginer='%4';")
            .arg(lstRegisWinTableNames.join(QChar(44)))
            .arg(lstSheetWinTableNames.join(QChar(44)))
            .arg(lstQueryWinTableNames.join(QChar(44)))
            .arg(loginer);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError().text() << "\n" << sql;
    qry.next();

    canRett = loginAsAdminOrBoss || qry.value(0).toBool();
    canLott = loginAsAdminOrBoss || qry.value(1).toBool();
    canBuyy = loginAsAdminOrBoss || qry.value(2).toBool();

    int idxBase = 3;
    for ( int i = idxBase; i < idxBase + lstRegisWinTableNames.length(); ++i )
        mapRights.insert(lstRegisWinTableNames.at(i - idxBase), qry.value(i).toUInt());

    idxBase += lstRegisWinTableNames.length();
    for ( int i = idxBase; i < idxBase + lstSheetWinTableNames.length(); ++i )
        mapRights.insert(lstSheetWinTableNames.at(i - idxBase), qry.value(i).toUInt());

    idxBase += lstSheetWinTableNames.length();
    for ( int i = idxBase; i < idxBase + lstQueryWinTableNames.length(); ++i )
        mapRights.insert(lstQueryWinTableNames.at(i - idxBase), qry.value(i).toUInt());

    sql = QStringLiteral("select loginer, bindcustomer from baililoginer where length(passhash)>0;");  //包括总经理
    qry.exec(sql);
    grantOffiShops = 0;
    grantCustomers = 0;
    while ( qry.next() ) {
        QString loginer = qry.value(0).toString();
        QString bindCustomer = qry.value(1).toString().trimmed();
        if ( !bindCustomer.isEmpty() ) {
            grantCustomers++;
        } else {
            if ( loginer != bossAccount ) {
                grantOffiShops++;
            }
        }
    }
    qry.finish();

    return 0;
}

//权限调用
bool canDo(const QString &winBaseName, const uint rightFlag)
{
    Q_ASSERT(winBaseName.indexOf(QChar('_')) < 0);

    if ( loginAsAdminOrBoss )
        return true;

    if ( ! mapRights.contains(winBaseName) )
        return false;

    uint v = mapRights.value(winBaseName);

    if ( rightFlag == 0 )
        return v != 0;  //只要有一个权限就通过，用于主窗口菜单于主控面板按钮的enabled
    else
        return (v & rightFlag) != 0;
}

//程序启动初始化，切换登录时，将各BsRegModel对象的时间设为0，分别调用reload()
int loginLoadRegis()
{
    //这也说明loginLoadOptions必须在先
    int hpMarkNum = mapOption.value("sheet_hpmark_define").toInt();

    //sizerType
    if ( dsSizer )
        dsSizer->reload();
    else
        dsSizer  = new BsSizerModel();

    //colorType
    if ( dsColorType )
        dsColorType->reload();
    else
        dsColorType = new BsColorTypeModel();

    //colorList
    if ( dsColorList )
        dsColorList->reload();
    else
        dsColorList = new BsColorListModel();

    //cargo
    if ( dsCargo ) {
        dsCargo->switchBookLogin();
        dsCargo->reload();
    }
    else {
        QStringList ls;
        ls << QStringLiteral("hpcode")
           << QStringLiteral("hpname")     //必须第二位，因为计算pinyin时取的qry.value(1).toString()
           << QStringLiteral("unit")
           << QStringLiteral("sizertype")
           << QStringLiteral("colortype")
           << QStringLiteral("setprice")
           << QStringLiteral("retprice")
           << QStringLiteral("lotprice")
           << QStringLiteral("buyprice");
        if ( hpMarkNum > 0 && hpMarkNum < 7 )
            ls << QStringLiteral("attr%1").arg(hpMarkNum);
        dsCargo = new BsRegModel(QStringLiteral("cargo"), ls);
        //注意，如果关键字是code类，则必须放name前面传入，且将要在下拉pop中显示的字段放最前。
    }

    //subject
    if ( dsSubject ) {
        dsSubject->switchBookLogin();
        dsSubject->reload();
    }
    else {
        QStringList ls;
        ls << QStringLiteral("kname");
        dsSubject = new BsRegModel(QStringLiteral("subject"), ls);
    }

    //shop
    if ( dsShop ) {
        dsShop->switchBookLogin();
        dsShop->reload();
    }
    else {
        QStringList ls;
        ls << QStringLiteral("kname")
           << QStringLiteral("regdis");
        dsShop = new BsRegModel(QStringLiteral("shop"), ls);
    }

    //customer
    if ( dsCustomer ) {
        dsCustomer->switchBookLogin();
        dsCustomer->reload();
    }
    else {
        QStringList ls;
        ls << QStringLiteral("kname")
           << QStringLiteral("regdis");
        dsCustomer = new BsRegModel(QStringLiteral("customer"), ls);
    }

    //supplier
    if ( dsSupplier )
    {
        dsSupplier->switchBookLogin();
        dsSupplier->reload();
    }
    else {
        QStringList ls;
        ls << QStringLiteral("kname")
           << QStringLiteral("regdis");
        dsSupplier = new BsRegModel(QStringLiteral("supplier"), ls);
    }

    //finRel
    if ( finRel ) {
        finRel->reload();
    } else {
        finRel = new BsFinRel();
    }

    //return
    return 0;
}

}


// extern global
namespace BailiSoft {

QString      dataDir;
QString      backupDir;
QString      imageDir;

QStringList  cargoSheetCommonFields;
QStringList  financeSheetCommonFields;
QStringList  cargoQueryCommonFields;
QStringList  financeQueryCommonFields;

QString      loginBook;
QString      loginFile;
QString      loginer;
QString      loginShop;
QString      loginPassword;
int          grantOffiShops;
int          grantCustomers;

bool         canRett;
bool         canLott;
bool         canBuyy;

BsSizerModel*                   dsSizer     = nullptr;
BsColorTypeModel*               dsColorType = nullptr;
BsColorListModel*               dsColorList = nullptr;
BsRegModel*                     dsCargo     = nullptr;
BsRegModel*                     dsSubject   = nullptr;
BsRegModel*                     dsShop      = nullptr;
BsRegModel*                     dsCustomer  = nullptr;
BsRegModel*                     dsSupplier  = nullptr;
BsFinRel*                       finRel      = nullptr;

QMap<QString, QString>                  mapOption;
QMap<QString, QString>                  mapFldUserSetName;
QVector<QPair<QString, bool> >          vecBarcodeRule;

}
