#ifndef BSIMPORTR15DLG_H
#define BSIMPORTR15DLG_H

#include <QtWidgets>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

namespace BailiSoft {

class BsR15UpgSetGrid;
class BsR15UpgColorList;
class BsR15UpgColorGrid;

class BsImportR15Dlg : public QDialog
{
    Q_OBJECT

public:
    BsImportR15Dlg(QWidget *parent, const QString &accessFile);
    ~BsImportR15Dlg(){}
    bool eventFilter(QObject *watched, QEvent *event);
    static bool testR15Book(const QString &fileName);

    QString mSqliteFile;
    QString mBookName;

protected:
    void showEvent(QShowEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void resizeEvent(QResizeEvent *e);

private slots:
    void reloadOldBookData();
    void loadSizeTypeCordFile();
    void doImport();
    void overDestroy();
    void showHelpHint();
    void regExpChanged(int currentRow, int currentColumn, int, int);
    void testRegExp();
    void applyRegExp();

private:
    void switchColorFormat(const bool keepColorsCommaFormat);
    QString checkReady();
    void matchCargoRegExp(const bool forTest);
    QString initNewSchema(const QStringList &sqls, QSqlDatabase &newdb);
    QString importTableByTable(QSqlDatabase &mdb, QSqlDatabase &newdb);

    QString prepareSizeMap();
    QString importSizerType(QSqlDatabase &newdb);
    QString importColorType(QSqlDatabase &newdb);
    QString importCargo(QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString importStaff(QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString importBaseRef(const QString &oldTable, const QString &newTable, QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString importSheet(const QString &oldSheet, const QString &newSheet, QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString importStockAsInitSYD(QSqlDatabase &mdb, QSqlDatabase &newdb);
    QString updateSheetSum(const QString &sheet, QSqlDatabase &newdb);
    QString saveDebugTestData(QSqlDatabase &newdb);

    QString getOldSizeColSel(const QString &prefix = QString());
    QString getOldSizeColSum(const QString &prefix = QString());
    QString batchExec(const QStringList &sqls, QSqlDatabase &newdb);

    QPushButton         *mpBtnExpApply;
    QLabel          *mpLblHelp;
    BsR15UpgSetGrid    *mpGrdSizer;
    BsR15UpgColorGrid  *mpGrdColor;
    BsR15UpgColorList  *mpLstColor;
    QCheckBox       *mpChkOnlyStock;
    QCheckBox       *mpChkColorFormat;
    QTableWidget    *mpGrdCargo;
    QPushButton     *mpBtnExpTest;

    QString         mAccessFile;
    QString         mAccessConn;
    QString         mSqliteConn;

    int             mOldSizeTypeCount;
    int             mOldSizeColCount;
    QVector<QPair<QString, QStringList> >       vecSizerType;       //regexp, sizelist
    QMap<QString, int>                          mapSizerType;       //cargo, vecSizerType.index
    QMap<QString, double>                       mapSetPrice;        //cargo, setPrice
};


// BsR15UpgSetGrid ============================================================================
class BsR15UpgSetGrid : public QTableWidget
{
    Q_OBJECT
public:
    BsR15UpgSetGrid(QWidget *parent);
    ~BsR15UpgSetGrid(){}
protected:
    void commitData(QWidget *editor);
};


// BsR15UpgColorGrid ============================================================================
class BsR15UpgColorGrid : public BsR15UpgSetGrid
{
    Q_OBJECT
public:
    BsR15UpgColorGrid(QWidget *parent);
    ~BsR15UpgColorGrid(){}
protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);
};


// BsR15UpgColorList ============================================================================
class BsR15UpgColorList : public QListWidget
{
    Q_OBJECT
public:
    BsR15UpgColorList(QWidget *parent);
    ~BsR15UpgColorList(){}
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
private:
    QPoint      mStartPos;
};


}


#endif // BSIMPORTR15DLG_H
