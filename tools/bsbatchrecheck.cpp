#include "bsbatchrecheck.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailiedit.h"
#include "main/bailigrid.h"

namespace BailiSoft {

BsBatchReCheck::BsBatchReCheck(QWidget *parent) : QDialog(parent)
{
    //输入件
    mpOptTable = new QComboBox(this);
    mpOptTable->addItem(mapMsg.value("win_cgd").split(QChar(9)).at(0), QStringLiteral("cgd"));
    mpOptTable->addItem(mapMsg.value("win_cgj").split(QChar(9)).at(0), QStringLiteral("cgj"));
    mpOptTable->addItem(mapMsg.value("win_cgt").split(QChar(9)).at(0), QStringLiteral("cgt"));
    mpOptTable->addItem(mapMsg.value("win_pfd").split(QChar(9)).at(0), QStringLiteral("pfd"));
    mpOptTable->addItem(mapMsg.value("win_pff").split(QChar(9)).at(0), QStringLiteral("pff"));
    mpOptTable->addItem(mapMsg.value("win_pft").split(QChar(9)).at(0), QStringLiteral("pft"));
    mpOptTable->addItem(mapMsg.value("win_lsd").split(QChar(9)).at(0), QStringLiteral("lsd"));
    mpOptTable->addItem(mapMsg.value("win_dbd").split(QChar(9)).at(0), QStringLiteral("dbd"));
    mpOptTable->addItem(mapMsg.value("win_syd").split(QChar(9)).at(0), QStringLiteral("syd"));
    mpOptTable->addItem(mapMsg.value("win_szd").split(QChar(9)).at(0), QStringLiteral("szd"));
    mpOptTable->setMinimumWidth(200);
    connect(mpOptTable, QOverload<int>::of(&QComboBox::currentIndexChanged),
          [=](int index){ mpBtnExec->setEnabled(index >= 0); });

    mpOptCheckAll = new QRadioButton(QStringLiteral("审核"), this);
    connect(mpOptCheckAll, &QRadioButton::clicked, this, &BsBatchReCheck::checkReady);

    mpOptUnCheckAll = new QRadioButton(QStringLiteral("撤销审核"), this);
    connect(mpOptUnCheckAll, &QRadioButton::clicked, this, &BsBatchReCheck::checkReady);

    QWidget* pnlForm = new QWidget(this);
    QHBoxLayout* layForm = new QHBoxLayout(pnlForm);
    layForm->setContentsMargins(50, 20, 50, 20);
    layForm->setSpacing(50);
    layForm->addWidget(mpOptCheckAll);
    layForm->addWidget(mpOptUnCheckAll);

    //执行按钮
    mpBtnExec = new QPushButton(QStringLiteral("全部审核"), this);
    mpBtnExec->setFixedSize(100, 30);
    mpBtnExec->setEnabled(false);
    connect(mpBtnExec, &QPushButton::clicked, this, &BsBatchReCheck::doExec);

    //布局
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(50, 30, 50, 30);
    lay->addWidget(mpOptTable, 0, Qt::AlignCenter);
    lay->addWidget(pnlForm);
    lay->addSpacing(15);
    lay->addWidget(mpBtnExec, 0, Qt::AlignCenter);
    lay->addStretch();

    mpOptTable->setCurrentIndex(-1);
    mpOptCheckAll->setChecked(true);

    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
}

void BsBatchReCheck::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() != Qt::Key_Enter && e->key() != Qt::Key_Return )
        QDialog::keyPressEvent(e);
}

void BsBatchReCheck::checkReady()
{
    mpBtnExec->setText((mpOptCheckAll->isChecked()) ? QStringLiteral("全部审核") : QStringLiteral("全部撤销审核"));
    mpBtnExec->setEnabled(mpOptTable->currentIndex() >= 0);
}

void BsBatchReCheck::doExec()
{
    QString sql = (mpOptCheckAll->isChecked())
            ? QStringLiteral("update %1 set chktime=%2, checker='%3'")
              .arg(mpOptTable->currentData().toString())
              .arg(QDateTime::currentSecsSinceEpoch())
              .arg(loginer)
            : QStringLiteral("update %1 set chktime=0, checker='%2'")
              .arg(mpOptTable->currentData().toString())
              .arg(loginer);

    //执行
    QSqlDatabase db = QSqlDatabase::database();
    db.exec(sql);
    if ( db.lastError().isValid() )
        QMessageBox::information(this, QString(), QStringLiteral("%1不成功，您可联系软件www.bailisoft.com协助解决。")
                                 .arg(mpBtnExec->text()));
    else
        QMessageBox::information(this, QString(), QStringLiteral("%1成功！").arg(mpBtnExec->text()));
}

}
