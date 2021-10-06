#include "bscreatecolortype.h"
#include "main/bailicode.h"
#include "main/bailifunc.h"
#include "comm/bsflowlayout.h"

#include <QtSql>

namespace BailiSoft {

BsCreateColorTypeDlg::BsCreateColorTypeDlg(const QString &cargo, QWidget *parent)
    : QDialog(parent), mCargo(cargo)
{
    setMinimumSize(600, 200);
    setWindowTitle(QStringLiteral("组合创建新色系"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
    setStyleSheet(QLatin1String("QCheckBox:checked{color:red;}"));

    mpEdtName = new QLineEdit(this);
    mpEdtName->setText(cargo);

    QHBoxLayout *layHeader = new QHBoxLayout;
    layHeader->addStretch();
    layHeader->addWidget(new QLabel(QStringLiteral("色系名："), this));
    layHeader->addWidget(mpEdtName);
    layHeader->addStretch();

    mpGrpAll = new QGroupBox(this);
    mpGrpAll->setTitle(QStringLiteral("所有颜色（自由勾选创建）"));
    mpLayAll = new BsFlowLayout(mpGrpAll);

    mpEdtNew = new QLineEdit(this);
    mpEdtNew->setPlaceholderText(QStringLiteral("新色入库（各色项用逗号分开，如有色号紧挨色名前）"));
    int w = mpEdtNew->sizeHint().height();

    mpBtnNew = new QToolButton(this);
    mpBtnNew->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mpBtnNew->setIcon(QIcon(":/icon/up.png"));
    mpBtnNew->setFixedSize(w, w);

    QHBoxLayout *layNew = new QHBoxLayout;
    layNew->setSpacing(1);
    layNew->addWidget(mpEdtNew, 1);
    layNew->addWidget(mpBtnNew);

    mpBtnOK      = new QPushButton(this);
    mpBtnOK->setText(mapMsg.value("btn_ok"));

    mpBtnCancel  = new QPushButton(this);
    mpBtnCancel->setText(mapMsg.value("btn_cancel"));
    mpBtnCancel->setDefault(true);

    QHBoxLayout *layButtons  = new QHBoxLayout;
    layButtons->addStretch();
    layButtons->addWidget(mpBtnOK);
    layButtons->addWidget(mpBtnCancel);
    layButtons->addStretch();

    QVBoxLayout *lay   = new QVBoxLayout(this);
    lay->setContentsMargins(10, 20, 10, 20);
    lay->addLayout(layHeader);
    lay->addWidget(mpGrpAll, 1);
    lay->addLayout(layNew);
    lay->addSpacing(20);
    lay->addLayout(layButtons);

    loadReady();

    connect(mpBtnNew, &QToolButton::clicked, this, &BsCreateColorTypeDlg::onClickNew);
    connect(mpBtnOK, &QPushButton::clicked, this, &BsCreateColorTypeDlg::onClickAccept);
    connect(mpBtnCancel, &QPushButton::clicked, this, &BsCreateColorTypeDlg::reject);
}

void BsCreateColorTypeDlg::loadReady()
{
    mapColors.clear();
    QSqlQuery qry;
    qry.setForwardOnly(true);

    //提取去重
    QString sql = QStringLiteral("select codename from colorbase;");
    qry.exec(sql);
    while ( qry.next() ) {
        QString codeName = qry.value(0).toString();
        if ( !mapColors.contains(codeName) ) {
            mapColors.insert(codeName, QString());
        }
    }
    sql = QStringLiteral("select namelist, codelist from colortype;");
    qry.exec(sql);
    while ( qry.next() ) {
        QStringList names = qry.value(0).toString().split(QChar(','));
        QStringList codes = qry.value(1).toString().split(QChar(','));
        for ( int i = 0, iLen = names.length(); i < iLen; ++i ) {
            QString name = names.at(i);
            QString code = ( i < codes.length() ) ? codes.at(i) : QString();
            if ( !mapColors.contains(name) || mapColors.value(name).isEmpty() ) {
                mapColors.insert(name, code);
            }
        }
    }

    //排序
    QStringList colors;
    QMapIterator<QString, QString> itColor(mapColors);
    while ( itColor.hasNext() ) {
        itColor.next();
        QString name = itColor.key();
        colors << name;
    }
    std::sort(colors.begin(), colors.end());

    //生成
    for ( int i = 0, iLen = colors.length(); i < iLen; ++i ) {
        QString name = colors.at(i);
        QString code = mapColors.value(name);

        QCheckBox *chk = new QCheckBox(name, mpGrpAll);
        chk->setToolTip(code);
        mpLayAll->addWidget(chk);
    }
}

void BsCreateColorTypeDlg::onClickNew()
{
    QStringList sqls;
    QString txt = mpEdtNew->text();
    txt.replace(QStringLiteral("，"), QStringLiteral(",")).replace(QChar(39), QString());
    QStringList txts = txt.split(QChar(','));

    for ( int i = 0, iLen = txts.length(); i < iLen; ++i ) {
        QString codeName = txts.at(i);
        sqls << QStringLiteral("insert or ignore into colorbase(codename) values('%1');").arg(codeName);

        if ( !mapColors.contains(codeName) ) {
            QCheckBox *chk = new QCheckBox(codeName, mpGrpAll);
            mpLayAll->addWidget(chk);
            mapColors.insert(codeName, QString());
        }
    }

    QString sqlErr = sqliteCommit(sqls);
    if ( sqlErr.isEmpty() )
        mpEdtNew->clear();
    else
        qDebug() << sqlErr;
}

void BsCreateColorTypeDlg::onClickAccept()
{
    if ( mpEdtName->text().trimmed().isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("请给色系命名！"));
        mpEdtName->setFocus();
        return;
    }

    QList<QCheckBox *> chks = mpGrpAll->findChildren<QCheckBox *>();
    for ( int i = 0, iLen = chks.length(); i < iLen; ++i ) {
        QCheckBox *chk = chks.at(i);
        if ( chk->isChecked() ) {
            mPickeds << chk->text();
        }
    }

    if ( mPickeds.length() > 0 )
        accept();
    else
        QMessageBox::information(this, QString(), QStringLiteral("构建色系需要至少一个色项！"));
}

}
