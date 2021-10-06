#ifndef BSIMPORTREGDLG_H
#define BSIMPORTREGDLG_H

#include <QtWidgets>

namespace BailiSoft {

class BsRegGrid;
class LxStringTableModel;
class LxImportTableView;

class BsImportRegDlg : public QDialog
{
    Q_OBJECT
public:
    BsImportRegDlg(QWidget *parent, BsRegGrid *grid);

    int getCordIndexOf(const QString &prFld);

    QRadioButton        *mpFirstRowIsTitle;
    QRadioButton        *mpFirstRowNotTitle;

    QRadioButton        *mpImportByAppend;
    QRadioButton        *mpImportByUpdate;

    LxImportTableView   *mpView;
    LxStringTableModel  *mpModel;
    
protected:
    void resizeEvent(QResizeEvent *);
    
private slots:
    void doHelp();
    void doFileOpen();
    void doCheckFinishOK();
    void doSetFirstRowColor(const bool isTitle);
    void doSetImportWayReady();
    void checkValid();
    void doImport();

private:
    void clearCordSettings();
    void autoCordinateCols();

    QToolButton         *mpBtnOpen;
    QWidget             *mpPnlFirstRowState;
    QWidget             *mpPnlImportWay;
    QHBoxLayout         *mpLayOKCancel;
    QPushButton         *mpBtnOK;

    BsRegGrid               *mppGrid;           //登记表格
    int                      mFirstDataRowIdx;  //首行开始索引
    QList<QPair<int, int> >  lstCords;          //srcIdx, dstIdx
};

class LxDragField : public QLabel
{
    Q_OBJECT
public:
    LxDragField(const QString &prFldName, const QString &prFldTitle, const bool prRequired, QWidget *parent = nullptr);
    QString fldName;
    QString fldTitle;
    int     cordIndex;
    bool    isRequired;

signals:
    void cordChanged();

protected:
    void mousePressEvent(QMouseEvent *);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

};

class LxImportTableView : public QTableView
{
    Q_OBJECT
public:
    explicit LxImportTableView(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    //视图Model的flags中，需要指定ItemFlags中包括ItemIsDragEnabled，startDrag才会被调用。
    //又由于要控制光标变化、整列选中等细微变化，故不用startDrag调用drag过程，而用mousePressEvent
    //配合mouseMoveEvent来调用，对drag来说，可以控制得更细化。
    //void startDrag(Qt::DropActions supportedActions);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    QPoint      mStartPos;
};

}

#endif // BSIMPORTREGDLG_H
