#include "bsimportsheetdlg.h"
#include "comm/lxstringtablemodel.h"
#include "comm/bsflowlayout.h"
#include "comm/pinyincode.h"
#include "main/bailifunc.h"
#include "main/bailicode.h"
#include "main/bailigrid.h"
#include "main/bailidata.h"

#define MAIN_HELP_STEP          "第一步：打开文件；\n第二步：指定文件第1行是数据还是列名称;\n第三步：指定文件各列分别对应什么列，方式为直接用鼠标拖动匹配，双击取消。其中，红色为必填项，不可忽略。"
#define MYREOPEN_FILE           "重新打开文件"
#define FIRST_ROW_IS_TITLE      "第1行是列名"
#define FIRST_ROW_NOT_TITLE     "第1行是数据"
#define IMPORT_WAYOF_APPEND     "添加式导入"
#define IMPORT_WAYOF_UPDATE     "更新式导入"
#define LXST_LABEL_FCOLOR       "*{color:#888}"
#define MARGIN_HORIZON_TEXT     6

namespace BailiSoft {

int BsImportSheetDlg::header_rows = 0;
int BsImportSheetDlg::footer_rows = 0;
int BsImportSheetDlg::cargo_col = 1;
int BsImportSheetDlg::color_col = 2;
int BsImportSheetDlg::sizer_col = 3;
int BsImportSheetDlg::qty_col = 4;
int BsImportSheetDlg::sizer_hcols = 0;

BsImportSheetDlg::BsImportSheetDlg(QWidget *parent, BsSheetCargoGrid *grid, const QStringList &csvData) :
    QDialog(parent), mppGrid(grid)
{
    //导入文件主表格
    mpModel = new LxCsvTableModel(csvData, this);
    mpView  = new QTableView(this);
    mpView->setModel(mpModel);
    mpView->setSelectionMode(QAbstractItemView::SingleSelection);
    //mpView->setFocusPolicy(Qt::NoFocus);
    mpView->setStyleSheet("QHeaderView{border-style:none; border-bottom:1px solid silver;} ");

    //设置参数控件
    mpHeaderRows = new QSpinBox(this);
    mpHeaderRows->setValue(BsImportSheetDlg::header_rows);
    mpHeaderRows->setMinimum(0);
    mpHeaderRows->setMaximum(mpModel->rowCount(QModelIndex()) - 1);
    connect(mpHeaderRows, &QSpinBox::valueChanged, this, &BsImportSheetDlg::hideHeadRows);

    mpFooterRows = new QSpinBox(this);
    mpFooterRows->setValue(BsImportSheetDlg::footer_rows);
    mpFooterRows->setMinimum(0);
    mpFooterRows->setMaximum(mpModel->rowCount(QModelIndex()) - 1);
    connect(mpFooterRows, &QSpinBox::valueChanged, this, &BsImportSheetDlg::hideFootRows);

    mpCargoCol = new QSpinBox(this);
    mpCargoCol->setValue(BsImportSheetDlg::cargo_col);
    mpCargoCol->setMinimum(1);
    mpCargoCol->setMaximum(mpModel->columnCount(QModelIndex()));

    mpColorCol = new QSpinBox(this);
    mpColorCol->setValue(BsImportSheetDlg::color_col);
    mpColorCol->setMinimum(1);
    mpColorCol->setMaximum(mpModel->columnCount(QModelIndex()));

    mpSizerCol = new QSpinBox(this);
    mpSizerCol->setValue(BsImportSheetDlg::sizer_col);
    mpSizerCol->setMinimum(1);
    mpSizerCol->setMaximum(mpModel->columnCount(QModelIndex()));

    mpQtyCol = new QSpinBox(this);
    mpQtyCol->setValue(BsImportSheetDlg::qty_col);
    mpQtyCol->setMinimum(1);
    mpQtyCol->setMaximum(mpModel->columnCount(QModelIndex()));

    mpSizerHCols = new QSpinBox(this);
    mpSizerHCols->setValue(BsImportSheetDlg::sizer_hcols);
    mpSizerHCols->setMinimum(0);
    mpSizerHCols->setMaximum(mpModel->columnCount(QModelIndex()) - 2);

    mpBtnOK = new QPushButton(QIcon(":/icon/ok.png"), mapMsg.value("btn_ok"), this);
    mpBtnOK->setIconSize(QSize(16, 16));
    mpBtnOK->setFixedSize(90, 32);
    connect(mpBtnOK, SIGNAL(clicked()), this, SLOT(doImport()));

    QPushButton *pBtnCancel = new QPushButton(QIcon(":/icon/cancel.png"), mapMsg.value("btn_cancel"), this);
    pBtnCancel->setIconSize(QSize(16, 16));
    pBtnCancel->setFixedSize(90, 32);
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    QVBoxLayout *pLayControls = new QVBoxLayout;
    pLayControls->setSpacing(1);

    pLayControls->addWidget(new QLabel(QStringLiteral("头部表外行：")));
    pLayControls->addWidget(mpHeaderRows);
    pLayControls->addSpacing(5);

    pLayControls->addWidget(new QLabel(QStringLiteral("尾部表外行：")));
    pLayControls->addWidget(mpFooterRows);
    pLayControls->addSpacing(25);

    pLayControls->addWidget(new QLabel(QStringLiteral("货号所在列：")));
    pLayControls->addWidget(mpCargoCol);
    pLayControls->addSpacing(5);

    pLayControls->addWidget(new QLabel(QStringLiteral("颜色所在列：")));
    pLayControls->addWidget(mpColorCol);
    pLayControls->addSpacing(5);

    pLayControls->addWidget(new QLabel(QStringLiteral("尺码所在列：")));
    pLayControls->addWidget(mpSizerCol);
    pLayControls->addSpacing(5);

    pLayControls->addWidget(new QLabel(QStringLiteral("数量所在列：")));
    pLayControls->addWidget(mpQtyCol);
    pLayControls->addSpacing(5);

    pLayControls->addWidget(new QLabel(QStringLiteral("尺码横排列数：")));
    pLayControls->addWidget(mpSizerHCols);
    pLayControls->addWidget(new QLabel(QStringLiteral("纵排尺码填0")));

    pLayControls->addSpacing(48);
    pLayControls->addWidget(mpBtnOK);
    pLayControls->addWidget(pBtnCancel);
    pLayControls->addStretch();

    //main layout
    QHBoxLayout *pLayMain = new QHBoxLayout(this);
    pLayMain->addWidget(mpView, 999);
    pLayMain->addSpacing(5);
    pLayMain->addLayout(pLayControls, 0);

    setMinimumSize(780, 500);
    setSizeGripEnabled(true);
    setWindowTitle(QStringLiteral("导入单据"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);

    //init
    mpView->resizeColumnsToContents();
    for ( int i = 0, iLen = mpModel->columnCount(QModelIndex()); i < iLen; ++i ) {
        if (mpView->columnWidth(i) > 200) mpView->setColumnWidth(i, 200);
        if (mpView->columnWidth(i) < 30) mpView->setColumnWidth(i, 30);
    }
    mpModel->resetHeaderRows(BsImportSheetDlg::header_rows);
    mpModel->resetFooterRows(BsImportSheetDlg::footer_rows);
}

void BsImportSheetDlg::hideEvent(QHideEvent *event)
{
    BsImportSheetDlg::header_rows = mpHeaderRows->value();
    BsImportSheetDlg::footer_rows = mpFooterRows->value();
    BsImportSheetDlg::cargo_col = mpCargoCol->value();
    BsImportSheetDlg::color_col = mpColorCol->value();
    BsImportSheetDlg::sizer_col = mpSizerCol->value();
    BsImportSheetDlg::qty_col = mpQtyCol->value();
    BsImportSheetDlg::sizer_hcols = mpSizerHCols->value();

    QDialog::hideEvent(event);
}

void BsImportSheetDlg::hideHeadRows(int count)
{
    mpModel->resetHeaderRows(count);
}

void BsImportSheetDlg::hideFootRows(int count)
{
    mpModel->resetFooterRows(count);
}

void BsImportSheetDlg::doImport()
{
    if (  mpModel->rowCount(QModelIndex()) < 1 ) {
        accept();
        return;
    }

    if ( mpSizerHCols->value() == 1 ) {
        QMessageBox::information(this, QString(), QStringLiteral("横排尺码必须大于1，如果纵排尺码列数应填0！"));
        reject();
        return;
    }

    for (int i = 0; i < mpModel->rowCount(QModelIndex()); ++i) {
        QString cargo = mpModel->data(mpModel->index(i, mpCargoCol->value()-1), Qt::DisplayRole).toString().trimmed();
        QString color = mpModel->data(mpModel->index(i, mpColorCol->value()-1), Qt::DisplayRole).toString().trimmed();
        if ( mpSizerHCols->value() > 0 ) {
            for ( int j = 0; j < mpSizerHCols->value(); j++ ) {
                QString sizerType = dsCargo->getValue(cargo, QStringLiteral("sizertype")).trimmed();
                QString sizer = dsSizer->getSizerNameByIndex(sizerType, j);
                qint64 inputDataQty = 10000 * mpModel->data(mpModel->index(i, mpSizerCol->value()-1+j), Qt::DisplayRole).toString().trimmed().toInt();
                mppGrid->inputNewCargoRow(cargo, color, sizer, inputDataQty, false);
            }
        }
        else {
            QString sizer = mpModel->data(mpModel->index(i, mpSizerCol->value()-1), Qt::DisplayRole).toString().trimmed();
            qint64 inputDataQty = 10000 * mpModel->data(mpModel->index(i, mpQtyCol->value()-1), Qt::DisplayRole).toString().trimmed().toInt();
            mppGrid->inputNewCargoRow(cargo, color, sizer, inputDataQty, false);
        }
    }

    accept();
}

///////////////////////////////////////////////////////////////////////////////

LxCsvTableModel::LxCsvTableModel(const QStringList &lines, QObject *parent) :QAbstractTableModel(parent)
{
    //底层数据
    mLines = lines;

    //解析
    mCols = 1;
    mHeaderRows = 0;
    mFooterRows = 0;

    for ( int i = 0, iLen = lines.length(); i < iLen; ++i ) {
        QString line = lines.at(i);
        QStringList cols = line.split(QRegularExpression(",\\s*(?![^\"]*\"\\,)"));
        if (cols.length() > mCols) {
            mCols = cols.length();
        }
    }
}

int LxCsvTableModel::rowCount(const QModelIndex &) const
{
    return mLines.length() - mHeaderRows - mFooterRows;
}

int LxCsvTableModel::columnCount(const QModelIndex &) const
{
    return mCols;
}

QVariant LxCsvTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole ) {

        int r = index.row();
        int c = index.column();

        QStringList cols = mLines.at(r + mHeaderRows).split(QRegularExpression(",\\s*(?![^\"]*\"\\,)"));
        QString s = (c < cols.length()) ? cols.at(c) : QString();
        if ( s.length() < 2 ) {
            return s.trimmed();
        } else {
            if (s.at(0) == QChar('"') && s.at(s.length()-1) == QChar('"')) {
                return s.mid(1, s.length() - 2).trimmed();  //都是因为CSV格式excel自作聪明的识别导致用户只用excel不用记事本，不得不加的\t
            } else {
                return s.trimmed();
            }
        }
    }

    return QVariant();
}

QVariant LxCsvTableModel::headerData(int section, Qt::Orientation, int role) const
{
    if ( role == Qt::DisplayRole ) {
        return QString::number(section + 1);
    }
    return QVariant();
}

Qt::ItemFlags LxCsvTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    flags |= Qt::ItemIsDragEnabled;
    return flags;
}

void LxCsvTableModel::resetHeaderRows(const int rows)
{
    beginResetModel();
    mHeaderRows = rows;
    endResetModel();
}

void LxCsvTableModel::resetFooterRows(const int rows)
{
    beginResetModel();
    mFooterRows = rows;
    endResetModel();
}


}
