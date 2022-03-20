#include "bailidialog.h"
#include "bailicode.h"
#include "bailidata.h"
#include "bailigrid.h"
#include "bailiedit.h"

namespace BailiSoft {

BsDialog::BsDialog(QWidget *parent, const bool addMode, const QString &table, const QList<BsField *> &fields)
    : QDialog(parent), mAddMode(addMode), mTable(table), mFields(fields)
{
    //标题
    QFont f(font());
    f.setBold(true);
    f.setPointSize(2 * f.pointSize());
    mpLblTitle = new QLabel(this);
    mpLblTitle->setFont(f);
    mpLblTitle->setAlignment(Qt::AlignCenter);

    QString act = (addMode) ? mapMsg.value("word_append") : mapMsg.value("word_change");
    mpLblTitle->setText( act + mapMsg.value(QStringLiteral("reg_%1").arg(table)));
    if ( addMode )
        mpLblTitle->setStyleSheet(QLatin1String("color:#080;"));
    else
        mpLblTitle->setStyleSheet(QLatin1String("color:blue;"));

    //求得标签宽度
    QFontMetrics fm(font());
    int labelMaxWidth = 0;
    for ( int i = 0, iLen = fields.length(); i < iLen; ++i ) {
        QString title = getShortName(fields.at(i)->mFldCnName);
        int w = fm.horizontalAdvance(title);
        if ( labelMaxWidth < w ) labelMaxWidth = w;
    }

    //form
    QWidget *pnlForm = new QWidget(this);
    QVBoxLayout *layForm = new QVBoxLayout(pnlForm);
    for ( int i = 0, iLen = fields.length(); i < iLen; ++i ) {
        BsField *fld = fields.at(i);
        QString fldName = fields.at(i)->mFldName;
        uint flags = fld->mFlags;

        if ( (flags & bsffHideSys) != bsffHideSys && (flags & bsffReadOnly) != bsffReadOnly ) {

            //model
            BsAbstractModel *dropModel = nullptr;
            if ( fldName.startsWith(QStringLiteral("attr")) && fldName.mid(4).toInt() > 0 ) {
                QString listSql = QStringLiteral("select vsetting from bailioption where optcode='%1_%2_list';")
                        .arg(table).arg(fldName);
                BsSqlListModel *ds = new BsSqlListModel(this, listSql);
                ds->reload();
                dropModel = ds;
                mAttrModels << ds;
            }
            if ( fldName == QStringLiteral("sizertype") ) {
                dropModel = dsSizer;
                dsSizer->reload();
            }
            if ( fldName == QStringLiteral("colortype") ) {
                dropModel = dsColorType;
                dsColorType->reload();
            }

            //editor create
            BsFldBox *editor = new BsFldBox(this, fld, dropModel);
            editor->installEventFilter(this);
            layForm->addWidget(editor);

            //editor properties
            QStringList defs = mapMsg.value(QStringLiteral("fld_%1").arg(fldName)).split(QChar(9));
            Q_ASSERT(defs.length() > 2);
            QString title = getShortName(fields.at(i)->mFldCnName);
            QString hint = defs.at(2);
            if ( addMode ) editor->setGreenEditorStyle();
            editor->setMinimumWidth(500);
            editor->mpEditor->setMyPlaceText(hint);
            editor->mpEditor->setMyPlaceColor(QColor(200, 200, 200));
            editor->mpLabel->setText(title);
            editor->mpLabel->setFixedWidth(labelMaxWidth);
            editor->mpLabel->setStyleSheet(QLatin1String("color:black;"));

            //特殊
            if ( fldName == QStringLiteral("regdis") ) {
                editor->mpEditor->setText(QStringLiteral("1.000"));
            }
            if ( (flags & bsffBool) == bsffBool ) {
                editor->mpEditor->setText(QStringLiteral("是"));
            }

            //keep
            mEditors << editor;
        }
    }

    //对话框
    mpBtnOk = new QPushButton(QIcon(":/icon/ok.png"), QStringLiteral("确定"), this);
    mpBtnOk->setIconSize(QSize(20, 20));
    connect(mpBtnOk, &QPushButton::clicked, this, &BsDialog::accept);

    QPushButton *pBtnCancel = new QPushButton(QIcon(":/icon/cancel.png"), QStringLiteral("取消"), this);
    pBtnCancel->setIconSize(QSize(20, 20));
    connect(pBtnCancel, &QPushButton::clicked, this, &BsDialog::reject);

    mpBtnHelp = new QPushButton(QIcon(":/icon/help.png"), QStringLiteral("帮助"), this);
    mpBtnHelp->setIconSize(QSize(20, 20));
    connect(mpBtnHelp, &QPushButton::clicked, this, &BsDialog::openHelpPage);

    QHBoxLayout* layButtonBox = new QHBoxLayout;
    layButtonBox->setSpacing(10);
    layButtonBox->addStretch();
    layButtonBox->addWidget(mpBtnOk);
    layButtonBox->addWidget(pBtnCancel);
    layButtonBox->addSpacing(20);
    layButtonBox->addWidget(mpBtnHelp);
    layButtonBox->addStretch();

    //总部布局
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setSpacing(0);
    lay->setContentsMargins(9, 9, 9, 18);
    lay->addWidget(mpLblTitle);
    lay->addWidget(pnlForm, 1);
    lay->addLayout(layButtonBox);
    setMinimumSize(sizeHint());
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);

    //白色背景
    QPalette pal(palette());
    pal.setColor(QPalette::Window, Qt::white);
    setAutoFillBackground(true);
    setPalette(pal);

    //初始
    mpBtnOk->setEnabled(!addMode);
    connect(mEditors.at(0)->mpEditor, &BsFldEditor::textChanged, this, &BsDialog::updateButtonState);

    //TODO...help
}

BsDialog::~BsDialog()
{
    qDeleteAll(mAttrModels);
    qDeleteAll(mEditors);
}

BsFldBox *BsDialog::getEditorByField(const QString &fieldName)
{
    for ( int i = 0, iLen = mEditors.length(); i < iLen; ++i ) {
        if ( mEditors.at(i)->mpEditor->mpField->mFldName == fieldName ) {
            return mEditors.at(i);
        }
    }
    return nullptr;
}

bool BsDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter
                || keyEvent->key() == Qt::Key_Down) {
            focusNextChild();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Up) {
            focusPreviousChild();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void BsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if ( mAddMode ) {
        mpBtnHelp->setFocus();
    } else {
        mEditors.at(0)->mpLabel->setStyleSheet(QLatin1String("font-weight:900;"));
        mEditors.at(0)->mpEditor->setStyleSheet(QLatin1String("font-weight:900; color:black;"));
        mEditors.at(0)->mpEditor->setEnabled(false);
        mEditors.at(0)->mpEditor->setToolTip(QStringLiteral("主值不可更改，如果输错，可以删除后重新添加。"));
    }
}

void BsDialog::openHelpPage()
{
    QString url = QStringLiteral("https://www.bailisoft.com/passage/open19_win_regis.html?win=%1").arg(mTable);
    QDesktopServices::openUrl(QUrl(url));
}

void BsDialog::updateButtonState()
{
    mpBtnOk->setEnabled(!mEditors.at(0)->mpEditor->text().trimmed().isEmpty());
}

QString BsDialog::getShortName(const QString &longName)
{
    QString txt = longName;
    return txt.replace(QRegularExpression("："), QString())
            .replace(QRegularExpression("（"), QString())
            .replace(QRegularExpression("）"), QString());
}

BsField *BsDialog::getFieldByName(const QString &name)
{
    for ( int i = 0, iLen = mFields.length(); i < iLen; ++i ) {
        BsField *fld = mFields.at(i);
        if ( fld->mFldName == name ) {
            return fld;
        }
    }
    return nullptr;
}

}
