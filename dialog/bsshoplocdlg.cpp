#include "bsshoplocdlg.h"
#include "main/bailicode.h"

namespace BailiSoft {

BsShopLocDlg::BsShopLocDlg(QString &shop, QWidget *parent) : QDialog(parent)
{
    mpLng = new QLineEdit(this);
    mpLng->setValidator(new QDoubleValidator(70.000001, 150.000001, 6));
    mpLng->setAlignment(Qt::AlignRight);
    mpLng->setText("121.000001");

    mpLat = new QLineEdit(this);
    mpLat->setValidator(new QDoubleValidator(0.000001, 60.000001, 6));
    mpLat->setAlignment(Qt::AlignRight);
    mpLat->setText("31.000001");

    setStyleSheet("QLineEdit{padding:0 6px;}");

    QLabel *helper = new QLabel(this);
    helper->setText("<a style='color:green' href='https://api.map.baidu.com/lbsapi/getpoint/index.html'>"
                    "百度地图经纬度坐标拾取系统</a>");
    helper->setOpenExternalLinks(true);
    helper->setAlignment(Qt::AlignCenter);

    QGroupBox *form = new QGroupBox(this);
    form->setTitle(QStringLiteral("“%1”位置").arg(shop));
    QGridLayout *layGrid = new QGridLayout(form);

    layGrid->addWidget(new QLabel(QStringLiteral("东经：")), 0, 0);
    layGrid->addWidget(mpLng, 0, 1);
    layGrid->addWidget(new QLabel(QStringLiteral("精确到小数点后6位")), 0, 2);

    layGrid->addWidget(new QLabel(QStringLiteral("北纬：")), 1, 0);
    layGrid->addWidget(mpLat, 1, 1);
    layGrid->addWidget(new QLabel(QStringLiteral("精确到小数点后6位")), 1, 2);

    layGrid->addWidget(helper, 2, 0, 1, 3, Qt::AlignCenter);

    //确定取消
    QDialogButtonBox *pBox = new QDialogButtonBox(this);
    pBox->setOrientation(Qt::Horizontal);
    pBox->setCenterButtons(true);

    QPushButton *pBtnOk = pBox->addButton(mapMsg.value("btn_ok"), QDialogButtonBox::AcceptRole);
    pBtnOk->setIcon(QIcon(":/icon/ok.png"));
    pBtnOk->setIconSize(QSize(20, 20));
    connect(pBtnOk, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *pBtnCancel = pBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setIcon(QIcon(":/icon/cancel.png"));
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    pBtnCancel->setFocus();

    //layout
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(20, 20, 20, 30);
    lay->setSpacing(20);
    lay->addWidget(form, 1);
    lay->addWidget(pBox, 0, Qt::AlignCenter);

    setWindowTitle(QStringLiteral("设置门店地理位置"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
}

void BsShopLocDlg::openMapUrl(QString url)
{
    QDesktopServices::openUrl(QUrl(url));
}

}
