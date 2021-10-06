#ifndef BSALARMREPORT_H
#define BSALARMREPORT_H

#include <QtWidgets>

namespace BailiSoft {

class BsQueryGrid;

enum bsStockAlarmType {bssatMin, bssatMax};

class BsAlarmReport : public QWidget
{
    Q_OBJECT
public:
    explicit BsAlarmReport(const bsStockAlarmType alarmType, QWidget *parent = nullptr);
    ~BsAlarmReport() {}
    QSize sizeHint() const;

    BsQueryGrid*        mpGrid;
    bsStockAlarmType    mAlarmType;

public slots:
    void reloadAlarm();
};


class BsMinAlarmReport : public BsAlarmReport
{
    Q_OBJECT
public:
    explicit BsMinAlarmReport(QWidget *parent);
    ~BsMinAlarmReport(){}
};

class BsMaxAlarmReport : public BsAlarmReport
{
    Q_OBJECT
public:
    explicit BsMaxAlarmReport(QWidget *parent);
    ~BsMaxAlarmReport(){}
};

}

#endif // BSALARMREPORT_H
