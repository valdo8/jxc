#ifndef BSMAIN_H
#define BSMAIN_H

#include <QMainWindow>
#include <QMdiArea>
#include <QtCore>
#include <QtWidgets>

namespace BailiSoft {

class BsMdiArea;
class BsServer;
class BsPublisher;

class BsMain : public QMainWindow
{
    Q_OBJECT
public:
    BsMain(QWidget *parent = nullptr);
    bool eventFilter(QObject *watched, QEvent *event);
    void clickQuickButton(const QString &winBaseName, const bool statt);
    BsMdiArea*          mpMdi;

public slots:
    void openLoginGuide();

protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *e);
    void resizeEvent(QResizeEvent *e);

private slots:
    void netServerStarted(const qint64 licDateEpochSecs);
    void netServerStopped();

    void openFileInfo();
    void openSetPassword();
    void openSetLoginer();
    void openSetBarcodeRule();
    void openSetSystemOption();
    void openSetNetService();

    void openRegSizerType();
    void openRegColorType();
    void openRegCargo();
    void openRegStaff();
    void openRegSubject();
    void openRegShop();
    void openRegSupplier();
    void openRegCustomer();
    void openRegPolicy();

    void openSheetCGD();
    void openSheetCGJ();
    void openSheetCGT();
    void openQryVICGD();
    void openQryVICGJ();
    void openQryVICGT();
    void openQryViCgRest();
    void openQryViCgNeat();
    void openQryViCgCash();

    void openSheetPFD();
    void openSheetPFF();
    void openSheetPFT();
    void openSheetLSD();
    void openQryVIPFD();
    void openQryVIPFF();
    void openQryVIPFT();
    void openQryVILSD();
    void openQryViPfRest();
    void openQryViPfNeat();
    void openQryViPfCash();
    void openQryViXsNeat();
    void openQryViXsCash();

    void openSheetDBD();
    void openSheetSYD();
    void openSheetSZD();
    void openQryVIDBD();
    void openQryVISYD();
    void openQryVISZD();
    void openQryViStock();
    void openQryViewAll();
    void openQryMinAlarm();
    void openQryMaxAlarm();

    void openToolBatchEdit();
    void openToolBatchCheck();
    void openToolStockReset();
    void openToolBarcodeMaker();
    void openToolLabelDesigner();

    void openHelpManual();
    void openHelpSite();
    void openHelpAbout();

private:
    int  getTotalMenuWidth();
    bool checkRaiseSubWin(const QString &winTable);
    void addNewSubWin(QWidget *win, const bool setCenterr = false);
    void closeAllSubWin();
    bool questionCloseAllSubWin();
    void openQryAlarm(const bool isMinType);
    void setMenuAllowable();
    void updateSheetDateForMemo();

    QAction* mpMenuSetLoginer;
    QAction* mpMenuSetBarcodeRule;
    QAction* mpMenuSetSystemOption;
    QAction* mpMenuSetNetService;

    QAction* mpMenuSizerType;
    QAction* mpMenuColorType;
    QAction* mpMenuCargo;
    QAction* mpMenuStaff;
    QAction* mpMenuSubject;
    QAction* mpMenuShop;
    QAction* mpMenuSupplier;
    QAction* mpMenuCustomer;

    QAction* mpMenuSheetCGD;
    QAction* mpMenuSheetCGJ;
    QAction* mpMenuSheetCGT;
    QAction* mpMenuQryVICGD;
    QAction* mpMenuQryVICGJ;
    QAction* mpMenuQryVICGT;
    QAction* mpMenuQryVIYCG;
    QAction* mpMenuQryVIJCG;
    QAction* mpMenuQryVIJCGM;

    QAction* mpMenuSheetPFD;
    QAction* mpMenuSheetPFF;
    QAction* mpMenuSheetPFT;
    QAction* mpMenuSheetLSD;
    QAction* mpMenuQryVIPFD;
    QAction* mpMenuQryVIPFF;
    QAction* mpMenuQryVIPFT;
    QAction* mpMenuQryVILSD;
    QAction* mpMenuQryVIYPF;
    QAction* mpMenuQryVIJPF;
    QAction* mpMenuQryVIJPFM;
    QAction* mpMenuQryVIJXS;
    QAction* mpMenuQryVIJXSM;

    QAction* mpMenuSheetDBD;
    QAction* mpMenuSheetSYD;
    QAction* mpMenuSheetSZD;
    QAction* mpMenuQryVIDBD;
    QAction* mpMenuQryVISYD;
    QAction* mpMenuQryVISZD;
    QAction* mpMenuQryVIYKC;
    QAction* mpMenuQryVIALL;
    QAction* mpMenuQryMinAlarm;
    QAction* mpMenuQryMaxAlarm;

    QAction* mpMenuToolStockReset;
    QAction* mpMenuToolBatchEdit;
    QAction* mpMenuToolBatchCheck;
    QAction* mpMenuToolBarcodeMaker;
    QAction* mpMenuToolLabelDesigner;

    QLabel*         mpLoginedFile;
    QLabel*         mpLoginedUser;
    QLabel*         mpLoginedRole;
    QLabel*         mpLoginedCompany;
    QToolButton*    mpBtnNet;

    BsServer*           mpServer;
    BsPublisher*       mpSentinel;
};

}

#endif // BSMAIN_H
