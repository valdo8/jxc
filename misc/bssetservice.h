#ifndef BSSETSERVICE_H
#define BSSETSERVICE_H

#include <QtWidgets>
#include <QtSql>

namespace BailiSoft {

class BsServer;

class BsSetService : public QTabWidget
{
    Q_OBJECT
public:
    explicit BsSetService(QWidget *parent, BsServer* netServer);
    ~BsSetService();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void clickServerStart();
    void clickServerStop();
    void clickSetBossword();
    void startFailed(const QString &errMsg);
    void qryLog();
    void qryChat();
    void loadFrontUsers();
    void loadChaters();
    void userComboChanged(const int idx);
    void waitRestartTick();
    void serviceStarted(const qint64 licEpochDate);
    void serviceStopped();

private:
    QWidget*            mpPnlSvr;
    QLabel*                 mpLblManulTitle;
    QLabel*                 mpLblManualContent;
    QPushButton*            mpBtnStart;
    QPushButton*            mpBtnStop;
    QLabel*                 mpLblPowerHint;

    QWidget*            mpPnlLog;
    QWidget*                mpPnlLogCon;
    QDateEdit*                  mpLogConDateB;
    QDateEdit*                  mpLogConDateE;
    QComboBox*                  mpLogConUser;
    QPushButton*                mpLogBtnQry;
    QLabel*                     mpLblUserInfo;
    QToolButton*                mpBtnBossword;
    QTableWidget*           mpGrdLog;

    QWidget*            mpPnlChat;
    QWidget*                mpPnlChatCon;
    QDateEdit*                  mpChatConDateB;
    QDateEdit*                  mpChatConDateE;
    QComboBox*                  mpChatConSender;
    QComboBox*                  mpChatConReceiver;
    QPushButton*                mpChatBtnQry;
    QTableWidget*           mpGrdChat;

    QTimer          mTicker;

    BsServer*       mppServer;
    int             mBossIndex;

    bool            mUserClicking = false;
    int             mRestartTicks = 0;
};

}

#endif // BSSETSERVICE_H
