#include "bspricebatch.h"
#include "main/bailicode.h"

namespace BailiSoft {

// BsPriceBatchDlg ==========================================================================
BsPriceBatchDlg::BsPriceBatchDlg(QWidget *parent, const QString &sheetPrice, const QString &trader, const double regDis)
    : QDialog(parent), mSheetPrice(sheetPrice), mTrader(trader), mRegDis(regDis)
{
    //Left
    mpOptPolicy = new QRadioButton(QStringLiteral("登记政策"), this);
    mpOptRegDis = new QRadioButton(QStringLiteral("登记折扣"), this);
    mpOptValDis = new QRadioButton(QStringLiteral("特别折扣"), this);
    mpOptSetPrice = new QRadioButton(QStringLiteral("标牌价"), this);
    mpOptRetPrice = new QRadioButton(QStringLiteral("零售价"), this);
    mpOptLotPrice = new QRadioButton(QStringLiteral("批发价"), this);
    mpOptBuyPrice = new QRadioButton(QStringLiteral("进货价"), this);

    QGroupBox *grpOption = new QGroupBox(QStringLiteral("划价方式："));
    QVBoxLayout *layOption = new QVBoxLayout(grpOption);
    layOption->addWidget(mpOptPolicy);
    layOption->addWidget(mpOptRegDis);
    layOption->addWidget(mpOptValDis);
    layOption->addWidget(mpOptSetPrice);
    layOption->addWidget(mpOptRetPrice);
    layOption->addWidget(mpOptLotPrice);
    layOption->addWidget(mpOptBuyPrice);

    //Right
    mpEdtDiscount = new QLineEdit(this);
    mpEdtDiscount->setPlaceholderText(QStringLiteral("折扣值"));
    QDoubleValidator *val = new QDoubleValidator(0.000, 1.000, 3, this);
    val->setNotation(QDoubleValidator::StandardNotation);
    mpEdtDiscount->setValidator(val);
    mpEdtDiscount->setFixedWidth(100);

    mpLblRemark = new QLabel(this);
    mpLblRemark->setWordWrap(true);
    mpLblRemark->setMinimumWidth(300);
    mpLblRemark->setMaximumWidth(400);

    QDialogButtonBox *pBox = new QDialogButtonBox(this);
    pBox->setOrientation(Qt::Horizontal);
    pBox->setCenterButtons(true);

    QPushButton* pBtnOk = pBox->addButton(mapMsg.value("btn_ok"), QDialogButtonBox::AcceptRole);
    pBtnOk->setIcon(QIcon(":/icon/ok.png"));
    pBtnOk->setIconSize(QSize(20, 20));
    connect(pBtnOk, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *pBtnCancel = pBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setIcon(QIcon(":/icon/cancel.png"));
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    QWidget *pnlBody = new QWidget(this);
    QVBoxLayout *layBody = new QVBoxLayout(pnlBody);
    layBody->addWidget(mpEdtDiscount, 0, Qt::AlignCenter);
    layBody->addWidget(mpLblRemark, 1);
    layBody->addWidget(pBox, 0, Qt::AlignCenter);

    //layout
    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->addWidget(grpOption);
    lay->addWidget(pnlBody, 1);

    //win
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
    setWindowTitle(QStringLiteral("批量划价"));
    setMinimumSize(sizeHint());    

    connect(mpOptPolicy, SIGNAL(clicked(bool)), this, SLOT(optionClicked()));
    connect(mpOptRegDis, SIGNAL(clicked(bool)), this, SLOT(optionClicked()));
    connect(mpOptValDis, SIGNAL(clicked(bool)), this, SLOT(optionClicked()));
    connect(mpOptSetPrice, SIGNAL(clicked(bool)), this, SLOT(optionClicked()));
    connect(mpOptRetPrice, SIGNAL(clicked(bool)), this, SLOT(optionClicked()));
    connect(mpOptLotPrice, SIGNAL(clicked(bool)), this, SLOT(optionClicked()));
    connect(mpOptBuyPrice, SIGNAL(clicked(bool)), this, SLOT(optionClicked()));

    mpOptPolicy->setChecked(true);
    optionClicked();
}

QString BsPriceBatchDlg::getPriceType()
{
    if ( mpOptRegDis->isChecked() ) {
        return QStringLiteral("discount");
    }
    else if ( mpOptValDis->isChecked() ) {
        return mpEdtDiscount->text();
    }
    else if ( mpOptSetPrice->isChecked() ) {
        return QStringLiteral("setprice");
    }
    else if ( mpOptRetPrice->isChecked() ) {
        return QStringLiteral("retprice");
    }
    else if ( mpOptLotPrice->isChecked() ) {
        return QStringLiteral("lotprice");
    }
    else if ( mpOptBuyPrice->isChecked() ) {
        return QStringLiteral("buyprice");
    }
    return QString();
}

void BsPriceBatchDlg::optionClicked()
{
    mpEdtDiscount->hide();

    if ( mpOptRegDis->isChecked() ) {
        mpLblRemark->setText(QStringLiteral("按“%1”的登记折扣“%2”乘以%3，不使用价格政策。")
                             .arg(mTrader).arg(mRegDis, 0, 'f', 3).arg(mSheetPrice));
    }
    else if ( mpOptValDis->isChecked() ) {
        mpLblRemark->setText(QStringLiteral("指定固定折扣乘以%1，不使用价格政策。").arg(mSheetPrice));
        mpEdtDiscount->show();
    }
    else if ( mpOptSetPrice->isChecked() ) {
        mpLblRemark->setText(QStringLiteral("按标牌价，不使用折扣，不检查价格政策。"));
    }
    else if ( mpOptRetPrice->isChecked() ) {
        mpLblRemark->setText(QStringLiteral("按零售价，不使用折扣，不检查价格政策。"));
    }
    else if ( mpOptLotPrice->isChecked() ) {
        mpLblRemark->setText(QStringLiteral("按批发价，不使用折扣，不检查价格政策。"));
    }
    else if ( mpOptBuyPrice->isChecked() ) {
        mpLblRemark->setText(QStringLiteral("按进货价，不使用折扣，不检查价格政策。"));
    }
    else {
        mpLblRemark->setText(QStringLiteral("检查价格政策，查找适合“%1”的政策折扣乘以%2，如果没有匹配政策，"
                                            "则使用其登记缺省折扣“%3”相乘。这也是单据逐行录入时自动跳出的默认价格模式。")
                             .arg(mTrader).arg(mSheetPrice).arg(mRegDis, 0, 'f', 3));
    }
}

}

