#include "bstagselectdlg.h"
#include "main/bailicode.h"
#include "main/bailidata.h"

#define UPIMAGE_MAX_WIDTH   1000
#define UPIMAGE_MAX_HEIGHT  1300

namespace BailiSoft {

BsTagSelectDlg::BsTagSelectDlg(QWidget *parent, const QString &cargo, const QStringList &tags,
                               const QString tagged)
    : QDialog(parent), mCargo(cargo), mTags(tags), mTagged(tagged)
{
    //标签表（左）
    mpTagList = new QListWidget(this);
    mpTagList->addItems(tags);
    mpTagList->setCurrentIndex(QModelIndex());  //setCurrentRow not works

    QLabel* lblTagMore = new QLabel(QStringLiteral("更多标签，请联系平台。"), this);
    lblTagMore->setStyleSheet(QLatin1String("color:#666;"));

    QGroupBox *pnlList = new QGroupBox(this);
    pnlList->setTitle(QStringLiteral("请选择品类标签："));
    pnlList->setFixedWidth(220);
    QVBoxLayout *layList = new QVBoxLayout(pnlList);
    layList->addWidget(mpTagList, 1);
    layList->addWidget(lblTagMore, 0, Qt::AlignCenter);

    //图片（中）
    QPushButton *btnLoadImage = new QPushButton(QIcon(":/icon/openfile.png"), QStringLiteral("加载图片"), this);
    btnLoadImage->setIconSize(QSize(20, 20));
    btnLoadImage->setFixedWidth(120);
    connect(btnLoadImage, &QPushButton::clicked, this, &BsTagSelectDlg::loadImage);

    mpLblImage = new QLabel(this);
    mpLblImage->setFixedSize(UPIMAGE_MAX_WIDTH / 2, UPIMAGE_MAX_HEIGHT / 2);

    QGroupBox *pnlImage = new QGroupBox(this);
    pnlImage->setTitle(QStringLiteral("展示图片："));
    QVBoxLayout *layImage = new QVBoxLayout(pnlImage);
    layImage->addWidget(btnLoadImage, 0, Qt::AlignCenter);
    layImage->addWidget(mpLblImage, 1, Qt::AlignCenter);

    //结果（右）
    QLabel* lblCargo = new QLabel(this);
    lblCargo->setText(cargo);
    lblCargo->setAlignment(Qt::AlignCenter);

    QLabel* lblRstTitle = new QLabel(this);
    lblRstTitle->setAlignment(Qt::AlignCenter);
    lblRstTitle->setText(QStringLiteral("归入标签"));

    mpResultLabel = new QLabel(this);
    mpResultLabel->setText(mTagged);
    mpResultLabel->setFixedWidth(200);
    mpResultLabel->setWordWrap(true);
    mpResultLabel->setAlignment(Qt::AlignCenter);
    mpResultLabel->setStyleSheet(QLatin1String("color:red;"));

    QPushButton *pBtnOk = new QPushButton(QIcon(":/icon/ok.png"), mapMsg.value("btn_ok"));
    pBtnOk->setIconSize(QSize(20, 20));
    pBtnOk->setFixedWidth(90);
    connect(pBtnOk, SIGNAL(clicked()), this, SLOT(clickOk()));

    QPushButton *pBtnCancel = new QPushButton(QIcon(":/icon/cancel.png"), mapMsg.value("btn_cancel"));
    pBtnCancel->setIconSize(QSize(20, 20));
    pBtnCancel->setFixedWidth(90);
    connect(pBtnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    QWidget* pnlDlg = new QWidget(this);
    QVBoxLayout *layDlg = new QVBoxLayout(pnlDlg);
    layDlg->setContentsMargins(0, 60, 0, 60);
    layDlg->setSpacing(16);
    layDlg->addWidget(lblCargo);
    layDlg->addWidget(lblRstTitle);
    layDlg->addWidget(mpResultLabel);
    layDlg->addSpacing(8);
    layDlg->addWidget(pBtnOk, 0, Qt::AlignCenter);
    layDlg->addWidget(pBtnCancel, 0, Qt::AlignCenter);
    layDlg->addStretch();

    //layout
    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->addWidget(pnlList);
    lay->addWidget(pnlImage);
    lay->addWidget(pnlDlg);

    setFixedSize(sizeHint());
    setWindowTitle(QStringLiteral("打标签"));
    setWindowFlags(windowFlags() &~ Qt::WindowContextHelpButtonHint);

    pBtnCancel->setFocus();
    connect(mpTagList, &QListWidget::currentTextChanged, mpResultLabel, &QLabel::setText);

    //加载图片
    mShowFile = checkCargoImageFile(cargo);
    if ( !mShowFile.isEmpty() ) {
        mpLblImage->setPixmap(QPixmap(mShowFile).scaled(mpLblImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void BsTagSelectDlg::loadImage()
{
    QString openFile = QFileDialog::getOpenFileName(this,
                                                   QStringLiteral("打开图片"),
                                                   QDir::homePath(),
                                                   QStringLiteral("图片文件(*.jpg)"));
    if ( openFile.isEmpty() ) {
        return;
    }

    mShowFile = openFile;
    QImageReader imgReader(mShowFile);
    imgReader.setAutoTransform(true);  //use QImage(filename) directly cannot detect orientation correctly
    QImage img = imgReader.read();
    mpLblImage->setPixmap(QPixmap::fromImage(img).scaled(mpLblImage->size(),
                                                         Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void BsTagSelectDlg::clickOk()
{
    QString err = checkImageSize();
    if ( ! err.isEmpty() ) {
        QMessageBox::information(this, QString(), err);
        return;
    }

    if ( mpResultLabel->text().isEmpty() ) {
        QMessageBox::information(this, QString(), QStringLiteral("所属品类未指定！"));
        return;
    }

    accept();
}

QString BsTagSelectDlg::checkImageSize()
{
    if ( mShowFile.isEmpty() ) {
        return QStringLiteral("未指定图片。");
    }

    //读取图片
    QImageReader imgReader(mShowFile);
    imgReader.setAutoTransform(true);  //use QImage(filename) directly cannot detect orientation correctly
    QImage img = imgReader.read();

    //尺寸比例
    if ( img.width() > img.height() || img.height() / img.width() > 2 ) {
        return QStringLiteral("图片尺寸不规范。宽度不能大于高度，但也不能小于高度的一半。");
    }

    //约定位置保存文件
    QString checkFile = checkCargoImageFile(mCargo);
    if ( checkFile.isEmpty() ) {
        checkFile = QDir(imageDir).absoluteFilePath(QStringLiteral("%1.jpg").arg(mCargo));
    }
    if (QFileInfo(mShowFile).dir() != QFileInfo(checkFile).dir()) {
        if ( QFile(checkFile).exists() ) {
            QFile(checkFile).remove();
        }
        QFile::copy(mShowFile, checkFile);
    }

    if ( img.width() <= UPIMAGE_MAX_WIDTH && img.height() <= UPIMAGE_MAX_HEIGHT ) {
        mResultImage = checkFile;
        return QString();
    }

    //缩图
    img = img.scaled(QSize(UPIMAGE_MAX_WIDTH, UPIMAGE_MAX_HEIGHT),
                     Qt::KeepAspectRatio, Qt::SmoothTransformation);

    //同目录需要先重命名备份原文件
    if ( QFileInfo(mShowFile).dir() == QDir(imageDir) ) {
        QString bakFile = QDir(imageDir).absoluteFilePath(QStringLiteral("%1_bak.jpg").arg(mCargo));
        bool renameResult = QFile(mShowFile).rename(bakFile);
        if ( false == renameResult  ) {
            return QStringLiteral("读取图片错误！");
        }
    }

    //保存缩小的图片为正式使用名称
    img.save(checkFile);
    mResultImage = checkFile;
    return QString();
}

}
