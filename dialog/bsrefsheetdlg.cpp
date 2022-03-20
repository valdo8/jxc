#include "bsrefsheetdlg.h"
#include "main/bailicode.h"

namespace BailiSoft {

BsRefSheetDlg::BsRefSheetDlg(const qint64 amount, QStringList &subjects, QWidget *parent)
    : QDialog(parent), mAmount(amount), mSubjects(subjects)
{
    mpForm = new QGroupBox(this);
    mpForm->setTitle(QStringLiteral("总金额：%1").arg(bsNumForRead(amount, 2)));
    QFormLayout *layForm = new QFormLayout(mpForm);

    mpValidator = new QDoubleValidator(this);
    if ( amount > 0 ) {
        mpValidator->setRange(0, amount / 10000.0, 2);
    } else {
        mpValidator->setRange(amount / 10000.0, 0, 2);
    }

    for ( int i = 0, iLen = subjects.length(); i < iLen; ++i ) {

        QLabel *lbl = new QLabel(subjects.at(i), this);

        QLineEdit *edt = new QLineEdit(this);
        edt->setValidator(mpValidator);
        connect(edt, &QLineEdit::textChanged, this, &BsRefSheetDlg::edtTextChanged);
        connect(edt, &QLineEdit::returnPressed, this, &BsRefSheetDlg::edtReturnPressed);

        layForm->addRow(lbl, edt);
    }

    mpLblRemaining = new QLabel(this);

    //确定取消
    QDialogButtonBox *pBox = new QDialogButtonBox(this);
    pBox->setOrientation(Qt::Horizontal);
    pBox->setCenterButtons(true);

    mpBtnOk = pBox->addButton(mapMsg.value("btn_ok"), QDialogButtonBox::AcceptRole);
    mpBtnOk->setIcon(QIcon(":/icon/ok.png"));
    mpBtnOk->setIconSize(QSize(20, 20));
    mpBtnOk->setEnabled(false);
    connect(mpBtnOk, &QPushButton::clicked, this, &BsRefSheetDlg::acceptOk);

    QPushButton *pBtnCancel = pBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setIcon(QIcon(":/icon/cancel.png"));
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, &QPushButton::clicked, this, &BsRefSheetDlg::reject);

    pBtnCancel->setFocus();

    //layout
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(20, 20, 20, 30);
    lay->setSpacing(5);
    lay->addWidget(mpForm, 1);
    lay->addWidget(mpLblRemaining, 0, Qt::AlignCenter);
    lay->addSpacing(20);
    lay->addWidget(pBox, 0, Qt::AlignCenter);

    setWindowTitle(QStringLiteral("金额记账"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
}

void BsRefSheetDlg::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    edtTextChanged(QString());
}

void BsRefSheetDlg::edtTextChanged(const QString &)
{
    qint64 sum = 0.0;
    for ( int i = 0, iLen = mpForm->children().length(); i < iLen; ++i ) {
        QLineEdit *edt = qobject_cast<QLineEdit*>(mpForm->children().at(i));
        if ( edt ) {
            sum += bsNumForSave(edt->text().toDouble()).toLongLong();
        }
    }
    qint64 diff = mAmount - sum;
    mpLblRemaining->setText(QStringLiteral("未分配余额：%1").arg(diff / 10000.0, 0, 'f', 2));
    mpBtnOk->setEnabled(diff == 0);
}

void BsRefSheetDlg::edtReturnPressed()
{
    QList<QLineEdit *> edts;
    for ( int i = 0, iLen = mpForm->children().length(); i < iLen; ++i ) {
        QLineEdit *edt = qobject_cast<QLineEdit*>(mpForm->children().at(i));
        if ( edt ) {
            edts << edt;
        }
    }

    int idx = -1;
    for ( int i = 0, iLen = edts.length(); i < iLen; ++i ) {
        if (edts.at(i) == QObject::sender()) {
            idx = i;
            break;
        }
    }

    if ( idx < edts.length() - 1 ) {
        edts.at(idx + 1)->setFocus();
    }
}

void BsRefSheetDlg::acceptOk()
{
    mAssigns.clear();
    for ( int i = 0, iLen = mpForm->children().length(); i < iLen; ++i ) {
        QLineEdit *edt = qobject_cast<QLineEdit*>(mpForm->children().at(i));
        if ( edt ) {
            mAssigns << bsNumForSave(edt->text().toDouble()).toLongLong();
        }
    }

    accept();
}

}
