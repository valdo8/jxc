#ifndef BSSETLOGINER_H
#define BSSETLOGINER_H

#include <QtWidgets>


// BsSetLoginer
namespace BailiSoft {

class BsUserList;
class BsRightModel;
class BsRightView;

enum BsRightModelType {bsrmtRegis, bsrmtSheet, bsrmtQuery};

class BsSetLoginer : public QDialog
{
    Q_OBJECT
public:
    BsSetLoginer(QWidget *parent);
    ~BsSetLoginer() {}

public slots:
    void addLoginer();
    void delLoginer();
    void resetPassword();
    void presetMinRight();
    void presetMaxRight();

    void saveChange();
    void cancelChange();

    void pickLoginerChanged(QListWidgetItem *current, QListWidgetItem *);
    void rightChanged();

private:
    void loadLoginer(const QListWidgetItem *it);
    void presetSaveRight(const bool maxx);
    QString iconFileOf(const bool locall, const bool pcc, const bool backerr, const bool shopp);

    QWidget*            mpLeft;
    QPushButton*            mpBtnAddLoginer;
    BsUserList*             mpList;

    QWidget*            mpRight;
    QLabel*                 mpCurrentLoginer;
    QTabWidget*             mpTab;
    QWidget*                    mpRole;
    QLabel*                         mpLoginRole;
    QCheckBox*                      mpRetPrice;
    QCheckBox*                      mpLotPrice;
    QCheckBox*                      mpBuyPrice;
    QLabel*                         mpLblCargoExp;
    QLineEdit*                      mpLimCargoExp;
    QLabel*                         mpRoleInfo;
    BsRightView*                mpViewRegis;
    BsRightView*                mpViewSheet;
    BsRightView*                mpViewQuery;
    QWidget*                mpSaveBox;
    QPushButton*                mpBtnSaveChange;
    QPushButton*                mpBtnCancelChange;

    BsRightModel*       mpModelRegis;
    BsRightModel*       mpModelSheet;
    BsRightModel*       mpModelQuery;

};

}


// BsUserList
namespace BailiSoft {

class BsUserList : public QListWidget
{
    Q_OBJECT
public:
    BsUserList(BsSetLoginer *parent);
    ~BsUserList(){}
protected:
    void mousePressEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);
private:
    BsSetLoginer*   mppWin;
    QMenu*          mpMenu;
    QAction*        mpAcAddLoginer;
    QAction*        mpAcDelLoginer;
    QAction*        mpAcResetPwd;
    QAction*        mpAcPresetMin;
    QAction*        mpAcPresetMax;
};

}


// BsRightModel
namespace BailiSoft {

class BsRightModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    BsRightModel(const BsRightModelType rmType, QObject *parent);
    ~BsRightModel(){}

    void clearData();
    void loadData(const QList<uint> &datas);
    int rowCount(const QModelIndex &) const;
    int columnCount(const QModelIndex &) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QStringList         mTvNames;
    QStringList         mFlagNames;
    BsRightModelType    mRmType;
    QList<uint>         mDatas;
};

}


// BsRightView
namespace BailiSoft {

class BsRightView : public QTableView
{
    Q_OBJECT
public:
    BsRightView(QWidget *parent);
    ~BsRightView(){}

};

}


// BsRightDelegate
namespace BailiSoft {

class BsRightDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit BsRightDelegate(QWidget *parent, const QStringList &options, const bool disableActMoney = false,
                             const bool disableDisMoney = false, const bool disablePayOwe = false);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
private:
    QCheckBox *getCheckBoxByText(QWidget *pnl, const QString &txt) const;

    QStringList         mOptions;
    bool                mDisableActMoney;
    bool                mDisableDisMoney;
    bool                mDisablePayOwe;
};

}


#endif // BSSETLOGINER_H
