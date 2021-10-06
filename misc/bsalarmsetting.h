#ifndef BSALARMSETTING_H
#define BSALARMSETTING_H

#include <QtWidgets>

namespace BailiSoft {

class BsAlarmGrid;

// BsAlarmSetting
class BsAlarmSetting : public QDialog
{
    Q_OBJECT
public:
    explicit BsAlarmSetting(const QString &cargo, QWidget *parent = nullptr);
    ~BsAlarmSetting(){}
    QString saveData();
    static void removeData(const QString &cargo);

protected:
    void showEvent(QShowEvent *e);

private:
    QLabel*             mpLblCargo;
    BsAlarmGrid*        mpGrid;
    QLabel*             mpFormat;
    QDialogButtonBox*   mpBox;
};

// BsAlarmGrid
class BsAlarmGrid : public QTableWidget
{
    Q_OBJECT
public:
    explicit BsAlarmGrid(QWidget *parent);
    ~BsAlarmGrid(){}
    void loadData(const QString &cargo);
    QString saveData();
protected:
    void commitData(QWidget *editor);
    void keyPressEvent(QKeyEvent *e);
private:
    QString mCargo;
};

}

#endif // BSALARMSETTING_H
