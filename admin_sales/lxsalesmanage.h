#ifndef LXSALESMANAGE_H
#define LXSALESMANAGE_H

#include <QtWidgets>
#include <QtSql>

namespace BailiSoft {

class LxSalesManage : public QWidget
{
    Q_OBJECT
public:
    explicit LxSalesManage(QWidget *parent, const QString &lockAdminPassword);

private slots:
    void userPicked(int index);
    void checkEnableButton();
    void makeHard();
    void makeSoft();

private:
    bool checkInputValid();

    QComboBox*      mpUsers;

    QLineEdit*      mpUserName;
    QLineEdit*      mpBuyQty;
    QLineEdit*      mpBuyMan;
    QLineEdit*      mpBuyTele;
    QLineEdit*      mpBuyAddr;
    QLineEdit*      mpNetName;

    QPushButton*    mpBtnMakeHard;
    QPushButton*    mpBtnMakeSoft;

    QTextEdit*      mpReport;

    QString         mLockAdminPassword;
    uint            mMaxUserId;
    QSet<QString>   mMadeHardIds;
};

}


#endif // LXSALESMANAGE_H
