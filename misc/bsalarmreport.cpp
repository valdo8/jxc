#include "bsalarmreport.h"
#include "main/bailigrid.h"
#include "main/bailicode.h"
#include "main/bailidata.h"

namespace BailiSoft {

BsAlarmReport::BsAlarmReport(const bsStockAlarmType alarmType, QWidget *parent)
    : QWidget(parent), mAlarmType(alarmType)
{
    mpGrid = new BsQueryGrid(this);
    mpGrid->verticalHeader()->hide();

    QPushButton *btnReload = new QPushButton(QIcon(":/icon/reload.png"), mapMsg.value("btn_reload"), this);
    btnReload->setFixedSize(100, 30);
    connect(btnReload, SIGNAL(clicked(bool)), this, SLOT(reloadAlarm()));

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 6);
    lay->setSpacing(3);
    lay->addWidget(mpGrid, 1);
    lay->addWidget(btnReload, 0, Qt::AlignCenter);

    //设置背景色，且不影响子部件
    QString backColor = ( alarmType == bssatMin ) ? "#fdd" : "#def";
    QPalette pal(palette());
    pal.setBrush(QPalette::Window, QColor(backColor));
    setAutoFillBackground(true);
    setPalette(pal);

    //加载数据
    reloadAlarm();
}

QSize BsAlarmReport::sizeHint() const
{
    mpGrid->resizeColumnsToContents();
    int w = mpGrid->columnWidth(0) + mpGrid->columnWidth(1) + mpGrid->columnWidth(2) +
            mpGrid->columnWidth(3) + mpGrid->columnWidth(4);
    return QSize(w, 400);
}

void BsAlarmReport::reloadAlarm()
{
    //临时表名称
    QString tmpTableCargo = QStringLiteral("tmp_alarm_cargo_%1").arg(loginer);
    QString tmpTablePool = QStringLiteral("tmp_alarm_pool_%1").arg(loginer);
    QString tmpTableResult = QStringLiteral("tmp_alarm_result_%1").arg(loginer);
    //    QString tmpTableStockSum = QStringLiteral("tmp_alarm_stock_sum_%1").arg(loginer);

    //批量语句
    QStringList sqls;

    //货号范围表
    sqls << QStringLiteral("DROP TABLE IF EXISTS temp.%1;").arg(tmpTableCargo);
    sqls << QStringLiteral("CREATE TEMP TABLE %1 AS SELECT DISTINCT cargo FROM stockalarm;").arg(tmpTableCargo);

    //库存分拆创建入池
    QStringList stockUnionUnitSqls;
    QStringList sizerNames = dsSizer->getWholeUniqueNames();
    foreach (QString sizerName, sizerNames) {
        if ( !sizerName.isEmpty() ) {
            stockUnionUnitSqls << QStringLiteral("SELECT cargo, color, '%1' AS sizer, "
                                                 "(CASE SUBSTR(sizers,2,1) WHEN '\v' THEN "
                                                 "(CASE WHEN INSTR(sizers,'%2')>2 THEN "
                                                 "CAST(SUBSTR(sizers,INSTR(sizers,'%2')+%3,"
                                                 "INSTR(SUBSTR(sizers,INSTR(sizers,'%2'))||'\n','\n')-"
                                                 "INSTR(SUBSTR(sizers,INSTR(sizers,'%2')),'\t')-1) AS INTEGER) "
                                                 "ELSE 0 END) ELSE "
                                                 "(CASE WHEN INSTR(sizers,'%2')>2 THEN "
                                                 "-1*CAST(SUBSTR(sizers,INSTR(sizers,'%2')+%3,"
                                                 "INSTR(SUBSTR(sizers,INSTR(sizers,'%2'))||'\n','\n')-"
                                                 "INSTR(SUBSTR(sizers,INSTR(sizers,'%2')),'\t')-1) AS INTEGER) "
                                                 "ELSE 0 END) "
                                                 "END) AS stock, 0 AS limqty "
                                                 "FROM vi_stock_nodb WHERE cargo IN "
                                                 "(SELECT cargo FROM %4) ")  //这里不能加分号
                                  .arg(sizerName).arg(sizerName + QChar(9)).arg(sizerName.length() + 1).arg(tmpTableCargo);
        }
    }
    sqls << QStringLiteral("DROP TABLE IF EXISTS temp.%1;").arg(tmpTablePool);
    sqls << QStringLiteral("CREATE TEMP TABLE %1 AS %2;")
            .arg(tmpTablePool).arg(stockUnionUnitSqls.join(QStringLiteral(" UNION ALL ")));

    //设置分拆入池(stockalarm表没有重复项，后面SUM没有问题)
    foreach (QString sizerName, sizerNames) {
        if ( !sizerName.isEmpty() ) {
            if ( mAlarmType == bssatMin )
                sqls << QStringLiteral("INSERT INTO %1(cargo, color, sizer, stock, limqty) "
                                       "SELECT cargo, color, '%2' AS sizer, 0 AS stock, "
                                       "(CASE WHEN INSTR(minsizers,'%3')>0 THEN "
                                       "CAST(SUBSTR(minsizers,INSTR(minsizers,'%3')+%4,"
                                       "INSTR(SUBSTR(minsizers,INSTR(minsizers,'%3'))||'\n','\n')-"
                                       "INSTR(SUBSTR(minsizers,INSTR(minsizers,'%3')),'\t')-1) AS INTEGER) "
                                       "ELSE 0 END) AS limqty "
                                       "FROM stockalarm; ")
                        .arg(tmpTablePool).arg(sizerName).arg(sizerName + QChar(9)).arg(sizerName.length() + 1);
            else
                sqls << QStringLiteral("INSERT INTO %1(cargo, color, sizer, stock, limqty) "
                                       "SELECT cargo, color, '%2' AS sizer, 0 AS stock, "
                                       "(CASE WHEN INSTR(maxsizers,'%3')>0 THEN "
                                       "CAST(SUBSTR(maxsizers,INSTR(maxsizers,'%3')+%4,"
                                       "INSTR(SUBSTR(maxsizers,INSTR(maxsizers,'%3'))||'\n','\n')-"
                                       "INSTR(SUBSTR(maxsizers,INSTR(maxsizers,'%3')),'\t')-1) AS INTEGER) "
                                       "ELSE 0 END) AS limqty "
                                       "FROM stockalarm; ")
                        .arg(tmpTablePool).arg(sizerName).arg(sizerName + QChar(9)).arg(sizerName.length() + 1);
        }
    }

    //SUM创建最终表
    sqls << QStringLiteral("DROP TABLE IF EXISTS temp.%1;").arg(tmpTableResult);
    sqls << QStringLiteral("CREATE TEMP TABLE %1 AS "
                           "SELECT cargo, color, sizer, SUM(stock) AS stock, SUM(limqty) AS limqty "
                           "FROM %2 GROUP BY cargo, color, sizer;")
            .arg(tmpTableResult).arg(tmpTablePool);

    //执行
    sqliteCommit(sqls);

    //展示
    QString compStr = ( mAlarmType == bssatMin ) ? QStringLiteral("<") : QStringLiteral(">");
    QString qrySql = QStringLiteral("SELECT cargo, color, sizer, limqty, stock AS nowstock FROM %1 "
                                    "WHERE stock %2 limqty order by cargo, color;")
            .arg(tmpTableResult).arg(compStr);
    mpGrid->loadData(qrySql);
}

BsMinAlarmReport::BsMinAlarmReport(QWidget *parent) : BsAlarmReport(bssatMin, parent)
{
    mpGrid->horizontalHeader()->setStyleSheet("QHeaderView::section{background-color:#fdd;}");
}

BsMaxAlarmReport::BsMaxAlarmReport(QWidget *parent) : BsAlarmReport(bssatMax, parent)
{
    mpGrid->horizontalHeader()->setStyleSheet("QHeaderView::section{background-color:#def;}");
}

}
