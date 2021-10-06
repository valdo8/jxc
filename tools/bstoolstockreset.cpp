#include "bstoolstockreset.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailiedit.h"
#include "main/bailigrid.h"

namespace BailiSoft {

BsToolStockReset::BsToolStockReset(QWidget *parent) : QDialog(parent)
{
    mpFldShop = new BsField(QStringLiteral("cargo"), QString(), 0, 20, QString());

    QStringList defs = mapMsg.value(QStringLiteral("fld_dated")).split(QChar(9));
    Q_ASSERT(defs.count() > 4);
    mpFldDate = new BsField("dated",
                            defs.at(0),
                            QString(defs.at(3)).toUInt(),
                            QString(defs.at(4)).toInt(),
                            defs.at(2));

    mpEdtShop = new BsFldEditor(this, mpFldShop, dsShop);
    mpEdtShop->setMinimumWidth(200);
    connect(mpEdtShop, &BsFldEditor::textChanged, this, &BsToolStockReset::checkReady);

    mpEdtDate = new BsFldEditor(this, mpFldDate, nullptr);
    mpEdtDate->setMinimumWidth(200);
    connect(mpEdtDate, &BsFldEditor::textChanged, this, &BsToolStockReset::checkReady);

    mpBtnExec = new QPushButton(mapMsg.value("word_execute"), this);
    mpBtnExec->setFixedSize(100, 30);
    mpBtnExec->setEnabled(false);
    connect(mpBtnExec, SIGNAL(clicked(bool)), this, SLOT(doExec()));

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(90, 30, 90, 30);
    lay->addWidget(new QLabel(QStringLiteral("盘点门店：")));
    lay->addWidget(mpEdtShop);
    lay->addSpacing(10);
    lay->addWidget(new QLabel(QStringLiteral("清零日期：")));
    lay->addWidget(mpEdtDate);
    lay->addSpacing(30);
    lay->addWidget(mpBtnExec, 0, Qt::AlignCenter);

    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
}

void BsToolStockReset::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() != Qt::Key_Enter && e->key() != Qt::Key_Return )
        QDialog::keyPressEvent(e);
}

void BsToolStockReset::checkReady()
{
    QDate dt = QDate::fromString(mpEdtDate->text(), QStringLiteral("yyyy-MM-dd"));
    mpBtnExec->setEnabled(mpEdtShop->currentTextRegValid() && dt.isValid());
}

void BsToolStockReset::doExec()
{
    //求newSheetId
    int newSheetId = 0;
    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.exec(QStringLiteral("select seq from sqlite_sequence where name='syd';"));
    if ( qry.next() ) {
        newSheetId = qry.value(0).toInt();
    }
    newSheetId++;

    qint64 rowtime = QDateTime::currentMSecsSinceEpoch();

    //总
    QStringList sqls;
    qint64 sumqty = 0;

    //读库存
    QString shop = mpEdtShop->getDataValueForSql();
    qint64 dated = mpEdtDate->getDataValueForSql().toLongLong();
    QString sql = QStringLiteral("select cargo, color, group_concat(sizers, '') as sizers "
                                 "from vi_stock "
                                 "where dated<=%1 and shop='%2' "
                                 "group by cargo, color;").arg(dated).arg(shop);
    qry.exec(sql);
    if ( qry.lastError().isValid() ) qDebug() << qry.lastError();
    while ( qry.next() ) {
        QString cargo = qry.value(0).toString();
        QString color = qry.value(1).toString();
        QString sizers = BsGrid::sizerTextSum(qry.value(2).toString());

        rowtime++;

        int newQty = 0;
        QStringList newSizers;
        QStringList oldSizers = sizers.split(QChar(10));
        for ( int i = 0, iLen = oldSizers.length(); i < iLen; ++i ) {
            QStringList oldPair = QString(oldSizers.at(i)).split(QChar(9));
            if ( oldPair.length() == 2 ) {
                QString oldSizer = oldPair.at(0);
                qint64 oldQty = QString(oldPair.at(1)).toLongLong();
                QString newPair = oldSizer + QChar(9) + QString::number(-oldQty);
                newQty -= oldQty;
                newSizers << newPair;
            }
        }
        QString minusSizers = newSizers.join(QChar(10));
        sumqty += newQty;
        QString sql = QStringLiteral("insert into syddtl(parentid, rowtime, cargo, color, sizers, qty) "
                                     "values(%1, %2, '%3', '%4', '%5', %6);")
                      .arg(newSheetId).arg(rowtime).arg(cargo).arg(color).arg(minusSizers).arg(newQty);
        sqls << sql;
    }
    qry.finish();

    sql = QStringLiteral("insert into syd(sheetid, proof, dated, shop, trader, stype, sumqty, summoney, sumdis) "
                                 "values(%1, '盘点清零', %2, '%3', '%3', '', %4, 0, 0);")
                  .arg(newSheetId).arg(dated).arg(shop).arg(sumqty);
    sqls << sql;

    sql = QStringLiteral("update sqlite_sequence set seq=%1 where name='syd';").arg(newSheetId);
    sqls << sql;

    //执行
    QString sqlErr = sqliteCommit(sqls);
    if ( sqlErr.isEmpty() ) {
        accept();
        QMessageBox::information(this, QString(), QStringLiteral("清库存成功！"));
    }
    else {
        QMessageBox::information(this, QString(), QStringLiteral("清库存不成功，您可联系软件www.bailisoft.com协助解决。"));
    }
}

}
