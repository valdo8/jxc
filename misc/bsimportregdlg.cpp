#include "bsimportregdlg.h"
#include "comm/lxstringtablemodel.h"
#include "comm/bsflowlayout.h"
#include "comm/pinyincode.h"
#include "main/bailifunc.h"
#include "main/bailicode.h"
#include "main/bailigrid.h"

#define MAIN_HELP_STEP          "第一步：打开文件；\n第二步：指定文件第1行是数据还是列名称;\n第三步：指定文件各列分别对应什么列，方式为直接用鼠标拖动匹配，双击取消。其中，红色为必填项，不可忽略。"
#define MYREOPEN_FILE           "重新打开文件"
#define FIRST_ROW_IS_TITLE      "第1行是列名"
#define FIRST_ROW_NOT_TITLE     "第1行是数据"
#define IMPORT_WAYOF_APPEND     "添加式导入"
#define IMPORT_WAYOF_UPDATE     "更新式导入"
#define LXST_LABEL_FCOLOR       "*{color:#888}"
#define MARGIN_HORIZON_TEXT     6

namespace BailiSoft {

BsImportRegDlg::BsImportRegDlg(QWidget *parent, BsRegGrid *grid) :
    QDialog(parent), mppGrid(grid)
{
    //待导入列池
    BsFlowLayout *pLayFields = new BsFlowLayout();
    for (int i = 0; i < grid->mCols.length(); ++i) {
        BsField *col = grid->mCols.at(i);
        uint flags = col->mFlags;
        if ( (flags & bsffHideSys) != bsffHideSys && (flags & bsffBool) != bsffBool ) {
            bool required = (i == 0);
            LxDragField *pTmp = new LxDragField(col->mFldName, col->mFldCnName, required, this);
            pTmp->setMinimumWidth(pTmp->sizeHint().width() + 20);
            pTmp->setWhatsThis(QStringLiteral(MAIN_HELP_STEP));
            connect(pTmp, SIGNAL(cordChanged()), this, SLOT(doCheckFinishOK()));
            pLayFields->addWidget(pTmp);
        }
    }

    //首行是否标题
    mpPnlFirstRowState = new QWidget(this);
    QGridLayout *pLayFirstRowState = new QGridLayout(mpPnlFirstRowState);
    mpFirstRowIsTitle = new QRadioButton(QStringLiteral("%1?").arg(QStringLiteral(FIRST_ROW_IS_TITLE)), mpPnlFirstRowState);
    connect(mpFirstRowIsTitle, SIGNAL(toggled(bool)), SLOT(doSetFirstRowColor(bool)));
    mpFirstRowNotTitle = new QRadioButton(QStringLiteral("%1?").arg(QStringLiteral(FIRST_ROW_NOT_TITLE)), mpPnlFirstRowState);
    connect(mpFirstRowNotTitle, SIGNAL(toggled(bool)), SLOT(doCheckFinishOK()));
    pLayFirstRowState->addWidget(mpFirstRowIsTitle,     0, 0, 1, 3, Qt::AlignRight);
    pLayFirstRowState->addWidget(new QLabel(" "),      0, 3, 1, 1);
    pLayFirstRowState->addWidget(mpFirstRowNotTitle,    0, 4, 1, 3, Qt::AlignLeft);
    pLayFirstRowState->setContentsMargins(0, 0, 0, 0);
    mpPnlFirstRowState->setMaximumHeight(mpFirstRowIsTitle->sizeHint().height());
    mpPnlFirstRowState->setStyleSheet("*{color:#A66}");
    mpPnlFirstRowState->hide();

    //导入文件主表格
    mpModel = new LxStringTableModel(QString(), this);
    mpView  = new LxImportTableView(this);
    mpView->setModel(mpModel);
    mpView->setSelectionMode(QAbstractItemView::SingleSelection);
    mpView->setSelectionBehavior(QAbstractItemView::SelectColumns);
    mpView->setFocusPolicy(Qt::NoFocus);     //去虚线框
    //mpView->setStyleSheet("*::item::selected{background-color: white; color: black;}");
    mpView->setAcceptDrops(true);
    mpView->setWhatsThis(QStringLiteral(MAIN_HELP_STEP));

    //结束确定取消按钮面板
    mpPnlImportWay = new QWidget(this);
    QGridLayout *pLayImportWay = new QGridLayout(mpPnlImportWay);
    mpImportByAppend = new QRadioButton(QStringLiteral("%1?").arg(QStringLiteral(IMPORT_WAYOF_APPEND)), mpPnlImportWay);
    connect(mpImportByAppend, SIGNAL(toggled(bool)), SLOT(doSetImportWayReady()));
    mpImportByUpdate = new QRadioButton(QStringLiteral("%1?").arg(QStringLiteral(IMPORT_WAYOF_UPDATE)), mpPnlImportWay);
    connect(mpImportByUpdate, SIGNAL(toggled(bool)), SLOT(doCheckFinishOK()));
    pLayImportWay->addWidget(mpImportByAppend, 0, 0, 1, 3, Qt::AlignRight);
    pLayImportWay->addWidget(new QLabel(" "),  0, 3, 1, 1);
    pLayImportWay->addWidget(mpImportByUpdate, 0, 4, 1, 3, Qt::AlignLeft);
    pLayImportWay->setContentsMargins(0, 0, 0, 0);
    mpPnlImportWay->setMaximumHeight(mpImportByAppend->sizeHint().height());
    mpPnlImportWay->setStyleSheet("*{color:#A66}");
    mpPnlImportWay->hide();

    mpBtnOK = new QPushButton(QIcon(":/icon/ok.png"), QStringLiteral("确定"), this);
    mpBtnOK->setIconSize(QSize(16, 16));
    connect(mpBtnOK, SIGNAL(clicked()), this, SLOT(checkValid()));
    mpBtnOK->setEnabled(false);

    QPushButton *pBtnCancel = new QPushButton(QIcon(":/icon/cancel.png"), QStringLiteral("取消"), this);
    pBtnCancel->setIconSize(QSize(16, 16));
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    QPushButton *pBtnHelp = new QPushButton(QIcon(":/icon/help.png"), QStringLiteral("帮助"), this);
    pBtnHelp->setIconSize(QSize(16, 16));
    connect(pBtnHelp, SIGNAL(clicked()), this, SLOT(doHelp()));

    mpLayOKCancel = new QHBoxLayout;
    mpLayOKCancel->addStretch();
    mpLayOKCancel->addSpacing(8);
    mpLayOKCancel->addWidget(mpBtnOK);
    mpLayOKCancel->addWidget(pBtnCancel);
    mpLayOKCancel->addSpacing(24);
    mpLayOKCancel->addWidget(pBtnHelp);

    QVBoxLayout *pLayMain = new QVBoxLayout(this);
    pLayMain->setContentsMargins(8, 8, 8, 8);
    pLayMain->addLayout(pLayFields, 0);
    pLayMain->addWidget(mpPnlFirstRowState, 0);
    pLayMain->addWidget(mpView, 999);
    pLayMain->addWidget(mpPnlImportWay, 0);
    pLayMain->addLayout(mpLayOKCancel, 0);

    mpBtnOpen = new QToolButton(this);
    mpBtnOpen->setIcon(QIcon(":/icon/openfile.png"));
    mpBtnOpen->setText(QStringLiteral("打开文件"));
    mpBtnOpen->setObjectName("btnFileOpen"); //约定
    mpBtnOpen->setIconSize(QSize(64, 32));
    mpBtnOpen->setMinimumWidth(100);
    mpBtnOpen->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    connect(mpBtnOpen, SIGNAL(clicked()), this, SLOT(doFileOpen()));

    setMinimumSize(750, 500);
    setSizeGripEnabled(true);
    setWindowTitle(QStringLiteral("导入"));

    //清除初始化设置
    clearCordSettings();
    connect(this, SIGNAL(accepted()), this, SLOT(doImport()));
}

int BsImportRegDlg::getCordIndexOf(const QString &prFld)
{
    QList<LxDragField *> ls = findChildren<LxDragField *>();
    for (int i = 0; i < ls.size(); ++i) {
        LxDragField *pFld = ls.at(i);
        if (pFld->fldName == prFld && pFld->cordIndex >= 0)
            return pFld->cordIndex;
    }

    return -1;
}

void BsImportRegDlg::resizeEvent(QResizeEvent *)
{
    if (mpModel->rowCount(QModelIndex()) == 0) {
        mpBtnOpen->setGeometry(
                    (mpView->width() - mpBtnOpen->width()) / 2,
                    mpView->pos().y() + (mpView->height() - mpBtnOpen->height()) / 2,
                    mpBtnOpen->sizeHint().width(),
                    mpBtnOpen->sizeHint().height());
    }
}

void BsImportRegDlg::doHelp()
{
    QDesktopServices::openUrl(QUrl("https://www.bailisoft.com/passage/jyb_import_guide.html"));
}

void BsImportRegDlg::doFileOpen()
{
    QString dir = QStandardPaths::locate(QStandardPaths::DesktopLocation, QString(), QStandardPaths::LocateDirectory);
    QString openData = openLoadTextFile(mapMsg.value("tool_import_data"), dir, mapMsg.value("i_formatted_csv_file"), this);
    if ( openData.isEmpty() )
        return;

    //数据
    mpModel->resetData(openData);

    //处理界面
    QToolButton *pBtnTmp = mpLayOKCancel->findChild<QToolButton *>("btnFileOpen");
    if (!pBtnTmp) {
        mpBtnOpen->setText(QStringLiteral(MYREOPEN_FILE));
        mpBtnOpen->setIconSize(QSize(24, 24));
        mpBtnOpen->setMinimumWidth(60);
        mpBtnOpen->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        mpBtnOpen->setAutoRaise(true);
        mpLayOKCancel->insertWidget(0, mpBtnOpen);
    }
    mpPnlFirstRowState->show();
    mpPnlImportWay->show();

    //清除设置
    clearCordSettings();
}

void BsImportRegDlg::doCheckFinishOK()
{
    QList<LxDragField *> ls = findChildren<LxDragField *>();
    bool allCorded = true;
    for (int i = 0; i < ls.size(); ++i) {
        if (ls.at(i)->isRequired && ls.at(i)->cordIndex < 0) {
            allCorded = false;
            break;
        }
    }

    if ( mpFirstRowIsTitle->isChecked() || mpFirstRowNotTitle->isChecked() ) {
        mpFirstRowIsTitle->setText(QStringLiteral(FIRST_ROW_IS_TITLE));
        mpFirstRowNotTitle->setText(QStringLiteral(FIRST_ROW_NOT_TITLE));
        mpPnlFirstRowState->setStyleSheet("*{color:#080}");
    }

    if ( mpImportByAppend->isChecked() || mpImportByUpdate->isChecked() ) {
        mpImportByAppend->setText(QStringLiteral(IMPORT_WAYOF_APPEND));
        mpImportByUpdate->setText(QStringLiteral(IMPORT_WAYOF_UPDATE));
        mpPnlImportWay->setStyleSheet("*{color:#080}");
    }

    mpBtnOK->setEnabled(allCorded && (mpFirstRowIsTitle->isChecked() || mpFirstRowNotTitle->isChecked()) &&
                                     (mpImportByAppend->isChecked() || mpImportByUpdate->isChecked()) );
}

void BsImportRegDlg::doSetFirstRowColor(const bool isTitle)
{
    if (isTitle) {
        mpModel->setFirstRowColor(true);
        autoCordinateCols();
    }
    else
        mpModel->setFirstRowColor(false);

    if ( mpFirstRowIsTitle->isChecked() || mpFirstRowNotTitle->isChecked() ) {
        mpFirstRowIsTitle->setText(QStringLiteral(FIRST_ROW_IS_TITLE));
        mpFirstRowNotTitle->setText(QStringLiteral(FIRST_ROW_NOT_TITLE));
        mpPnlFirstRowState->setStyleSheet("*{color:#080}");
    }
}

void BsImportRegDlg::doSetImportWayReady()
{
    doCheckFinishOK();
    if ( mpImportByAppend->isChecked() || mpImportByUpdate->isChecked() ) {
        mpImportByAppend->setText(QStringLiteral(IMPORT_WAYOF_APPEND));
        mpImportByUpdate->setText(QStringLiteral(IMPORT_WAYOF_UPDATE));
        mpPnlImportWay->setStyleSheet("*{color:#080}");
    }
}

void BsImportRegDlg::checkValid()
{
    //首行开始索引
    mFirstDataRowIdx = (mpFirstRowIsTitle->isChecked()) ? 1 : 0;

    //记录列匹配
    lstCords.clear();
    for (int i = 0; i < mppGrid->mCols.size(); ++i) {
        BsField *col = mppGrid->mCols.at(i);
        int idxCord = getCordIndexOf(col->mFldName);
        if (idxCord >= 0) {
            lstCords.append(qMakePair(idxCord, i));     //srcIdx, dstIdx
        }
    }
    if ( lstCords.isEmpty() || lstCords.at(0).second != 0 ) {
        QMessageBox::information(this, QString(), QStringLiteral("主列必须指定！"));
        reject();
        return;
    }
    accept();
}

void BsImportRegDlg::doImport()
{
    //两种情况
    if ( mpImportByAppend->isChecked() ) {
        //遍历数据源行
        for (int i = mFirstDataRowIdx; i < mpModel->rowCount(QModelIndex()); ++i) {
            //主值
            QModelIndex srcKeyIdx = mpModel->index(i, lstCords.at(0).first);
            QString keyValue = mpModel->data(srcKeyIdx, Qt::DisplayRole).toString();

            //没有主值重复的行才可以添加
            int row = mppGrid->findRowByKeyValue(keyValue);
            if ( row < 0 ) {
                //添新行
                mppGrid->appendNewRow();
                int newRow = mppGrid->rowCount() - 1;

                //填值
                for (int j = 0, jLen = lstCords.size(); j < jLen; ++j) {
                    QPair<int, int> cordPair = lstCords.at(j);
                    QModelIndex srcCellIdx = mpModel->index(i, cordPair.first);
                    mppGrid->item(newRow, cordPair.second)->setText(mpModel->data(srcCellIdx, Qt::DisplayRole).toString().trimmed());
                }
            }
        }
    }
    else {
        //遍历数据源行
        for (int i = mFirstDataRowIdx; i < mpModel->rowCount(QModelIndex()); ++i) {
            //主值
            QModelIndex srcKeyIdx = mpModel->index(i, lstCords.at(0).first);
            QString keyValue = mpModel->data(srcKeyIdx, Qt::DisplayRole).toString();

            //找得到得行才更新
            int row = mppGrid->findRowByKeyValue(keyValue);
            if ( row >= 0 ) {
                //更新值
                for (int j = 0, jLen = lstCords.size(); j < jLen; ++j) {
                    QPair<int, int> cordPair = lstCords.at(j);
                    QModelIndex srcCellIdx = mpModel->index(i, cordPair.first);
                    mppGrid->item(row, cordPair.second)->setText(mpModel->data(srcCellIdx, Qt::DisplayRole).toString().trimmed());
                }
                //设状态
                mppGrid->updateRowState(row);
            }
        }
    }
}

void BsImportRegDlg::clearCordSettings()
{
    mpFirstRowIsTitle->setAutoExclusive(false);
    mpFirstRowNotTitle->setAutoExclusive(false);
    mpFirstRowIsTitle->setChecked(false);
    mpFirstRowNotTitle->setChecked(false);
    mpFirstRowIsTitle->setAutoExclusive(true);
    mpFirstRowNotTitle->setAutoExclusive(true);
    mpFirstRowIsTitle->setText(QStringLiteral("%1?").arg(QStringLiteral(FIRST_ROW_IS_TITLE)));
    mpFirstRowNotTitle->setText(QStringLiteral("%1?").arg(QStringLiteral(FIRST_ROW_NOT_TITLE)));
    mpPnlFirstRowState->setStyleSheet("*{color:#A66}");
    mpModel->setFirstRowColor(false);

    QList<LxDragField *> ls = findChildren<LxDragField *>();
    for (int i = 0; i < ls.size(); ++i) {
        LxDragField *pFld = ls.at(i);
        pFld->cordIndex = -1;
        pFld->setText(QString("%1\n<?>").arg(pFld->fldTitle));
        pFld->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        if (pFld->isRequired)
            pFld->setStyleSheet("*{color:#A66}");
        else
            pFld->setStyleSheet(LXST_LABEL_FCOLOR);
        pFld->setMinimumWidth(pFld->fontMetrics().horizontalAdvance(pFld->text()) + MARGIN_HORIZON_TEXT);
    }
}

void BsImportRegDlg::autoCordinateCols()
{
    QList<LxDragField *> ls = findChildren<LxDragField *>();

    bool allCorded = true;
    for (int i = 0; i < ls.size(); ++i) {
        LxDragField *pFld = ls.at(i);
        if (pFld->cordIndex >= 0)
            continue;

        for (int col = 0; col < mpModel->columnCount(QModelIndex()); ++col) {
           QString valFirstRow = mpModel->index(0, col, QModelIndex()).data().toString();
           if (valFirstRow == pFld->fldTitle) {
                pFld->cordIndex = col;
                pFld->setText(QString("%1\n<%2>").arg(pFld->fldTitle).arg(valFirstRow));
                pFld->setStyleSheet("*{color:#080}");
                pFld->setFrameStyle(QFrame::Panel | QFrame::Raised);
                break;
           }
        }

        if (pFld->isRequired && pFld->cordIndex < 0)
            allCorded = false;
    }

    if (allCorded && (mpFirstRowIsTitle->isChecked() || mpFirstRowNotTitle->isChecked()) &&
                     (mpImportByAppend->isChecked() || mpImportByUpdate->isChecked()))
        mpBtnOK->setEnabled(true);
}


/////////////////////////////////////////////////////////////////////////////////////////////

LxDragField::LxDragField(const QString &prFldName, const QString &prFldTitle, const bool prRequired, QWidget *parent)
    : QLabel(parent), fldName(prFldName), fldTitle(prFldTitle), cordIndex(-1), isRequired(prRequired)
{
    setMargin(5);
    setLineWidth(2);
    setAlignment(Qt::AlignCenter);
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setAcceptDrops(true);
}

void LxDragField::mousePressEvent(QMouseEvent *)
{
    cordIndex = -1;
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    if (isRequired)
        setStyleSheet("*{color:#A66}");
    else
        setStyleSheet(LXST_LABEL_FCOLOR);
    setText(QString("%1\n<?>").arg(fldTitle));

    emit cordChanged();
}

void LxDragField::dragEnterEvent(QDragEnterEvent *event)
{
    if (cordIndex < 0) {
        setFrameStyle(QFrame::Panel | QFrame::Raised);
        if (isRequired)
            setStyleSheet("*{color:#D00}");
        else
            setStyleSheet("*{color:#000}");
    }
    else {
        QFont f(font());
        f.setItalic(true);
        setFont(f);
    }
    event->accept();
}

void LxDragField::dragLeaveEvent(QDragLeaveEvent *event)
{
    if (cordIndex < 0) {
        setFrameStyle(QFrame::Panel | QFrame::Sunken);
        if (isRequired)
            setStyleSheet("*{color:#A66}");
        else
            setStyleSheet(LXST_LABEL_FCOLOR);
    }
    else {
        QFont f(font());
        f.setItalic(false);
        setFont(f);
    }
    event->accept();
}

void LxDragField::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void LxDragField::dropEvent(QDropEvent *event)
{
    QString sDrag = event->mimeData()->text();
    int iPos = sDrag.indexOf("|");
    QString sSrcCol = sDrag.mid(iPos + 1);
    if (iPos > 0) {
        bool intTranOk = false;
        cordIndex = sDrag.left(iPos).toInt(&intTranOk);
        if (intTranOk) {
            setText(QString("%1\n<%2>").arg(fldTitle).arg(sSrcCol));
            setStyleSheet("*{color:#080}");
        } else
            cordIndex = -1;
    }

    emit cordChanged();

    event->accept();
}

/////////////////////////////////////////////////////////////////////////////////////////////

LxImportTableView::LxImportTableView(QWidget *parent) : QTableView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setDefaultSectionSize(20);
    horizontalHeader()->hide();
}

void LxImportTableView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        mStartPos = event->pos();
    QTableView::mousePressEvent(event);
}

void LxImportTableView::mouseMoveEvent(QMouseEvent *event)
{
    if (model()->columnCount() < 1 || model()->rowCount() < 1) {
        QTableView::mouseMoveEvent(event);
        return;
    }

    if (event->buttons() & Qt::LeftButton){
        int distance = (event->pos() - mStartPos).manhattanLength();
        if (distance >= QApplication::startDragDistance()){

            int idxCol = currentIndex().column();
            QMimeData *mimeData = new QMimeData;
            QString sFirstRowCell = currentIndex().sibling(0, idxCol).data().toString();
            mimeData->setText(QString("%1|%2").arg(idxCol).arg(sFirstRowCell));

            int visualX = columnViewportPosition(idxCol) + verticalHeader()->width();
            int colW = columnWidth(idxCol);
            int rowsH = height() - horizontalScrollBar()->height();

            QPixmap pixmap(colW, rowsH);
            QPainter painter(&pixmap);
            painter.end();
            render(&pixmap, QPoint(0, 0), QRegion(QRect(visualX, 0, colW, rowsH)));

            QDrag *drag = new QDrag(this);
            drag->setMimeData(mimeData);
            drag->setPixmap(pixmap);
            drag->setHotSpot(QPoint(0, 0));

            drag->exec();
        }
    }
    QTableView::mouseMoveEvent(event);
}

void LxImportTableView::dragEnterEvent(QDragEnterEvent *event)
{
    event->ignore();
}

void LxImportTableView::dragMoveEvent(QDragMoveEvent *event)
{
    event->ignore();
}

void LxImportTableView::dropEvent(QDropEvent *event)
{
    event->ignore();
}


}
