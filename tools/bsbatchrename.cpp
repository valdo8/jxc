#include "bsbatchrename.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailiedit.h"
#include "main/bailigrid.h"

namespace BailiSoft {

BsBatchRename::BsBatchRename(QWidget *parent) : QDialog(parent)
{
    //输入件（currentIndex顺序有约定）
    mpRegTable = new QComboBox(this);
    mpRegTable->addItem(mapMsg.value("win_customer").split(QChar(9)).at(0), QStringLiteral("customer"));
    mpRegTable->addItem(mapMsg.value("win_supplier").split(QChar(9)).at(0), QStringLiteral("supplier"));
    mpRegTable->addItem(mapMsg.value("win_shop").split(QChar(9)).at(0), QStringLiteral("shop"));
    mpRegTable->addItem(mapMsg.value("win_staff").split(QChar(9)).at(0), QStringLiteral("staff"));
    mpRegTable->addItem(mapMsg.value("win_subject").split(QChar(9)).at(0), QStringLiteral("subject"));
    mpRegTable->addItem(mapMsg.value("win_cargo").split(QChar(9)).at(0), QStringLiteral("cargo"));
    mpRegTable->addItem(mapMsg.value("win_colortype").split(QChar(9)).at(0), QStringLiteral("color"));
    mpRegTable->addItem(mapMsg.value("win_sizertype").split(QChar(9)).at(0), QStringLiteral("sizers"));
    mpRegTable->setCurrentIndex(0);
    mpRegTable->setMinimumWidth(200);
    connect(mpRegTable, SIGNAL(currentIndexChanged(int)), this, SLOT(tableIndexChanged(int)));

    mpEdtOld = new QLineEdit(this);
    connect(mpEdtOld, &QLineEdit::textChanged, this, &BsBatchRename::checkReady);

    mpEdtNew = new QLineEdit(this);
    connect(mpEdtNew, &QLineEdit::textChanged, this, &BsBatchRename::checkReady);

    mpCargoField = new BsField(QStringLiteral("cargo"), QString(), 0, 20, QString());
    mpConCargo = new BsFldEditor(this, mpCargoField, dsCargo);

    //form
    QFormLayout *layForm = new QFormLayout;
    layForm->addRow(QStringLiteral("更新对象"), mpRegTable);
    layForm->addRow(QStringLiteral("原值"), mpEdtOld);
    layForm->addRow(QStringLiteral("新值"), mpEdtNew);
    layForm->addRow(QStringLiteral("限制货号"), mpConCargo);
    mpLblConCargo = layForm->labelForField(mpConCargo);
    mpConCargo->hide();
    mpLblConCargo->hide();

    //执行按钮
    mpBtnExec = new QPushButton(mapMsg.value("word_execute"), this);
    mpBtnExec->setFixedSize(100, 30);
    mpBtnExec->setEnabled(false);
    connect(mpBtnExec, SIGNAL(clicked(bool)), this, SLOT(doExec()));

    //布局
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(50, 20, 60, 20);
    lay->addLayout(layForm);
    lay->addSpacing(15);
    lay->addWidget(mpBtnExec, 0, Qt::AlignCenter);
    lay->addStretch();

    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
}

BsBatchRename::~BsBatchRename()
{
    delete mpCargoField;
}

void BsBatchRename::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() != Qt::Key_Enter && e->key() != Qt::Key_Return )
        QDialog::keyPressEvent(e);
}

void BsBatchRename::tableIndexChanged(int)
{
    QString tbl = mpRegTable->currentData().toString();
    bool colorr = tbl == QStringLiteral("color") || tbl == QStringLiteral("sizer");
    mpConCargo->setVisible(colorr);
    mpLblConCargo->setVisible(colorr);
}

void BsBatchRename::checkReady()
{
    mpBtnExec->setEnabled(!mpEdtOld->text().isEmpty() || !mpEdtNew->text().isEmpty());
}

void BsBatchRename::doExec()
{
    bool needConCargo = ( mpRegTable->currentIndex() == 6 );

    mpConCargo->setVisible(needConCargo);
    mpLblConCargo->setVisible(needConCargo);

    if ( !(mpRegTable->currentIndex() >= 0 &&
                           !mpEdtOld->text().isEmpty() &&
                           !mpEdtNew->text().isEmpty() &&
                           (!needConCargo || !mpConCargo->text().isEmpty())
                         ) ) {
        QMessageBox::information(this, QString(), QStringLiteral("请填写完整！"));
        return;
    }

    QStringList sqls;

    QStringList tbls;
    tbls << "cgd" << "cgj" << "cgt" << "pfd" << "pff" << "pft" << "lsd" << "dbd" << "syd";  //注意不要含szd

    QString strOld = mpEdtOld->text();
    QString strNew = mpEdtNew->text();
    strOld.replace(QChar(39), QChar(8217));
    strNew.replace(QChar(39), QChar(8217));

    switch (mpRegTable->currentIndex()) {
    //customer
    case 0:
        sqls << QStringLiteral("update pfd set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        sqls << QStringLiteral("update pff set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        sqls << QStringLiteral("update pft set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        sqls << QStringLiteral("update lsd set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        sqls << QStringLiteral("update szd set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        break;
    //supplier
    case 1:
        sqls << QStringLiteral("update cgd set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        sqls << QStringLiteral("update cgj set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        sqls << QStringLiteral("update cgt set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        sqls << QStringLiteral("update szd set trader='%1' where trader='%2';").arg(strNew).arg(strOld);
        break;
    //shop, staff
    case 2:
    case 3:
        for ( int i = 0, iLen = tbls.length(); i < iLen; ++i ) {
            sqls << QStringLiteral("update %1 set %2='%3' where %2='%4';")
                    .arg(tbls.at(i)).arg(mpRegTable->currentData().toString()).arg(strNew).arg(strOld);
        }
        sqls << QStringLiteral("update szd set %1='%2' where %1='%3';")
                .arg(mpRegTable->currentData().toString()).arg(strNew).arg(strOld);
        break;

    //subject
    case 4:
        sqls << QStringLiteral("update szddtl set subject='%1' where subject='%2'").arg(strNew).arg(strOld);
        break;

    //cargo
    case 5:
        for ( int i = 0, iLen = tbls.length(); i < iLen; ++i ) {
            QString sql = QStringLiteral("update %1dtl set %2='%3' where %2='%4';")
                    .arg(tbls.at(i)).arg(mpRegTable->currentData().toString()).arg(strNew).arg(strOld);
            sqls << sql;
        }
        break;
    //color
    case 6:
        for ( int i = 0, iLen = tbls.length(); i < iLen; ++i ) {
            QString cargoCon = (mpConCargo->text().isEmpty())
                    ? QString()
                    : QStringLiteral(" and cargo='%1' ").arg(mpConCargo->text());
            QString sql = QStringLiteral("update %1dtl set %2='%3' where %2='%4' %5;")
                    .arg(tbls.at(i)).arg(mpRegTable->currentData().toString())
                    .arg(strNew).arg(strOld).arg(cargoCon);
            sqls << sql;
        }
        break;
    //sizers
    default:
        for ( int i = 0, iLen = tbls.length(); i < iLen; ++i ) {
            QString cargoCon = (mpConCargo->text().isEmpty())
                    ? QString()
                    : QStringLiteral(" and cargo='%1'").arg(mpConCargo->text());

            sqls << QStringLiteral("update %1dtl set sizers = "
                                   "substr(sizers, 1, instr(('\n' || sizers), '\n%3\t') - 1) || '%2' || "
                                   "substr(sizers, instr(('\n' || sizers), '\n%3\t') + %4) "
                                   "where ('\n' || sizers) like '%\n%3\t%' %5;")
                    .arg(tbls.at(i)).arg(strNew).arg(strOld).arg(strOld.length()).arg(cargoCon);
        }
        break;
    }

    //shop补充调拨单trader
    if ( mpRegTable->currentIndex() == 2 )
        sqls << QStringLiteral("update dbd set trader='%1' where trader='%2';").arg(strNew).arg(strOld);

    //执行
    QString sqlErr = sqliteCommit(sqls);
    if ( sqlErr.isEmpty() )
        QMessageBox::information(this, QString(), QStringLiteral("更改成功！"));
    else
        QMessageBox::information(this, QString(), QStringLiteral("更改不成功，您可联系软件www.bailisoft.com协助解决。"));
}

}
