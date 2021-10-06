#include "bsoption.h"
#include "main/bailicode.h"
#include "main/bailidata.h"

#include <QtSql>

#define PROLE_OLD_VALUE     0
#define PROLE_DEF_VALUE     1

namespace BailiSoft {

BsOption::BsOption(QWidget *parent) : QDialog(parent)
{
    mpGrid = new QTableWidget(this);
    mpGrid->setColumnCount(2);
    mpGrid->horizontalHeader()->hide();
    mpGrid->horizontalHeader()->setStretchLastSection(true);
    mpGrid->verticalHeader()->hide();
    mpGrid->verticalHeader()->setDefaultSectionSize(24);
    mpGrid->setColumnWidth(0, 200);
    mpGrid->setMinimumSize(600, 360);

    netEncryptionKeyUpdated = false;
    bossNameChanged = false;

    QPushButton *btnOk      = new QPushButton(QIcon(":/icon/ok.png"), mapMsg.value("btn_ok"), this);
    btnOk->setFixedSize(80, 28);

    QPushButton *btnCancel  = new QPushButton(QIcon(":/icon/cancel.png"), mapMsg.value("btn_cancel"), this);
    btnCancel->setFixedSize(80, 28);

    QPushButton *btnHelp   = new QPushButton(QIcon(":/icon/help.png"), mapMsg.value("btn_help"), this);
    btnHelp->setFixedSize(80, 28);

    QWidget *btnBox = new QWidget(this);
    QHBoxLayout *layBtn = new QHBoxLayout(btnBox);
    layBtn->setContentsMargins(0, 0, 0, 0);
    layBtn->addStretch();
    layBtn->addWidget(btnOk);
    layBtn->addWidget(btnCancel);
    layBtn->addSpacing(12);
    layBtn->addWidget(btnHelp);
    layBtn->addStretch();

    QVBoxLayout *lay = new QVBoxLayout;
    lay->addWidget(mpGrid, 1);
    lay->addWidget(btnBox);
    setLayout(lay);
    setMinimumSize(sizeHint());
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    loadData();

    connect(btnOk, SIGNAL(clicked(bool)), this, SLOT(saveData()));
    connect(btnCancel, SIGNAL(clicked(bool)), this, SLOT(reject()));
    connect(btnHelp, SIGNAL(clicked(bool)), this, SLOT(doHelp()));
}

void BsOption::doHelp()
{
    QDesktopServices::openUrl(QUrl("https://www.bailisoft.com/passage/jyb_sys_preference.html"));
}

void BsOption::saveData()
{
    //检查
    QString newBossAccount;
    QStringList pnames, pvals;
    for ( int i = 0, iLen = mpGrid->rowCount(); i < iLen; ++i ) {
        QTableWidgetItem *itName  = mpGrid->item(i, 0);
        QTableWidgetItem *itValue = mpGrid->item(i, 1);
        if ( itValue->text() != itValue->data(Qt::UserRole + PROLE_OLD_VALUE).toString() ) {
            QString optCode = itName->data(Qt::UserRole).toString();
            QString settingValue = itValue->text();
            pnames << optCode;
            pvals << settingValue;
            if ( optCode == QStringLiteral("app_boss_name") ) {
                newBossAccount = settingValue;
            }
        }
    }
    if ( pnames.length() == 0 ) {
        reject();
        return;
    }

    bossNameChanged = !newBossAccount.isEmpty();
    netEncryptionKeyUpdated = pnames.indexOf(QStringLiteral("app_encryption_key")) >= 0;

    //准备保存
    QStringList sqls;
    if ( !newBossAccount.isEmpty() ) {
        if ( !loginAsBoss ) {
            QMessageBox::information(this, QString(), QStringLiteral("%1自己才能更换总经理账号！").arg(bossAccount));
            return;
        }
        QString sql = QStringLiteral("update baililoginer set loginer='%1' where loginer='%2';")
                .arg(newBossAccount).arg(bossAccount);
        sqls << sql;
    }
    for ( int i = 0, iLen = pnames.length(); i < iLen; ++i ) {
        QString optCode = pnames.at(i);
        QString vsetting = QString(pvals.at(i)).replace(QChar(39), QStringLiteral("''"));
        if ( optCode.startsWith(QStringLiteral("stypes_")) || optCode.endsWith(QStringLiteral("_list")) ) {
            vsetting.replace(QChar(32), QString()).replace(QStringLiteral("，"), QStringLiteral(","));
        }
        QString sql = QString("update bailioption set vsetting='%1' where optcode='%2';").arg(vsetting).arg(optCode);
        sqls << sql;
    }

    //执行保存
    QSqlDatabase defaultdb = QSqlDatabase::database();
    QSqlQuery qry;
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    defaultdb.transaction();
    foreach (QString sql, sqls) {
        qry.exec(sql);
        if ( qry.lastError().isValid() ) {
            defaultdb.rollback();
            QMessageBox::information(this, QString(), QStringLiteral("更改配置未成功！"));
            return;
        }
    }
    defaultdb.commit();

    //关闭对话框
    accept();

    //系统更新
    loginLoadOptions();
}

void BsOption::loadData()
{
    int iRow = 0;
    int logoRowIdx = -1;
    int colorRowIdx = -1;
    QSqlQuery qry;
    qry.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    qry.exec("select optcode, optname, vsetting, vdefault, vformat from bailioption order by optcode;");
    while ( qry.next() ) {
        mpGrid->setRowCount(++iRow);

        QTableWidgetItem *itName = new QTableWidgetItem(qry.value(1).toString());
        itName->setData(Qt::UserRole, qry.value(0));
        itName->setFlags(Qt::NoItemFlags);
        mpGrid->setItem(iRow - 1, 0, itName);

        QTableWidgetItem *itValue = new QTableWidgetItem(qry.value(2).toString());
        itValue->setData(Qt::UserRole + PROLE_OLD_VALUE, qry.value(2).toString());
        itValue->setData(Qt::UserRole + PROLE_DEF_VALUE, qry.value(3).toString());
        itValue->setToolTip(qry.value(4).toString());
        mpGrid->setItem(iRow - 1, 1, itValue);

        if ( qry.value(0).toString() == "app_company_plogo" ) {
            logoRowIdx = iRow - 1;
        }

        if ( qry.value(0).toString() == "app_company_pcolor" ) {
            colorRowIdx = iRow - 1;
        }
    }

    if ( logoRowIdx >= 0 ) {
        mpGrid->setItemDelegateForRow(logoRowIdx, new LxPicturePickDelegate(1, logoRowIdx, mpGrid));
    }

    if ( colorRowIdx >= 0 ) {
        mpGrid->setItemDelegateForRow(colorRowIdx, new LxColorPickDelegate(1, colorRowIdx, mpGrid));
    }
}

/*****************************************************************************/

LxPicturePickDelegate::LxPicturePickDelegate(const int colIdx, const int rowIdx, QTableWidget *parent)
    : QStyledItemDelegate(parent)
{
    mppView = parent;
    mColIdx = colIdx;
    mRowIdx = rowIdx;
}

QWidget *LxPicturePickDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ( index.column() == mColIdx ) {
        QToolButton *btn = new QToolButton(parent);
        btn->setIcon(QIcon(":/icon/openfile.png"));
        btn->setIconSize(QSize(48, 48));
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        connect(btn, SIGNAL(clicked(bool)), this, SLOT(commitAndCloseEditor()));
        return btn;
    } else {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void LxPicturePickDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ( index.column() == mColIdx ) {
        QString imgData = index.model()->data(index, Qt::DisplayRole).toString();
        if ( imgData.length() > 100 ) {
            QRect rect(option.rect.left(), option.rect.top(), option.rect.height(), option.rect.height());
            QImage img;
            QByteArray baData = imgData.toLatin1();
            img.loadFromData(QByteArray::fromBase64(baData));
            painter->drawImage(rect, img);
        }
        else {
            QString strShow = QStringLiteral("图片（300x300）");
            painter->drawText(option.rect.adjusted(3, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, strShow);
        }
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

void LxPicturePickDelegate::commitAndCloseEditor()
{
    QString label = QStringLiteral("图片文件：");
    QString filter = QStringLiteral("图片文件(*.png)");
    QString deskPath = QStandardPaths::locate(QStandardPaths::DesktopLocation,"", QStandardPaths::LocateDirectory);
    QFileDialog *dlg = new QFileDialog(nullptr, label, deskPath, filter);
    dlg->deleteLater();
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setFileMode(QFileDialog::ExistingFile);
    #ifdef Q_OS_MAC
    dlg->setOption(QFileDialog::DontUseNativeDialog, true);
    #endif

    dlg->setLabelText(QFileDialog::FileName, label);
    if ( dlg->exec() != QDialog::Accepted ) {
        return;
    }

    QStringList files = dlg->selectedFiles();
    if ( files.isEmpty() ) {
        return;
    }

    QImage imgSrc(files.at(0));
    QImage imgDst = imgSrc.scaled(QSize(300, 300), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QByteArray imgData;
    QBuffer buffer(&imgData);

    buffer.open(QIODevice::WriteOnly);
    imgDst.save(&buffer, "png");
    buffer.close();

    QModelIndex idx = mppView->model()->index(mRowIdx, mColIdx);
    mppView->model()->setData(idx, QString(imgData.toBase64()));

    QToolButton *btn = qobject_cast<QToolButton *>(sender());
    emit commitData(btn);
    emit closeEditor(btn);
}


/*****************************************************************************/

LxColorPickDelegate::LxColorPickDelegate(const int colIdx, const int rowIdx, QTableWidget *parent)
    : QStyledItemDelegate(parent)
{
    mppView = parent;
    mColIdx = colIdx;
    mRowIdx = rowIdx;
}

QWidget *LxColorPickDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ( index.column() == mColIdx ) {
        QToolButton *btn = new QToolButton(parent);
        btn->setIcon(QIcon(":/icon/colors.png"));
        btn->setIconSize(QSize(48, 48));
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        connect(btn, SIGNAL(clicked(bool)), this, SLOT(commitAndCloseEditor()));
        return btn;
    } else {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void LxColorPickDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ( index.column() == mColIdx ) {
        QString text = index.model()->data(index, Qt::DisplayRole).toString();
        if ( text.length() == 6 ) {
            QString r = text.left(2);
            QString g = text.mid(2, 2);
            QString b = text.right(2);
            bool ok;
            painter->fillRect(option.rect.adjusted(1, 1, option.rect.height() - option.rect.width(), -1),
                              QBrush(QColor(r.toInt(&ok, 16), g.toInt(&ok, 16), b.toInt(&ok, 16))));
        }
        else {
            QString strShow = QStringLiteral("双击选择");
            painter->drawText(option.rect.adjusted(3, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, strShow);
        }
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

void LxColorPickDelegate::commitAndCloseEditor()
{
    QColor color = QColorDialog::getColor();
    if ( color == QColor(Qt::white) ) color = QColor(0x11, 0x99, 0x00);
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    QString data = QStringLiteral("%1%2%3")
            .arg(r, 2, 16, QLatin1Char('0'))
            .arg(g, 2, 16, QLatin1Char('0'))
            .arg(b, 2, 16, QLatin1Char('0'));

    QModelIndex idx = mppView->model()->index(mRowIdx, mColIdx);
    mppView->model()->setData(idx, data);

    QToolButton *btn = qobject_cast<QToolButton *>(sender());
    emit commitData(btn);
    emit closeEditor(btn);
}


}

