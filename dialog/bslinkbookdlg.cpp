#include "bslinkbookdlg.h"
#include "main/bailicode.h"
#include "main/bailifunc.h"

namespace BailiSoft {

BsLinkBookDlg::BsLinkBookDlg(QWidget *parent) : QDialog(parent)
{
    mpBacker = new QLineEdit(this);

    mpFront = new QLineEdit(this);

    mpPassword = new QLineEdit(this);
    disableEditInputMethod(mpPassword);

    QWidget *form = new QWidget(this);
    QFormLayout *layForm = new QFormLayout(form);
    layForm->addRow(QStringLiteral("公司名："), mpBacker);
    layForm->addRow(QStringLiteral("用户名："), mpFront);
    layForm->addRow(QStringLiteral("验证码："), mpPassword);

    //确定取消
    QDialogButtonBox *pBox = new QDialogButtonBox(this);
    pBox->setOrientation(Qt::Horizontal);

    mpBtnOk = pBox->addButton(mapMsg.value("btn_ok"), QDialogButtonBox::AcceptRole);
    mpBtnOk->setIcon(QIcon(":/icon/ok.png"));
    mpBtnOk->setIconSize(QSize(20, 20));
    mpBtnOk->setEnabled(false);
    connect(mpBtnOk, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *pBtnCancel = pBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setIcon(QIcon(":/icon/cancel.png"));
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    //layout
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(form, 1);
    lay->addWidget(pBox);
    setWindowTitle(QStringLiteral("连接网络账册"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);

    connect(mpBacker, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(mpFront, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(mpPassword, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
}

void BsLinkBookDlg::updateOkButtonState()
{
    mpBtnOk->setEnabled(
                !mpBacker->text().isEmpty() &&
                !mpFront->text().isEmpty() &&
                !mpPassword->text().isEmpty() &&
                mpBacker->text().indexOf(QChar('/')) < 0
                );
}

}
