#include "bsloginerbaseinfodlg.h"
#include "main/bailicode.h"
#include "main/bailiedit.h"
#include "main/bailifunc.h"

#include <QtSql>

namespace BailiSoft {

// BsLoginerBaseInfoDlg ==========================================================================
BsLoginerBaseInfoDlg::BsLoginerBaseInfoDlg(QWidget *parent, const bool forEdit)
    : QDialog(parent), mForEdit(forEdit)
{
    const int field_max_width = 380;

    //用户名
    QLabel* lblName = new QLabel(QStringLiteral("用户名"));
    mpEdtName = new QLineEdit(this);
    mpEdtName->setMaxLength(8);
    mpEdtName->setMinimumWidth(field_max_width);

    mpLblPassword = new QLabel(QStringLiteral("登录密码"));

    mpLblIcon = new QLabel(this);
    mpLblIcon->setFixedSize(64, 64);
    mpLblIcon->setPixmap(QPixmap(":/icon/share.png").scaled(64, 64));

    //后台登录密码
    mpEdtDeskPass = new QLineEdit(this);
    mpEdtDeskPass->setMaxLength(16);
    mpEdtDeskPass->setMinimumWidth(field_max_width);
    mpEdtDeskPass->setAttribute(Qt::WA_InputMethodEnabled, false);

    //前台登录密码
    mpEdtNetCode = new QLineEdit(this);
    mpEdtNetCode->setMaxLength(16);
    mpEdtNetCode->setMinimumWidth(field_max_width);
    mpEdtNetCode->setAttribute(Qt::WA_InputMethodEnabled, false);

    //布局
    QGroupBox* grpForm = new QGroupBox(this);
    QVBoxLayout* layForm = new QVBoxLayout(grpForm);
    layForm->addWidget(lblName, 0, Qt::AlignLeft | Qt::AlignBottom);
    layForm->addWidget(mpEdtName, 0, Qt::AlignCenter);
    layForm->addSpacing(5);
    layForm->addWidget(mpLblPassword, 0, Qt::AlignLeft | Qt::AlignBottom);
    layForm->addWidget(mpEdtDeskPass, 0, Qt::AlignCenter);
    layForm->addWidget(mpEdtNetCode, 0, Qt::AlignCenter);

    QWidget *pnlAccount = new QWidget(this);
    QHBoxLayout *layAccount = new QHBoxLayout(pnlAccount);
    layAccount->setContentsMargins(0, 0, 0, 0);
    layAccount->addWidget(grpForm, 1);
    layAccount->addSpacing(10);
    layAccount->addWidget(mpLblIcon, 0, Qt::AlignCenter);

    //设备类型
    mpDeviceLocal      = new QRadioButton(QStringLiteral("本机或内网共享"), this);
    mpDeviceRemotePc   = new QRadioButton(QStringLiteral("外网电脑或笔记本"), this);
    mpDeviceRemoteMob  = new QRadioButton(QStringLiteral("外网手机或平板"), this);
    mpGrpDevice = new QGroupBox(QStringLiteral("登录设备"), this);
    QHBoxLayout* layDevice = new QHBoxLayout(mpGrpDevice);
    layDevice->setContentsMargins(30, 10, 30, 10);
    layDevice->setSpacing(30);
    layDevice->addWidget(mpDeviceLocal);
    layDevice->addWidget(mpDeviceRemotePc);
    layDevice->addWidget(mpDeviceRemoteMob);

    //帐号类型
    mpTypeBacker      = new QRadioButton(QStringLiteral("总部使用"), this);
    mpTypeShop      = new QRadioButton(QStringLiteral("门店使用"), this);
    mpTypeCustomer  = new QRadioButton(QStringLiteral("客户使用"), this);
    mpGrpType = new QGroupBox(QStringLiteral("账号类型"), this);
    QHBoxLayout* layType = new QHBoxLayout(mpGrpType);
    layType->setContentsMargins(30, 10, 30, 10);
    layType->setSpacing(30);
    layType->addWidget(mpTypeBacker);
    layType->addWidget(mpTypeShop);
    layType->addWidget(mpTypeCustomer);

    //绑定
    mpCmdBind = new QComboBox(this);
    mpCmdBind->setMinimumWidth(200);

    mpGrpBind = new QGroupBox(QStringLiteral("总部不设绑定"), this);
    QVBoxLayout* layBind = new QVBoxLayout(mpGrpBind);
    layBind->addWidget(mpCmdBind, 0, Qt::AlignCenter);

    //确定取消
    QDialogButtonBox *pBox = new QDialogButtonBox(this);
    pBox->setOrientation(Qt::Horizontal);
    pBox->setCenterButtons(true);

    mpBtnOk = pBox->addButton(mapMsg.value("btn_ok"), QDialogButtonBox::AcceptRole);
    mpBtnOk->setIcon(QIcon(":/icon/ok.png"));
    mpBtnOk->setIconSize(QSize(20, 20));
    connect(mpBtnOk, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *pBtnCancel = pBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setIcon(QIcon(":/icon/cancel.png"));
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    //布局
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->addWidget(pnlAccount, 1, Qt::AlignCenter);
    lay->addWidget(mpGrpDevice, 0, Qt::AlignCenter);
    lay->addWidget(mpGrpType, 0, Qt::AlignCenter);
    lay->addWidget(mpGrpBind, 0, Qt::AlignCenter);
    lay->addSpacing(20);
    lay->addWidget(pBox, 0, Qt::AlignCenter);

    //初始
    mpBtnOk->setEnabled(false);
    mpCmdBind->setEnabled(false);
    mpDeviceLocal->setChecked(true);
    mpTypeBacker->setChecked(true);
    mpEdtNetCode->hide();

    mpGrpDevice->setMinimumWidth(pnlAccount->sizeHint().width());
    mpGrpType->setMinimumWidth(pnlAccount->sizeHint().width());
    mpGrpBind->setMinimumWidth(pnlAccount->sizeHint().width());

    if ( forEdit ) {
        mpEdtName->setEnabled(false);
        mpGrpType->hide();
        mpGrpBind->hide();
    }

    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
    setWindowTitle(QStringLiteral("添加用户"));
    setMinimumSize(sizeHint());

    connect(mpDeviceLocal, SIGNAL(clicked(bool)), this, SLOT(deviceClicked()));
    connect(mpDeviceRemotePc, SIGNAL(clicked(bool)), this, SLOT(deviceClicked()));
    connect(mpDeviceRemoteMob, SIGNAL(clicked(bool)), this, SLOT(deviceClicked()));

    connect(mpTypeBacker, SIGNAL(clicked(bool)), this, SLOT(typeClicked()));
    connect(mpTypeShop, SIGNAL(clicked(bool)), this, SLOT(typeClicked()));
    connect(mpTypeCustomer, SIGNAL(clicked(bool)), this, SLOT(typeClicked()));

    connect(mpEdtName, SIGNAL(textChanged(QString)), this, SLOT(checkReady()));
    connect(mpEdtDeskPass, SIGNAL(textChanged(QString)), this, SLOT(checkReady()));
    connect(mpEdtNetCode, SIGNAL(textChanged(QString)), this, SLOT(checkReady()));
}

void BsLoginerBaseInfoDlg::setPasswordAndDeviceType(const QString &deskPass, const QString &netCode,
                                                    const QString &bindShop, const QString &bindCustomer)
{
    mpEdtDeskPass->setText(deskPass);
    mpEdtNetCode->setText(netCode);

    mpEdtDeskPass->setVisible( netCode.isEmpty() );
    mpEdtNetCode->setVisible( !netCode.isEmpty() );

    if ( netCode.isEmpty() ) {
        mpDeviceLocal->setChecked(true);
    } else {
        if ( deskPass.isEmpty() ) {
            mpDeviceRemoteMob->setChecked(true);
        } else {
            mpDeviceRemotePc->setChecked(true);
        }
        if ( !bindCustomer.isEmpty() ) {
            mpTypeCustomer->setChecked(true);
        }
        else if ( !bindShop.isEmpty() ) {
            mpTypeShop->setChecked(true);
        }
        else {
            mpTypeBacker->setChecked(true);
        }
    }

    checkReady();
}

void BsLoginerBaseInfoDlg::deviceClicked()
{
    if ( mpDeviceLocal->isChecked() ) {
        mpEdtDeskPass->clear();
        mpEdtNetCode->clear();

        mpLblPassword->setText(QStringLiteral("后台直接打开账册的登录密码"));
        mpLblPassword->setStyleSheet(QLatin1String());
        mpEdtDeskPass->show();
        mpEdtNetCode->hide();
    }

    if ( mpDeviceRemotePc->isChecked() ) {
        mpEdtDeskPass->setText(generateReadableRandomString(16));   //约定为PC
        mpEdtNetCode->clear();

        mpLblPassword->setText(QStringLiteral("前台登录密码"));
        mpLblPassword->setStyleSheet(QLatin1String("color:red;"));
        mpEdtDeskPass->hide();
        mpEdtNetCode->show();
    }

    if ( mpDeviceRemoteMob->isChecked() ) {
        mpEdtDeskPass->clear();                                     //约定为移动设备
        mpEdtNetCode->clear();

        mpLblPassword->setText(QStringLiteral("前台登录密码"));
        mpLblPassword->setStyleSheet(QLatin1String("color:red;"));
        mpEdtDeskPass->hide();
        mpEdtNetCode->show();
    }

    checkReady();
}

void BsLoginerBaseInfoDlg::typeClicked()
{
    if ( mpTypeBacker->isChecked() ) {
        mpGrpBind->setTitle(QStringLiteral("总部不设绑定"));
        mpCmdBind->clear();
        mpCmdBind->setEnabled(false);

        mpDeviceLocal->setEnabled(true);
    }

    if ( mpTypeShop->isChecked() ) {
        mpGrpBind->setTitle(QStringLiteral("绑定门店"));
        mpCmdBind->setEnabled(true);
        fillCombox(QStringLiteral("select kname from shop order by kname;"));

        mpDeviceLocal->setEnabled(false);
        mpDeviceRemoteMob->setChecked(true);

        mpLblPassword->setText(QStringLiteral("前台登录密码"));
        mpLblPassword->setStyleSheet(QLatin1String("color:red;"));
    }

    if ( mpTypeCustomer->isChecked() ) {
        mpGrpBind->setTitle(QStringLiteral("绑定客户"));
        mpCmdBind->setEnabled(true);
        fillCombox(QStringLiteral("select kname from customer order by kname;"));

        mpDeviceLocal->setEnabled(false);
        mpDeviceRemoteMob->setChecked(true);

        mpLblPassword->setText(QStringLiteral("前台登录密码"));
        mpLblPassword->setStyleSheet(QLatin1String("color:red;"));
    }

    checkReady();
}

void BsLoginerBaseInfoDlg::checkReady()
{
    if ( mpDeviceLocal->isChecked() ) {
        mpLblIcon->setPixmap(QPixmap(":/icon/share.png").scaled(64, 64));
    }
    else if ( mpDeviceRemotePc->isChecked() ) {
        if ( mpTypeBacker->isChecked() ) {
            mpLblIcon->setPixmap(QPixmap(":/icon/pcoffice.png").scaled(64, 64));
        }
        else if ( mpTypeShop->isChecked() ) {
            mpLblIcon->setPixmap(QPixmap(":/icon/pcshop.png").scaled(64, 64));
        }
        else {
            mpLblIcon->setPixmap(QPixmap(":/icon/pcguest.png").scaled(64, 64));
        }
    }
    else {
        if ( mpTypeBacker->isChecked() ) {
            mpLblIcon->setPixmap(QPixmap(":/icon/mobioffice.png").scaled(64, 64));
        }
        else if ( mpTypeShop->isChecked() ) {
            mpLblIcon->setPixmap(QPixmap(":/icon/mobishop.png").scaled(64, 64));
        }
        else {
            mpLblIcon->setPixmap(QPixmap(":/icon/mobiguest.png").scaled(64, 64));
        }
    }

    bool nameValid = !mpEdtName->text().isEmpty();
    bool bindOk = mpCmdBind->currentIndex() >= 0;
    bool deskPwdOk = !mpEdtDeskPass->text().isEmpty();
    bool netPwdOk = !mpEdtNetCode->text().isEmpty();

    if ( mForEdit ) {
        mpBtnOk->setEnabled(true);
    }
    else {
        if ( mpDeviceLocal->isChecked() ) {
            if ( mpTypeBacker->isChecked() )
                mpBtnOk->setEnabled(nameValid && deskPwdOk);
            else
                mpBtnOk->setEnabled(nameValid && deskPwdOk && bindOk);
        } else {
            if ( mpTypeBacker->isChecked() )
                mpBtnOk->setEnabled(nameValid && netPwdOk);
            else
                mpBtnOk->setEnabled(nameValid && netPwdOk && bindOk);
        }
    }
}

void BsLoginerBaseInfoDlg::fillCombox(const QString &sql)
{
    disconnect(mpCmdBind, SIGNAL(currentIndexChanged(int)), nullptr, nullptr);

    mpCmdBind->clear();
    QSqlQuery qry;
    qry.exec(sql);
    while ( qry.next() ) {
        mpCmdBind->addItem(qry.value(0).toString());
    }
    qry.finish();
    mpCmdBind->setCurrentIndex(-1);

    connect(mpCmdBind, SIGNAL(currentIndexChanged(int)), this, SLOT(checkReady()));
}

}

