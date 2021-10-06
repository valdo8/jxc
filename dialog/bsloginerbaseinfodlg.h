#ifndef BSLOGINERBASEINFODLG_H
#define BSLOGINERBASEINFODLG_H

#include <QtWidgets>

namespace BailiSoft {

// BsLoginerBaseInfoDlg ==========================================================================
class BsLoginerBaseInfoDlg : public QDialog
{
    Q_OBJECT
public:
    BsLoginerBaseInfoDlg(QWidget *parent, const bool forEdit = false);
    void setPasswordAndDeviceType(const QString &deskPass, const QString &netCode,
                                  const QString &bindShop, const QString &bindCustomer);
    QString getDeskPass() const { return mpEdtDeskPass->text(); }
    QString getNetCode() const { return mpEdtNetCode->text(); }

    QLineEdit*          mpEdtName;
    QLabel*             mpLblPassword;

    QLabel*             mpLblIcon;

    QGroupBox*      mpGrpDevice;
    QRadioButton*       mpDeviceLocal;
    QRadioButton*       mpDeviceRemotePc;
    QRadioButton*       mpDeviceRemoteMob;

    QGroupBox*      mpGrpType;
    QRadioButton*       mpTypeBacker;
    QRadioButton*       mpTypeShop;
    QRadioButton*       mpTypeCustomer;

    QGroupBox*      mpGrpBind;
    QComboBox*          mpCmdBind;

    QPushButton*    mpBtnOk;

private slots:
    void deviceClicked();
    void typeClicked();
    void checkReady();

private:
    void fillCombox(const QString &sql);

    bool mForEdit;

    QLineEdit*          mpEdtDeskPass;
    QLineEdit*          mpEdtNetCode;

};

}

#endif // BSLOGINERBASEINFODLG_H
