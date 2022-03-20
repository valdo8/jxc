#ifndef BAILIDATA_H
#define BAILIDATA_H

#include <QtCore>
#include <QObject>
#include <QtWidgets>
#include <QtSql>


// BsAbstractModel
namespace BailiSoft {

class BsAbstractModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    BsAbstractModel(QObject *parent) : QAbstractTableModel(parent) {}
    int columnCount(const QModelIndex &parent = QModelIndex()) const { Q_UNUSED(parent) return 1;}
    virtual void reload() = 0;
    virtual bool keyExists(const QString &keyValue) = 0;
};

}


// BsSizerModel
namespace BailiSoft {

class BsSizerModel : public BsAbstractModel
{
    Q_OBJECT
public:
    BsSizerModel();
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void reload();
    bool keyExists(const QString &keyValue) { return mapName.contains(keyValue); }
    QStringList getSizerList(const QString &sizerType);
    QStringList getCodeList(const QString &sizerType);
    QString getSizerNameByIndex(const QString &sizerType, const int idx);
    QString getSizerNameByCode(const QString &sizerType, const QString &code);
    int getColIndexBySizerCode(const QString &sizerType, const QString &code);
    int getColIndexBySizerName(const QString &sizerType, const QString &name);
    int getMaxColCount() { return mMaxCount; }
    QStringList getWholeUniqueNames();
    QStringList getAllTypes() { return lstTypeName; }

private:
    QStringList                     lstTypeName;
    QStringList                     lstTypePinyin;
    QMap<QString, bool>             mapScan;
    QMap<QString, QStringList>      mapName;
    QMap<QString, QStringList>      mapCode;
    int                             mMaxCount;
};

}


// BsColorTypeModel
namespace BailiSoft {

class BsColorTypeModel : public BsAbstractModel
{
    Q_OBJECT
public:
    BsColorTypeModel();
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void reload();
    bool keyExists(const QString &keyValue) { return lstTypeName.indexOf(keyValue) >= 0; }
private:
    QStringList     lstTypeName;
    QStringList     lstTypePinyin;
};
}

// BsColorListModel
namespace BailiSoft {

class BsColorListModel : public BsAbstractModel
{
    Q_OBJECT
public:
    BsColorListModel();
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void reload();
    bool keyExists(const QString &keyValue) { return mapName.contains(keyValue); }
    void setFilterByCargoType(const QString &colorType);
    bool foundColorInType(const QString &color, const QString &colorType);
    QString getColorByCodeInType(const QString &code, const QString &colorType);
    QStringList getColorListByType(const QString &colorType);
    QStringList getCodeListByType(const QString &colorType);
    QString getFirstColorByType(const QString &colorType);

private:
    QString         mFilteringType;
    QMap<QString, QStringList>  mapName;
    QMap<QString, QStringList>  mapCode;
    QMap<QString, bool>         mapScan;
};
}


// BsSqlListModel
namespace BailiSoft {

class BsSqlListModel : public BsAbstractModel
{
    Q_OBJECT
public:
    BsSqlListModel(QWidget *parent, const QString &sql, const bool commaList = true);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void reload();
    bool keyExists(const QString &keyValue) { return mValues.indexOf(keyValue) >= 0; }
private:
    QString         mSql;
    QStringList     mValues;
    QStringList     mPinyins;
    bool            mCommaList = true;
};

}


// BsRegModel
namespace BailiSoft {

class BsRegModel : public BsAbstractModel
{
    Q_OBJECT
public:
    BsRegModel(const QString &table, const QStringList &fields);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void reload();
    bool keyExists(const QString &keyValue) { return mRecords.contains(keyValue); }

    void switchBookLogin();
    QString getTableName() { return mTable; }

    QString getValue(const QString &keyValue, const QString &fldName);
    QString getCargoBasicInfo(const QString &keyValue);

private:
    bool        mUseCode;
    int         mCargoSizeIdx;
    int         mCargoColorIdx;
    int         mCargoNameIdx;
    int         mCargoSetPriceIdx;


    QString                     mTable;
    QStringList                 mFields;
    int                         mPriceDots;

    QMap<QString, QStringList>  mRecords;
    QStringList                 mRecIndex;

    qint64               mReloadEpochSecs;
};

}


// BsSqlModel
namespace BailiSoft {

class BsSqlModel : public BsAbstractModel
{
    Q_OBJECT
public:
    BsSqlModel(QWidget *parent, const QString &sql);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void reload();
    bool keyExists(const QString &keyValue) { return mRecords.contains(keyValue); }

    void switchBookLogin();

    QString getValue(const QString &keyValue, const QString &fldName);

private:
    QString                     mSql;
    QStringList                 mFields;

    QMap<QString, QStringList>  mRecords;
    QStringList                 mRecIndex;
};
}


// BsFinRel
namespace BailiSoft {

class BsFinRel
{
public:
    BsFinRel();
    void reload();
    bool linkDefineNothing(const QString &sheet);
    bool linkDefineInvalidd(const QString &sheet);
    bool getDialogAssign(const QString &sheet, const qint64 amount, QWidget *winParent);
    bool checkValuesAssign(const QString &sheet, QList<qint64> inValues, QList<qint64> exValues);
    QStringList qryBatchSqls(const QString &sheet, const qint64 dated, const QString &proof,
                             const QString &shop, const QString &trader);
private:
    QMap<QString, QPair<QStringList, QStringList> > mapDefine;
    QList<qint64>   mInVals;
    QList<qint64>   mExVals;
    QChar           mDivChar;
};
}


// init
namespace BailiSoft {

int     checkDataDir();
QString checkCargoImageFile(const QString &hpcode);
void    copyDemoBook();
int     openDefaultSqliteConn();
int     loginLoadOptions();
int     loginLoadRights();
int     loginLoadRegis();
bool    canDo(const QString &winBaseName, const uint rightFlag = 0);

}


// extern global
namespace BailiSoft {

extern QString                  dataDir;
extern QString                  backupDir;
extern QString                  imageDir;

extern QStringList              cargoSheetCommonFields;
extern QStringList              financeSheetCommonFields;
extern QStringList              cargoQueryCommonFields;
extern QStringList              financeQueryCommonFields;

extern QString                  loginBook;
extern QString                  loginFile;
extern QString                  loginer;
extern QString                  loginShop;
extern QString                  loginPassword;
extern int                      grantOffiShops;
extern int                      grantCustomers;

extern bool                     canRett;
extern bool                     canLott;
extern bool                     canBuyy;

extern BsSizerModel*                    dsSizer;
extern BsColorTypeModel*                dsColorType;
extern BsColorListModel*                dsColorList;
extern BsRegModel*                      dsCargo;
extern BsRegModel*                      dsSubject;
extern BsRegModel*                      dsShop;
extern BsRegModel*                      dsCustomer;
extern BsRegModel*                      dsSupplier;
extern BsFinRel*                        finRel;

extern QMap<QString, QString>                   mapOption;
extern QMap<QString, QString>                   mapFldUserSetName;         //指用户额外自定义得字段名，可为空
extern QVector<QPair<QString, bool> >           vecBarcodeRule;

}


#endif // BAILIDATA_H
