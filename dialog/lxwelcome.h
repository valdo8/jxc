#ifndef LXWELCOME_H
#define LXWELCOME_H

#include <QtWidgets>
#include <QtNetwork>

#define LXAPP_VERSION_MAJOR     17
#define LXAPP_VERSION_MINOR     1
#define LXAPP_VERSION_PATCH     50
/*
48.ok查询结果打印行细线条以适用于针打
48.ok查询结果筛选隐藏行不再打印出来。
48.ok单据统计查询角度增加单据号、品名hpname（含库存）
48.ok查询工具箱金额演算增加差额以计算毛利
48.ok主窗口工具，批量审核工具（限总经理）
50.ok登记窗口添加“新增”按钮。
50.ok爱美无价功能
50.ok总经理账号名可以更换的功能
*/


namespace BailiSoft {

enum bsVersionReqType { bsverNum, bsVerDoc, bsverDown };

class LxProgressBar;

class LxWelcome : public QWidget
{
    Q_OBJECT
public:
    explicit LxWelcome();
    bool    mCanOver;
    bool    mNeedQuit;

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);

private slots:
    void doNetCheckStart();
    void doClickLeft();
    void doClickRight();
    void doClickUpgradeDoc();
    void doSimulateLoadOver();

    void httpReadyRead();
    void httpReadProgress(qint64 bytesRead, qint64 totalBytes);
    void httpFinished();
    void httpError(QNetworkReply::NetworkError code);
    void httpSslErrors(const QList<QSslError> &errors);

private:
    void startRequest(QUrl prUrl, const bsVersionReqType reqType);

    void execCheckVersionSuccess();
    void execCheckVersionFail();

    void execGetUpgradeDocSuccess();
    void execGetUpgradeDocFail();

    void execUpgradeDownAbort();
    void execUpgradeDownSuccess();
    void execUpgradeDownFail(const QString &prErr);

    bool needUpgradeNow();

    bsVersionReqType    mCurrentReqType;
    int                 mNetMajorNum;
    int                 mNetMinorNum;
    int                 mNetPatchNum;
    QString             mNetVerFile;
    QString             mNetDocFile;
    QString             mNetAppFile;

    QLabel          *mpLblImg;
    QLabel          *mpBuildNum;
    LxProgressBar   *mpProgress;
    QToolButton     *mpBtnLeft;
    QToolButton     *mpBtnRight;

    qint64          mTimeStart; //记录网络版本成功获取时间（如果时间太短，使用模拟加载等待3秒钟效果）

    QUrl                    mUsingUrl;
    QNetworkReply          *mpNetReply;
    QByteArray              mBytesVernum;
    QByteArray              mBytesUpgdoc;
    QFile                  *mpFileDown;
    bool                    mDownFileAborted;
};

class LxProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit LxProgressBar(QWidget *parent);

    void showMessage(const QString &prMsg, const bool prFlatt);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QString     mShowMsg;

};

}

#endif // LXWELCOME_H
