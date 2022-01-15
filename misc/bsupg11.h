#ifndef BSUPG11_H
#define BSUPG11_H

#include <QtWidgets>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

namespace BailiSoft {

class BsUpg11 : public QDialog
{
    Q_OBJECT

public:
    BsUpg11(QWidget *parent);
    ~BsUpg11(){}

private slots:
    void doImport();

private:
    QString importStockAsSYD(QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString updateSheetSum(const QString &sheet, QSqlDatabase &newdb);
    QString importCargo(QSqlDatabase &mdb, QSqlDatabase &newdb);

    QString getOldSizeColSel(const QString &prefix = QString());
    QString getOldSizeColSum(const QString &prefix = QString());
    QString batchExec(const QStringList &sqls, QSqlDatabase &newdb);

    QPushButton     *mpBtnExec;

    int             mOldSizeColCount;
    QStringList     mShops;
    QStringList     mSizers;

    QSet<QString>   mCargoSet;
};

}


#endif // BSUPG11_H
