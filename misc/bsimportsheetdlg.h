#ifndef BSIMPORTSHEETDLG_H
#define BSIMPORTSHEETDLG_H

#include <QtWidgets>

namespace BailiSoft {

class BsSheetCargoGrid;
class LxCsvTableModel;

class BsImportSheetDlg : public QDialog
{
    Q_OBJECT
public:
    BsImportSheetDlg(QWidget *parent, BailiSoft::BsSheetCargoGrid *grid, const QStringList &csvData);

    static int header_rows;
    static int footer_rows;
    static int cargo_col;
    static int color_col;
    static int sizer_col;
    static int qty_col;
    static int sizer_hcols;

protected:
    void hideEvent(QHideEvent *event);
       
private slots:
    void hideHeadRows(int count);
    void hideFootRows(int count);
    void doImport();

private:
    QTableView          *mpView;
    LxCsvTableModel     *mpModel;

    QSpinBox            *mpHeaderRows;
    QSpinBox            *mpFooterRows;
    QSpinBox            *mpCargoCol;
    QSpinBox            *mpColorCol;
    QSpinBox            *mpSizerCol;
    QSpinBox            *mpQtyCol;
    QSpinBox            *mpSizerHCols;

    QPushButton         *mpBtnOK;

    BsSheetCargoGrid        *mppGrid;           //单据表格
};

///////////////////////////////////////////////////////////////////////////////

class LxCsvTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit LxCsvTableModel(const QStringList &lines, QObject *parent = nullptr);
    int rowCount(const QModelIndex &) const;
    int columnCount(const QModelIndex &) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void resetHeaderRows(const int rows);
    void resetFooterRows(const int rows);

private:
    QStringList mLines;
    int mCols;
    int mHeaderRows;
    int mFooterRows;

};

}

#endif // BSIMPORTSHEETDLG_H
