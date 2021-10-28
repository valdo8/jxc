#ifndef BSIMPORTR16DLG_H
#define BSIMPORTR16DLG_H

#include <QtWidgets>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

namespace BailiSoft {

class BsImpCargo {
public:
    BsImpCargo(const QString &prname, const QString &pcolors, const QString &prstype, const double psprice, const int pindex) :
        hpname(prname), colors(pcolors), sizetype(prstype), setPrice(psprice), grdIndex(pindex) {}
    QString hpname;
    QString colors;
    QString sizetype;
    double  setPrice;
    int     grdIndex;
};

class BsImportR16Dlg : public QDialog
{
    Q_OBJECT

public:
    BsImportR16Dlg(QWidget *parent, const QString &accessFile);
    ~BsImportR16Dlg(){}

    QString mSqliteFile;
    QString mBookName;

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:
    void clickSqlExecute();
    void clickR16ExpTest();
    void clickR16ExpRestore();
    void clickR16ExpApply();
    void editR16ExpChanged();

    void doImport();
    void overDestroy();
    void fetchColorName();
    void dealLeftAsSameStyle();
    void dealAllAsSameStyle();
    void gridDoubleClicked(QTableWidgetItem *item);

private:
    void applyR16Test();
    void restoreR16Test();

    void initSourceR16Data();
    QString importToSqlite(const QString &sqliteFile);
    QString initNewSchema(const QStringList &sqls, QSqlDatabase &newdb);
    QString importTableByTable(QSqlDatabase &mdb, QSqlDatabase &newdb);

    QString prepareSizeMap(QSqlDatabase &mdb);
    QString importSizerType(QSqlDatabase &newdb);
    QString importColorType(QSqlDatabase &newdb);
    QString importCargo(QSqlDatabase &newdb);
    QString importStaff(QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString importBaseRef(const QString &newTable, QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString importSheet(const QString &oldSheet, const QString &newSheet, QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString importStock(const int newSheetId, const QString &shop, QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString updateSheetSum(const QString &sheet, QSqlDatabase &newdb);
    QString saveDebugTestData(QSqlDatabase &newdb);

    QString getOldSizeColSel(const QString &prefix = QString());
    QString getOldSizeColSum(const QString &prefix = QString());
    QString getOldSizeColHaving(const QString &prefix = QString());
    QString batchExec(const QStringList &sqls, QSqlDatabase &newdb);


    QTextEdit*      mpEdtSql;
    QPushButton*    mpBtnSql;
    QLineEdit*      mpR16Exp;
    QLabel*         mpLblR16ExpHelp;

    QTableWidget*   grdR16Cargo;
    QPushButton*    mpBtnR16ExpTest;
    QPushButton*    mpBtnFetchColorName;
    QPushButton*    mpBtnR16ExpRestore;
    QPushButton*    mpBtnR16ExpApply;
    QCheckBox*      mpChkOnlyStock;

    QString         mAccessFile;
    QString         mAccessConn;
    QString         mSqliteConn;

    int                                         mOldSizeColCount;
    QMap<QString, int>                          mapGridRow;         //R16Cargo, R16CargoGridRowIdx
    QMap<QString, QStringList>                  mapSizeType;        //R16SizeTypeName, R16SizeList
    QMap<QString, BsImpCargo*>                  mapCargo;           //R17Cargo, ...
};

}


#endif // BSIMPORTR16DLG_H
