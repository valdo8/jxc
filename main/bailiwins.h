#ifndef BAILIWINS_H
#define BAILIWINS_H

#include <QtWidgets>
#include <QtSql>

/************************** win类与grid类继承层次完全一致 **************************/

namespace BailiSoft {

class BsSheetIdLabel;
class BsSheetCheckLabel;
class BsConCheck;
class BsFldEditor;
class BsFldBox;
class BsField;
class BsGrid;
class BsRegGrid;
class BsAbstractFormGrid;
class BsSheetGrid;
class BsSheetCargoGrid;
class BsSheetFinanceGrid;
class BsQueryGrid;
class BsAbstractModel;
class BsListModel;
class BsRegModel;
class BsSqlModel;
class BsQryCheckor;
class BsSheetStockPickGrid;
class LxPrinter;

enum bsWindowType { bswtMisc, bswtReg, bswtSheet, bswtQuery };

enum bsQryType {
    bsqtSumSheet    = 0x00000001,   //指单据一般合计
    bsqtSumCash     = 0x00000002,   //指只统计主表的欠款统计vijcgm, vijpfm, vijxsm
    bsqtSumMinus    = 0x00000004,   //指有减法在内的统计，vijcg, vijpf, vijxs, viycg, viypf, viykc, vijcgm, vijpfm, vijxsm
    bsqtSumOrder    = 0x00000008,   //只订单相关，vicgd, viycg, vipfd, viypf
    bsqtSumRest     = 0x00000010,   //指viycg, viypf, viykc
    bsqtSumStock    = 0x00000020,   //指viykc
    bsqtViewAll     = bsqtSumSheet | bsqtSumStock    //指viall
};

//以下序号设计必须与bssetloginer单元中initRightFlagNames中三列表完全一致，并且由于在数据库永久存储，所以一旦规定，不能再改。
enum bsRightRegis { bsrrOpen = 1, bsrrNew = 2, bsrrUpd = 4, bsrrDel = 8, bsrrExport = 16 };
enum bsRightSheet { bsrsOpen = 1, bsrsNew = 2, bsrsUpd = 4, bsrsDel = 8, bsrsCheck  = 16, bsrsPrint = 32, bsrsExport = 64 };
enum bsRightQuery { bsrqQty  = 1, bsrqMny = 2, bsrqDis = 4, bsrqPay = 8, bsrqOwe    = 16, bsrqPrint = 32, bsrqExport = 64 };

//关于Action状态控制，virtual BsAbstractFormWin::setEditable()基本控制，其余在后代setEditable()重载函数中，
//或者BsQryWin::updateToolActions()中
//对一个个action列举设置即可。————好设计不是代码量少，是思路清晰简单，因此不要过度设计！
enum bsActionOnOff { bsacfDirty = 1, bsacfClean = 2,
                     bsacfNotPlusId = 4, bsacfPlusId = 8,
                     bsacfNotChk = 16, bsacfChecked = 32,
                     bsacfQrySelecting = 64, bsacfQryReturned = 128,
                     bsacfQryByShop = 256, bsacfQryByTrader = 512, bsacfQryByCargo = 1024 };

// BsWin
class BsWin : public QWidget
{
    Q_OBJECT
public:
    explicit BsWin(QWidget *parent, const QString &mainTable, const QStringList &fields, const uint bsWinType);
    ~BsWin();
    virtual bool isEditing() = 0;
    BsField *getFieldByName(const QString &name);
    QString table() const { return mMainTable; }
    void setQuickDate(const QString &periodName, BsFldBox *dateB, BsFldBox *dateE, QToolButton *button);
    void exportGrid(const BsGrid* grid, const QStringList headerPairs = QStringList());
    QString pairTextToHtml(const QStringList &pairs, const bool lastRed = false);
    bool getOptValueByOptName(const QString &optName);

    QString             mMainTable;
    uint                mBsWinType;

public slots:
    void displayGuideTip(const QString &tip);
    void forceShowMessage(const QString &msg);
    void hideFoceMessage();

protected:
    void showEvent(QShowEvent *e);
    void closeEvent(QCloseEvent *e);

    virtual void doToolExport();

    QMainWindow     *mppMain;               //传入主窗
    BsGrid              *mpGrid;            //后代创建具体类
    BsQueryGrid         *mpQryGrid;
    BsAbstractFormGrid  *mpFormGrid;
    BsRegGrid           *mpRegGrid;
    BsSheetGrid         *mpSheetGrid;
    BsSheetCargoGrid    *mpSheetCargoGrid;
    BsSheetFinanceGrid  *mpSheetFinanceGrid;

    QLabel          *mpGuide;
    QWidget         *mpPnlMessage;
    QLabel              *mpLblMessage;
    QPushButton         *mpBtnMessage;

    QToolBar        *mpToolBar;

    QAction         *mpAcToolSeprator;
    QMenu           *mpMenuToolCase;
    QMenu           *mpMenuOptionBox;
    QAction         *mpAcMainHelp;

    QAction         *mpAcToolExport;

    QAction         *mpAcOptGuideNotShowAnymore;

    QSplitter       *mpSpl;
    QWidget             *mpBody;
    QTabWidget          *mpTaber;

    QStatusBar      *mpStatusBar;

    QString         mGuideClassTip;
    QString         mGuideObjectTip;
    bool            mGuideTipSwitch;

    QString             mRightWinName;
    QList<BsField*>     mFields;     //只是主表字段

    //公用数据集
    BsListModel         *mpDsStype;
    BsSqlModel          *mpDsStaff;
    BsAbstractModel     *mpDsTrader;

    int                 mDiscDots;
    int                 mPriceDots;
    int                 mMoneyDots;

private slots:
    void clickHelp();
    void clickToolExport() { doToolExport(); }

    void clickOptGuideNotShowAnymore();

private:
    void loadAllOptionSettings();
    void saveAllOptionSettings();
};


// BsQryWin
class BsQryWin : public BsWin
{
    Q_OBJECT
public:
    explicit BsQryWin(QWidget *parent, const QString &name, const QStringList &fields, const uint qryFlags);
    ~BsQryWin(){}
    bool isEditing() {return false;}

protected:
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);

    void doToolExport();

    QAction *mpAcMainBackQry;
    QAction *mpAcMainHistory;
    QAction *mpAcMainPrint;
    QAction *mpToolAddCalcSetMoney;
    QAction *mpToolAddCalcRetMoney;
    QAction *mpToolAddCalcLotMoney;
    QAction *mpToolAddCalcBuyMoney;

    QWidget             *mpPanel;
    QGroupBox               *mpPnlCon;
    QToolButton                 *mpBtnPeriod;
    BsConCheck                  *mpConCheck;
    BsFldBox                  *mpConDateB;
    BsFldBox                  *mpConDateE;
    BsFldBox                  *mpConShop;
    BsFldBox                  *mpConStype;
    BsFldBox                  *mpConStaff;
    BsFldBox                  *mpConTrader;
    BsFldBox                  *mpConCargo;          //也用为mpConSubject
    BsFldBox                  *mpConColorType;
    BsFldBox                  *mpConSizerType;
    QGroupBox               *mpPnlSel;
    QGroupBox               *mpPnlVal;
    QLabel                  *mpLblCon;
    QCheckBox               *mpHaving;

    QWidget             *mpPnlQryConfirm;
    QToolButton             *mpBtnBigOk;
    QToolButton             *mpBtnBigCancel;

private slots:
    void clickQuickPeriod();
    void clickQryExecute();
    void clickBigCancel();
    void clickQryBack();
    void clickHistory();
    void clickPrint();
    void cargoInputing(const QString &text, const bool);
    void conSizeTypeChanged();
    void chkSizersPicked(const QString &stype);
    void doToolAddCalcSetMoney();
    void doToolAddCalcRetMoney();
    void doToolAddCalcLotMoney();
    void doToolAddCalcBuyMoney();

private:
    void updateToolActions();
    bool checkFieldIfSelected(const QString &fldName);
    bool canListHistory();
    void setFloatorGeometry();
    QStringList getConExpPairsFromMapRangeCon(const bool includeDate, const bool includeCargoRel = true);
    QStringList getSizersQtySplitSql(const QStringList &selExps,
                                     const QString &fromSource,
                                     const QStringList &conExps,
                                     const QString &sizerType,
                                     const QString &tmpTableName,
                                     const bool forStockk = false);
    QString prepairViewAllData(const QSet<QString> &setSel, const QStringList &noTimeConExps);
    QString doSqliteQuery();

    uint                    mQryFlags;
    BsQryCheckor           *mpSizerCheckor;
    QStringList             mLabelPairs;
    QMap<QString, QString>  mapRangeCon;  //field, value
    QString                 mFromSource;

};


// BsAbstractFormWin
class BsAbstractFormWin : public BsWin
{
    Q_OBJECT
public:
    explicit BsAbstractFormWin(QWidget *parent, const QString &name, const QStringList &fields, const uint bsWinType);
    ~BsAbstractFormWin() {}
    bool isEditing() {return mEditable;}
    QSize sizeHint() const;
    virtual void setEditable(const bool editt);

protected:
    void closeEvent(QCloseEvent *e);

    virtual void doNew() = 0;
    virtual void doEdit() = 0;
    virtual void doDel() = 0;
    virtual void doSave() = 0;
    virtual void doCancel() = 0;
    virtual void doToolImport() = 0;

    virtual bool isValidRealSheetId() = 0;
    virtual bool isSheetChecked() = 0;
    virtual bool mainNeedSaveDirty() = 0;

    QAction *mpAcMainNew;
    QAction *mpAcMainEdit;
    QAction *mpAcMainDel;
    QAction *mpAcMainSave;
    QAction *mpAcMainCancel;

    QAction *mpAcToolImport;
    QAction *mpToolHideCurrentCol;
    QAction *mpToolShowAllCols;

    QAction *mpAcOptHideDropRow;

    QLabel      *mpSttValKey;
    QLabel      *mpSttLblUpman;
    QLabel      *mpSttValUpman;
    QLabel      *mpSttLblUptime;
    QLabel      *mpSttValUptime;

    QStringList     mDenyFields;

private slots:
    void clickNew() { doNew(); }
    void clickEdit() { doEdit(); }
    void clickDel() { doDel(); }
    void clickSave() { doSave(); }
    void clickCancel() { doCancel(); }

    void clickToolImport() { doToolImport(); }

    void clickOptHideDropRow();

private:
    bool    mEditable;
};


// BsRegWin
class BsRegWin : public BsAbstractFormWin
{
    Q_OBJECT
public:
    explicit BsRegWin(QWidget *parent, const QString &name, const QStringList &fields);
    ~BsRegWin(){}

protected:
    void showEvent(QShowEvent *e);
    void closeEvent(QCloseEvent *e);
    void doNew();
    void doEdit() {}
    void doDel() {}
    void doSave();
    void doCancel();
    void doToolImport();

    bool isValidRealSheetId();
    bool isSheetChecked() { return false; }
    bool mainNeedSaveDirty() {return false; }

    QAction *mpToolCreateColorType;
    QAction *mpToolAlarmSetting;
    QAction *mpToolAlarmRemove;

private slots:
    void doOpenEditMode();
    void showStatus(const QStringList &values);
    void clickToolAlarmSetting();
    void clickToolAlarmRemove();
    void clickToolCreateColorType();

    void clickAmGeo();
    void clickToolAmUnGeo();
    void clickAmTag();
    void clickToolAmUnTag();

private:
    bool checkGetTags();

    QAction*    mpAcAmGeo;
    QAction*    mpToolAmUnGeo;
    QAction*    mpAcAmTag;
    QAction*    mpToolAmUnTag;

    QStringList mHttpTags;
};


// BsAbstractSheetWin
class BsAbstractSheetWin : public BsAbstractFormWin
{
    Q_OBJECT
public:
    explicit BsAbstractSheetWin(QWidget *parent, const QString &name, const QStringList &fields);
    ~BsAbstractSheetWin();
    void setEditable(const bool editt);
    void openSheet(const int sheetId);

    //以下主要用于打印调用
    QString getPrintValue(const QString &valueName) const;
    QString getPrintValue(const QString &cargoTableField, const int gridRow);
    QString getGridItemValue(const int row, const int col) const;
    QString getGridItemValue(const int row, const QString &fieldName) const;
    BsField* getGridFieldByName(const QString &fieldName) const;
    int getGridColByField(const QString &fieldName) const;
    int getGridRowCount() const;
    QStringList getSizerNameListForPrint() const;
    QStringList getSizerQtysOfRowForPrint(const int row);
    bool isLastRow(const int row);

    LxPrinter   *mpPrinter;
    bool         mAllowPriceMoney;
    int          mCurrentSheetId;

signals:
    void shopStockChanged(const QString &shop, const QString &relSheet, const int relId);

protected:
    void closeEvent(QCloseEvent *e);

    void doOpenFind();
    void doCheck();
    void doPrint();

    void doNew();
    void doEdit();
    void doDel();
    void doSave();
    void doCancel();

    bool isValidRealSheetId() { return mCurrentSheetId > 0; }
    bool isSheetChecked() { return ! mpSttValChkTime->text().isEmpty(); }
    bool mainNeedSaveDirty();
    double getTraderDisByName(const QString &name);

    virtual bool printZeroSizeQty() { return false; }
    virtual void doOpenQuery() = 0;
    virtual void doSyncFindGrid() = 0;
    virtual void updateTabber(const bool editablee) = 0;

    QAction*    mpAcMainOpen;
    QAction*    mpAcMainCheck;
    QAction*    mpAcMainPrint;

    QAction*    mpToolPrintSetting;
    QAction*    mpToolUnCheck;
    QAction*    mpToolAdjustCurrentRowPosition;

    QAction*    mpAcOptSortBeforePrint;
    QAction*    mpAcOptHideNoQtySizerColWhenOpen;
    QAction*    mpAcOptHideNoQtySizerColWhenPrint;

    QVBoxLayout         *mpLayBody;

    QWidget                 *mpPnlOpener;
    QWidget                     *mpPnlOpenCon;
    QToolButton                     *mpBtnOpenBack;
    QToolButton                     *mpBtnPeriod;
    BsConCheck                      *mpConCheck;
    BsFldBox                      *mpConDateB;
    BsFldBox                      *mpConDateE;
    BsFldBox                      *mpConShop;
    BsFldBox                      *mpConStype;
    BsFldBox                      *mpConStaff;
    BsFldBox                      *mpConTrader;
    QToolButton                     *mpBtnQuery;
    BsQueryGrid                 *mpFindGrid;

    QWidget                 *mpPnlHeader;
    QHBoxLayout                 *mpLayTitleBox;
    QLabel                          *mpSheetName;
    BsSheetIdLabel                  *mpSheetId;
    BsSheetCheckLabel               *mpCheckMark;
    QGridLayout                 *mpLayEditBox;
    BsFldBox                      *mpDated;
    BsFldBox                      *mpStype;
    BsFldBox                      *mpShop;
    BsFldBox                      *mpProof;
    BsFldBox                      *mpStaff;
    BsFldBox                      *mpTrader;
    BsFldBox                  *mpRemark;

    QWidget                 *mpPnlPayOwe;
    BsFldBox                  *mpActPay;
    BsFldBox                  *mpActOwe;

    QLabel      *mpSttLblChecker;
    QLabel      *mpSttValChecker;
    QLabel      *mpSttLblChkTime;
    QLabel      *mpSttValChkTime;

    QList<BsField*>     mFindFlds;

private slots:
    void clickOpenFind() { doOpenFind(); }
    void clickCheck() { doCheck(); }
    void clickPrint() { doPrint(); }

    void clickToolPrintSetting();
    void clickToolUnCheck();
    void clickToolAdjustCurrentRowPosition();

    void clickQuickPeriod();
    void clickExecuteQuery();
    void clickCancelOpenPage();
    void doubleClickOpenSheet(QTableWidgetItem *item);

private:
    void savedReconcile(const int sheetId, const qint64 uptime);
    void cancelRestore();
};


// BsSheetCargoWin
class BsSheetCargoWin : public BsAbstractSheetWin
{
    Q_OBJECT
public:
    explicit BsSheetCargoWin(QWidget *parent, const QString &name, const QStringList &fields);
    ~BsSheetCargoWin();

protected:
    void showEvent(QShowEvent *e);
    void doOpenQuery();
    void doSyncFindGrid();
    void updateTabber(const bool editablee);
    bool printZeroSizeQty() { return mpAcOptPrintZeroSizeQty->isChecked(); }

    void doToolExport();
    void doToolImport();

private slots:
    void traderEditing(const QString &text, const bool);
    void showCargoInfo(const QStringList &values);
    void sumMoneyChanged(const QString &sumValue);
    void actPayChanged();
    void taberIndexChanged(int index);
    void doToolAutoRePrice();
    void doToolDefineFieldName();
    void doToolImportBatchBarcodes();
    void doToolPrintCargoLabels();
    void pickStockTraderChecked();
    void loadPickStock();

private:
    void restoreTaberMiniHeight();
    void loadFldsUserNameSetting();

    BsListModel*        mpDsAttr1;
    BsListModel*        mpDsAttr2;
    BsListModel*        mpDsAttr3;
    BsListModel*        mpDsAttr4;
    BsListModel*        mpDsAttr5;
    BsListModel*        mpDsAttr6;

    QList<BsField*>     mGridFlds;
    BsField*            mPickSizerFld;
    BsField*            mPickAttr1Fld;
    BsField*            mPickAttr2Fld;
    BsField*            mPickAttr3Fld;
    BsField*            mPickAttr4Fld;
    BsField*            mPickAttr5Fld;
    BsField*            mPickAttr6Fld;

    QLabel*             mpPnlScan;
    QWidget*            mpPnlPick;
    BsSheetStockPickGrid*   mpPickGrid;
    QWidget*                mpPickCons;
    BsFldEditor*                mpPickSizerType;
    BsFldEditor*                mpPickAttr1;
    BsFldEditor*                mpPickAttr2;
    BsFldEditor*                mpPickAttr3;
    BsFldEditor*                mpPickAttr4;
    BsFldEditor*                mpPickAttr5;
    BsFldEditor*                mpPickAttr6;

    QCheckBox*                  mpPickDate;
    QCheckBox*                  mpPickCheck;
    QCheckBox*                  mpPickTrader;

    QAction*    mpAcToolDefineName;
    QAction*    mpAcToolAutoRePrice;
    QAction*    mpAcToolImportBatchBarcodes;
    QAction*    mpAcToolPrintCargoLabels;

    QAction*    mpAcOptPrintZeroSizeQty;
    QAction*    mpAcOptAutoUseFirstColor;
};


// BsSheetFinanceWin
class BsSheetFinanceWin : public BsAbstractSheetWin
{
    Q_OBJECT
public:
    explicit BsSheetFinanceWin(QWidget *parent, const QStringList &fields);
    ~BsSheetFinanceWin();

protected:
    void doOpenQuery();
    void doSyncFindGrid();
    void updateTabber(const bool) {}
    void doToolImport() {}

private:
    QList<BsField*>     mGridFlds;
};

}



#endif // BAILIWINS_H
