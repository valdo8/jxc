#include "bsupg11.h"
#include "quuid.h"
#include "comm/pinyincode.h"
#include "main/bailisql.h"
#include "main/bailicode.h"
#include "main/bailidata.h"
#include "main/bailifunc.h"

#define IMPORT_HELP     "升迁建账过程为——创建新的库文件，并从老的数据文件中导入数据，同时改变为新的数据格式，\
以适应新的程序。因此，升迁建账完全不影响原数据文件。如果不满意，可以仍然继续使用原R15系统。因此，请放心操作甚至\
多次练习。"

#define REGEXP_HELP     "<font color='#080'><b>正则表达式</b>简单语法：\\d通配数字，\\D通配字母，\\w通配字母或数字，\
前面不加反斜线为原字符不通配；几种可能用方括号列举，比如[ABC]表示A或B或C。<br/>\
字符种类识别后，还要识别位数。位数表示法是{x,y}的形式。\
x为至少位数，y为最多位数；固定位数用{x}表示；不限最多位数用{x,}表示。<br/>\
例如：<b>\\D{2}\\d{8}</b>表示两位字母后跟8位数字；\
<b>\\d{6,}-\\D\\d{2,4}-\\D{1,8}</b>表示最少6位数字后带减号再后面是一位字母跟2到4位数字，\
再又是减号，后面是1到8位数字。以此类推。</font>"

#define COLOR_DRAG_HELP  "原R15系统色号每货号重复登记，没有分组，而事实上，多个不同货号完全应当对应相同的颜色系列。\
左边的列表将原账册中所有出现的色号全部列出，请将其逐一拖动到上表格——注意：同系列的色号应当放到同一系列里。\
对那些不再使用以及错误的色号，也要拖上去，单独放到一行中，随便取个系列名。"


namespace BailiSoft {

BsUpg11::BsUpg11(QWidget *parent) : QDialog(parent)
{
    mpBtnExec = new QPushButton(QStringLiteral("执行"), this);
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(mpBtnExec, 0, Qt::AlignCenter);
    setMinimumSize(300, 200);

    connect(mpBtnExec, &QPushButton::clicked, this, &BsUpg11::doImport);
}

void BsUpg11::doImport()
{
    //观察所得
    mOldSizeColCount = 10;
    mSizers << QStringLiteral("XS")
            << QStringLiteral("S")
            << QStringLiteral("M")
            << QStringLiteral("L")
            << QStringLiteral("XL")
            << QStringLiteral("XXL")
            << QStringLiteral("3XL")
            << QStringLiteral("F")
            << QStringLiteral("4XL")
            << QStringLiteral("5XL");
    mShops << QStringLiteral("0总仓库")
           << QStringLiteral("1◎◎◎天意总店")
           << QStringLiteral("2￥￥￥西门坡店");

    //file
    QString bookName = QStringLiteral("天意");
    QString accessFile = QStringLiteral("E:/ty.mdb");
    QString sqliteFile;

    QString accessConn = QStringLiteral("MSAccessImportR11Conn");
    QString sqliteConn = QStringLiteral("SqliteImportR11Conn");

    //mdb conn
    QString strErr;
    {
        QSqlDatabase mdb = QSqlDatabase::database(accessConn);
        if ( mdb.isValid() )
            mdb.close();
        else
            mdb = QSqlDatabase::addDatabase("QODBC", accessConn);
        mdb.setDatabaseName(QStringLiteral("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};FIL={MS Access};DBQ=%1")
                            .arg(QDir::toNativeSeparators(accessFile)));
        if ( mdb.open() ) {
            QString fileBase = QFileInfo(accessFile).baseName() + QStringLiteral(".jyb");
            sqliteFile = QDir(dataDir).absoluteFilePath(fileBase);
            if ( QFile::exists(sqliteFile) ) {
                if ( ! QFile::remove(sqliteFile) ) {
                    strErr = QStringLiteral("%1文件被占用，无法删除，请关闭Sqlite工具！").arg(sqliteFile);
                }
            }
        }
        else {
            strErr = QStringLiteral("mdb文件打开失败！\n%1").arg(mdb.lastError().text());
        }

        //sqlite conn
        if ( strErr.isEmpty() ) {
            //sqls
            QStringList sqls = BailiSoft::sqliteInitSqls(bookName, true);

            //init
            QSqlDatabase sdb = QSqlDatabase::addDatabase("QSQLITE", sqliteConn);
            sdb.setDatabaseName(sqliteFile);
            if ( sdb.open() ) {
                sdb.transaction();
                for ( int i = 0, iLen = sqls.length(); i < iLen; ++i ) {

                    QString sql = sqls.at(i);
                    sql = sql.trimmed();

                    if ( sql.startsWith("BEGIN") || sql.startsWith("begin") ) continue;
                    if ( sql.startsWith("COMMIT") || sql.startsWith("commit") ) break;

                    if ( sql.length() > 9 ) {
                        sdb.exec(sql);
                        if ( sdb.lastError().isValid() ) {
                            strErr = sdb.lastError().text() + QStringLiteral("[初始化新库时]\n") + sql;
                            sdb.rollback();
                            break;
                        }
                    }
                }
                if ( strErr.isEmpty() ) {
                    sdb.commit();
                }
            } else {
                strErr = QStringLiteral("系统不支持Sqlite，%1创建不成功。错误信息：%2").arg(sqliteFile).arg(sdb.lastError().text());
            }

            //import
            if ( strErr.isEmpty() ) {

                QSqlDatabase mdb = QSqlDatabase::database(accessConn);

                QString strErr = importStockAsSYD(mdb, sdb);

                if ( strErr.isEmpty() ) {
                    strErr = updateSheetSum("syd", sdb);
                }

                if ( strErr.isEmpty() ) {
                    strErr = importCargo(mdb, sdb);
                }
            }
        }
        QSqlDatabase::removeDatabase(sqliteConn);
    }
    QSqlDatabase::removeDatabase(accessConn);

    //报告
    if ( strErr.isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("导入成功！"));
    } else {
        QMessageBox::information(this, QString(), QStringLiteral("导入失败！\n%1").arg(strErr));
    }
    accept();
}

QString BsUpg11::importStockAsSYD(QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery mqry(mdb);
    QString oldSizeCols = getOldSizeColSum(QString());
    int sheetId = 0;

    mCargoSet.clear();

    //按门店
    foreach (QString shop, mShops) {

        //主表
        sheetId++;
        QDate dateD = QDateTime::currentDateTime().date();
        qint64 nowTime = QDateTime::currentMSecsSinceEpoch();
        QString sql = QStringLiteral("insert into SYD("
                                     "sheetID, proof, dateD, shop, trader, stype, staff, remark, "
                                     "checker, chktime, upMan, upTime) "
                                     "values(%1, '导入期初', %2, '%3', '导入期初', '导入期初', '系统', '', "
                                     "'系统', %4, '系统', %4);")
                .arg(sheetId).arg(QDateTime(dateD).toMSecsSinceEpoch() / 1000).arg(shop).arg(nowTime);
        sqls << sql;

        //细表
        sql = QStringLiteral("select style, color, sum(qty), %1 from resultStock "
                             "where stock='%2' "
                             "group by style, color "
                             "having sum(qty)<>0;").arg(oldSizeCols).arg(shop);
        mqry.exec(sql);
        if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();

        //R11行遍历
        while ( mqry.next() ) {

            QString cargo = mqry.value(0).toString().toUpper();
            QString color = mqry.value(1).toString();
            int actQty = mqry.value(2).toInt();
            color = color.replace(QChar(39), QString()).replace(QChar('['), QString()).replace(QChar(']'), QString());   //特殊观察

            mCargoSet.insert(cargo);

            //因为R11缺陷，有的qty列与各SZ列之和并不相等，故重求
            int sumQty = 0;
            for ( int i = 1, iLen = mSizers.length(); i <= iLen; ++i ) {
                sumQty += mqry.value(2 + i).toInt();
            }
            bool bugRow = (sumQty != actQty);

            //尺码遍历求sizers
            QStringList sizers;
            for ( int i = 1, iLen = mSizers.length(); i <= iLen; ++i ) {

                int qty = mqry.value(2 + i).toInt();

                if ( qty != 0 ) {

                    //求和BUG行处理
                    if ( bugRow ) {

                        //以合计行为准（现在就退出）
                        if ( actQty == 0 )
                            break;

                        //只将总数放一个尺码，后面break退出for循环，不再处理其它尺码
                        if ( actQty != 0 ) {
                            qty = actQty;
                        }
                    }

                    QString sizer = mSizers.at(i - 1);
                    sizers << QStringLiteral("%1\t%2").arg(sizer).arg(qty * 10000);

                    //已经全放一个尺码时跳过
                    if ( bugRow && actQty != 0 )
                        break;
                }
            }

            //SQL   （教训：values()中逗号后慎加空格，因为sqlite奇葩整形会存储字符串型！）
            QString sql = QStringLiteral("insert into syddtl(parentid, rowtime, cargo, color, sizers, "
                                         "price, qty, discount, actMoney, disMoney) "
                                         "values(%1,%2,'%3','%4','%5',0,%6,0,0,0);")
                    .arg(sheetId)
                    .arg(nowTime++)
                    .arg(cargo)
                    .arg(color)
                    .arg(sizers.join(QChar(10)))
                    .arg(actQty * 10000);

            sqls << sql;
        }
    }

    return batchExec(sqls, newdb);
}

QString BsUpg11::updateSheetSum(const QString &sheet, QSqlDatabase &newdb)
{
    QStringList sqls;
    QSqlQuery newqry(newdb);

    //更新主表合计
    newqry.exec(QStringLiteral("select parentid, sum(qty) as sqty, sum(actMoney) as mny, sum(dismoney) as sumdis "
                               "from %1dtl group by parentid;").arg(sheet));
    if ( newqry.lastError().isValid() ) return newqry.lastError().text() + "\n" + newqry.lastQuery();
    while ( newqry.next() ) {
        sqls <<  QStringLiteral("update %1 set sumQty=%2, sumMoney=%3, sumdis=%4 where sheetID=%5;")
                 .arg(sheet)
                 .arg(newqry.value(1).toInt())
                 .arg(newqry.value(2).toLongLong())
                 .arg(newqry.value(3).toLongLong())
                 .arg(newqry.value(0).toString());
    }

    //更新主表实付款
    sqls << QStringLiteral("update %1 set actPay=sumMoney;").arg(sheet);

    return batchExec(sqls, newdb);
}

QString BsUpg11::importCargo(QSqlDatabase &mdb, QSqlDatabase &newdb)
{
    QMap<QString, QString> colorMap;

    QStringList sqls;
    QSqlQuery mqry(mdb);
    mqry.exec(QStringLiteral("select style, styleName, unit, colorList, setPrice, brand, season, pattern, fabric from objStyle;"));
    if ( mqry.lastError().isValid() ) return mqry.lastError().text() + "\n" + mqry.lastQuery();
    while ( mqry.next() ) {
        QString cargo = mqry.value(0).toString().toUpper();
        QString colorList = mqry.value(3).toString().replace(QChar(39), QString())
                .replace(QChar('['), QString()).replace(QChar(']'), QString());
        QString price = bsNumForSave(mqry.value(4).toDouble());     //四价统一

        if (mCargoSet.contains(cargo)) {
            //cargo sqls
            sqls << QStringLiteral("insert into cargo(hpcode, hpname, unit, sizertype, colortype, "
                                   "setPrice, retPrice, lotPrice, buyPrice, "
                                   "attr1, attr2, attr3, attr4, attr5, attr6, upMan, upTime) "
                                   "values('%1', '%2', '%3', '通码', '%4', %5, %5, %5, %5, "
                                   "'%6', '%7', '%8', '%9', '', '', '系统', %10);")
                    .arg(cargo)
                    .arg(mqry.value(1).toString())
                    .arg(mqry.value(2).toString())
                    .arg(colorList)
                    .arg(price)
                    .arg(mqry.value(5).toString())
                    .arg(mqry.value(6).toString())
                    .arg(mqry.value(7).toString())
                    .arg(mqry.value(8).toString())
                    .arg(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);

            //colorType prepare
            colorMap.insert(cargo, colorList);
        }
    }

    //sizerType
    QString sql = QStringLiteral("insert into sizertype(tname, namelist, codelist, upman, uptime) "
                                 "values('通码', 'XS,S,M,L,XL,XXL,3XL,F,4XL,5XL', 'XS,S,M,L,XL,XXL,3XL,F,4XL,5XL', '系统', %1);")
            .arg(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
    sqls << sql;

    //shops
    QString shop;
    foreach (shop, mShops) {
        sql = QStringLiteral("insert into shop(kname, regdis, upman, uptime) "
                             "values('%1', 10000, '系统', %2);")
                .arg(shop)
                .arg(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
        sqls << sql;
    }

    return batchExec(sqls, newdb);
}

QString BsUpg11::getOldSizeColSel(const QString &prefix)
{
    QStringList sl;
    for ( int i = 1, iLen = mOldSizeColCount; i <= iLen; ++i ) {
        sl << QStringLiteral("%1SZ%2").arg(prefix).arg(i, 2, 10, QLatin1Char('0'));
    }
    return sl.join(QChar(44));
}

QString BsUpg11::getOldSizeColSum(const QString &prefix)
{
    QStringList sl;
    for ( int i = 1, iLen = mOldSizeColCount; i <= iLen; ++i ) {
        sl << QStringLiteral("SUM(%1SZ%2) AS %2").arg(prefix).arg(i, 2, 10, QLatin1Char('0'));
    }
    return sl.join(QChar(44));
}

QString BsUpg11::batchExec(const QStringList &sqls, QSqlDatabase &newdb)
{
    QString sql;
    QSqlQuery newqry(newdb);

    newdb.transaction();
    foreach (sql, sqls) {
        if ( sql.trimmed().length() > 1 ) {
            newqry.exec(sql);
            if ( newqry.lastError().isValid() ) {
                newdb.rollback();
                qDebug() << sql;
                qDebug() << newqry.lastError().text();
                return newqry.lastError().text() + "\n" + sql;
            }
        }
    }
    newdb.commit();

    return "";
}


}
