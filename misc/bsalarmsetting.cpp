#include "bsalarmsetting.h"
#include "main/bailicode.h"
#include "main/bailidata.h"

namespace BailiSoft {

// BsAlarmSetting
BsAlarmSetting::BsAlarmSetting(const QString &cargo, QWidget *parent) : QDialog(parent)
{
    //货号标题
    mpLblCargo = new QLabel(this);
    mpLblCargo->setAlignment(Qt::AlignCenter);
    mpLblCargo->setText(cargo);
    mpLblCargo->setStyleSheet(QLatin1String("font-weight:900; color:red; "));

    //数据表格
    mpGrid = new BsAlarmGrid(this);
    mpGrid->loadData(cargo);

    //格式说明
    mpFormat = new QLabel(this);
    mpFormat->setAlignment(Qt::AlignCenter);
    mpFormat->setText(QStringLiteral("每个色码格用逗号分隔警报“最低数”与“最高数”。"));
    mpFormat->setStyleSheet(QLatin1String("color:#999; "));

    //确定取消
    mpBox = new QDialogButtonBox(this);
    mpBox->setOrientation(Qt::Horizontal);
    mpBox->setCenterButtons(true);

    QPushButton *pBtnOk = mpBox->addButton(mapMsg.value("btn_ok"), QDialogButtonBox::AcceptRole);
    pBtnOk->setIcon(QIcon(":/icon/ok.png"));
    pBtnOk->setIconSize(QSize(20, 20));
    connect(pBtnOk, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *pBtnCancel = mpBox->addButton(mapMsg.value("btn_cancel"), QDialogButtonBox::RejectRole);
    pBtnCancel->setIcon(QIcon(":/icon/cancel.png"));
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    //layout
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(mpLblCargo);
    lay->addWidget(mpGrid, 1);
    lay->addWidget(mpFormat);
    lay->addWidget(mpBox);
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);
}

QString BsAlarmSetting::saveData()
{
    return mpGrid->saveData();
}

void BsAlarmSetting::removeData(const QString &cargo)
{
    QSqlDatabase db = QSqlDatabase::database();
    db.exec(QStringLiteral("delete from stockalarm where cargo='%1';").arg(cargo));
}


void BsAlarmSetting::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    resize(mpGrid->horizontalHeader()->defaultSectionSize() * (1 + mpGrid->columnCount()) + 30,
           mpGrid->verticalHeader()->defaultSectionSize() * (1 + mpGrid->rowCount()) +
           mpLblCargo->height() + mpFormat->height() + mpBox->height() + 100);
}


// BsAlarmGrid ====================================================================
BsAlarmGrid::BsAlarmGrid(QWidget *parent) : QTableWidget(parent)
{
    horizontalHeader()->setDefaultSectionSize(66);
    verticalHeader()->setDefaultSectionSize(24);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalHeader()->setStyleSheet("QHeaderView::section { border-style:none; "
                                      "border-bottom: 1px solid #ccc; border-right: 1px solid #ccc;}");
    verticalHeader()->setStyleSheet("QHeaderView::section { border-style:none; border-bottom: "
                                    "1px solid #ccc; border-right: 1px solid #ccc; }");
}

void BsAlarmGrid::loadData(const QString &cargo)
{
    mCargo = cargo;

    //load sizers
    QString sizerType = dsCargo->getValue(cargo, "sizertype");
    QStringList sizers = dsSizer->getSizerList(sizerType);

    //load colors
    QString colorType = dsCargo->getValue(cargo, "colortype");
    QStringList colors = dsColorList->getColorListByType(colorType);

    //dimensions
    setColumnCount(sizers.length());
    setHorizontalHeaderLabels(sizers);

    setRowCount(colors.length());
    setVerticalHeaderLabels(colors);

    //ready cell data
    QMap<QString, QPair<qint64, qint64> > mapValues;

    QSqlQuery qry;
    qry.setForwardOnly(true);
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec(QStringLiteral("select color, minsizers, maxsizers from stockalarm "
                            "where cargo='%1' order by color;").arg(cargo));
    while (qry.next())
    {
        QString color = qry.value(0).toString();

        QMap<QString, qint64>   mapMinQtys;
        QStringList minQtys = qry.value(1).toString().split(QChar(10), QString::SkipEmptyParts);
        for ( int i = 0, iLen = minQtys.length(); i < iLen; ++i )
        {
            QStringList p = QString(minQtys.at(i)).split(QChar(9));
            Q_ASSERT(p.length() == 2);
            mapMinQtys.insert(p.at(0), QString(p.at(1)).toLongLong());
        }

        QMap<QString, qint64>   mapMaxQtys;
        QStringList maxQtys = qry.value(2).toString().split(QChar(10), QString::SkipEmptyParts);
        for ( int i = 0, iLen = maxQtys.length(); i < iLen; ++i )
        {
            QStringList p = QString(maxQtys.at(i)).split(QChar(9));
            Q_ASSERT(p.length() == 2);
            mapMaxQtys.insert(p.at(0), QString(p.at(1)).toLongLong());
        }

        for ( int i = 0, iLen = sizers.length(); i < iLen; ++i )
        {
            QString sizer = sizers.at(i);
            if ( mapMinQtys.contains(sizer) && mapMaxQtys.contains(sizer) )
            {
                QString k = QStringLiteral("%1-%2").arg(color).arg(sizer);
                qint64 min = mapMinQtys.value(sizer);
                qint64 max = mapMaxQtys.value(sizer);
                mapValues.insert(k, qMakePair(min, max));
            }
        }
    }
    qry.finish();

    for ( int i = 0, iLen = rowCount(); i < iLen; ++i )
    {
        QString color = colors.at(i);

        for ( int j = 0, jLen = columnCount(); j < jLen; ++j )
        {
            QString sizer = sizers.at(j);
            QString k = QStringLiteral("%1-%2").arg(color).arg(sizer);
            QTableWidgetItem *it = new QTableWidgetItem();
            if ( mapValues.contains(k) )
            {
                QPair<qint64, qint64> pair = mapValues.value(k);
                QString minQtyText = bsNumForRead(pair.first, 0);
                QString maxQtyText = bsNumForRead(pair.second, 0);
                it->setText(QStringLiteral("%1,%2").arg(minQtyText).arg(maxQtyText));
            }
            else
                it->setText(QStringLiteral("0,0"));
            setItem(i, j, it);
        }
    }
}

QString BsAlarmGrid::saveData()
{
    //sqls
    double minTotal = 0.0, maxTotal = 0.0;
    QStringList sqls;
    for ( int i = 0, iLen = rowCount(); i < iLen; ++i )
    {
        QString color = model()->headerData(i, Qt::Vertical).toString();
        QStringList minPairs, maxPairs;
        for ( int j = 0, jLen = columnCount(); j < jLen; ++j )
        {
            QString sizer = model()->headerData(j, Qt::Horizontal).toString();
            QStringList pair = item(i, j)->text().split(QChar(','));
            double min = QString(pair.at(0)).toDouble();
            double max = QString(pair.at(1)).toDouble();
            minTotal += min;
            maxTotal += max;
            minPairs << QStringLiteral("%1\t%2").arg(sizer).arg(bsNumForSave(min));
            maxPairs << QStringLiteral("%1\t%2").arg(sizer).arg(bsNumForSave(max));
        }
        sqls << QStringLiteral("INSERT OR REPLACE INTO stockalarm(cargo, color, minsizers, maxsizers) "
                               "values('%1', '%2', '%3', '%4');").arg(mCargo).arg(color)
                .arg(minPairs.join(QChar(10))).arg(maxPairs.join(QChar(10)));
    }

    //货号表
    sqls << QStringLiteral("update cargo set almin=%1, almax=%2 where hpcode='%3';")
            .arg(bsNumForSave(minTotal)).arg(bsNumForSave(maxTotal)).arg(mCargo);

    //save
    sqliteCommit(sqls);

    //返回
    return QStringLiteral("%1,%2").arg(QString::number(minTotal, 'f', 0)).arg(QString::number(maxTotal, 'f', 0));
}

void BsAlarmGrid::commitData(QWidget *editor)
{
    QLineEdit *edt = qobject_cast<QLineEdit*>(editor);
    if ( edt ) {
        QString input = edt->text();
        input.replace(QRegularExpression("\\D+"), QStringLiteral(","));
        QStringList nums = input.split(QChar(','));
        int min, max;
        if ( nums.length() > 1 ) {
            min = QString(nums.at(0)).toInt();
            max = QString(nums.at(1)).toInt();
        }
        else {
            min = QString(nums.at(0)).toInt();
            max = QString(nums.at(0)).toInt();
        }
        edt->setText(QStringLiteral("%1,%2").arg(min).arg(max));
    }
    QTableWidget::commitData(editor);
}

void BsAlarmGrid::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return ) {
        QKeyEvent *tmpEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
        qApp->postEvent(this, tmpEvent, 0);
        //参见Qtw文档，postEvent不用自行delete
        return;
    }
    QTableWidget::keyPressEvent(e);
}


}
