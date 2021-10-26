#ifndef BAILICODE_H
#define BAILICODE_H

#include <QtCore>

//本单元因为几乎所有窗口都要引用，故放最常用宏、定义、变量及函数

#define BSWIN_TABLE     "bswintable"
#define BSACFLAGS       "stateflags"
#define BSACRIGHT       "rightallow"
#define BSVALUE_OLD     "oldvalue"

namespace BailiSoft {

//QSettings键名
const QString BSR17BookList          = QLatin1String("BSR17BookList");
const QString BSR17UserPreference    = QLatin1String("BSR17UserPreference");
const QString BSR17WinSize           = QLatin1String("BSR17WinSize");
const QString BSR17OptionBox         = QLatin1String("BSR17OptionBox");
const QString BSR17ColumnWidth       = QLatin1String("BSR17ColumnWidth");

//版本号
extern int lxapp_version_major;
extern int lxapp_version_minor;
extern int lxapp_version_patch;

//表基名（英）
extern QStringList    lstRegisWinTableNames;
extern QStringList    lstSheetWinTableNames;
extern QStringList    lstQueryWinTableNames;

//表基名（中）
extern QStringList    lstRegisWinTableCNames;
extern QStringList    lstSheetWinTableCNames;
extern QStringList    lstQueryWinTableCNames;

//常用特殊权限
extern QString  bossAccount;
extern bool     loginAsBoss;
extern bool     loginAsAdmin;
extern bool     loginAsAdminOrBoss;

//中文字符串
extern QMap<QString, QString>     mapMsg;

//初始化函数
void initWinTableNames();
void initMapMsg();

//数据库批量提交函数
QString sqliteCommit(const QStringList sqls);

//数值转换函数
inline QString bsNumForSave(const double val) { return QString::number((qint64)((round(val * 10000)))); }
inline QString bsNumForRead(const qint64 val, const int dots) { return QString::number(val/10000.0, 'f', dots); }

}

#endif // BAILICODE_H
