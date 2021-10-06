#include "bsmdiarea.h"
#include "main/bailidata.h"
#include "main/bailicode.h"
#include "main/bsmain.h"

namespace BailiSoft {

// BsMdiArea ==================================================================================
BsMdiArea::BsMdiArea(QWidget *parent, const QColor &clr) : QMdiArea(parent), mColor(clr)
{
    mppMain = qobject_cast<BsMain*>(parent);
    Q_ASSERT(mppMain);

    mpPnlGuide = new BsMainQuickPanel(this);
    mpPnlGuide->hide();
    connect(mpPnlGuide, &BsMainQuickPanel::clickButton, mppMain, &BsMain::clickQuickButton);
    connect(mpPnlGuide, &BsMainQuickPanel::mouseEntered, this, &BsMdiArea::expandGuide);
    connect(mpPnlGuide, &BsMainQuickPanel::mouseLeaved, this, &BsMdiArea::shrinkGuide);
}

void BsMdiArea::shrinkGuide()
{
    mpPnlGuide->hideAllButtons();
    mpPnlGuide->setGeometry(0, 0, 3, height());
    mpPnlGuide->show();
}

void BsMdiArea::expandGuide()
{
    mpPnlGuide->showAllButtons();
    mpPnlGuide->setGeometry(0, 0, 600, height());
    mpPnlGuide->show();
}

void BsMdiArea::paintEvent(QPaintEvent *e)
{
    QMdiArea::paintEvent(e);

    QPainter painter(viewport());

    QString colorText = mapOption.value("app_company_pcolor");
    QString r = colorText.left(2);
    QString g = colorText.mid(2, 2);
    QString b = colorText.right(2);
    bool ok;
    QColor comColor = QColor(r.toInt(&ok, 16), g.toInt(&ok, 16), b.toInt(&ok, 16));
    emit backgroundChanged(comColor);

    QByteArray logoData = mapOption.value("app_company_plogo").toLatin1();
    if ( logoData.length() > 100 ) {
        QImage img = QImage::fromData(QByteArray::fromBase64(logoData));
        painter.fillRect(rect(), comColor);
        bool center = mapOption.value("app_company_plway").contains(QStringLiteral("中"));
        if ( center ) {
            painter.drawImage((width() - 300) / 2, (height() - 300) / 2, img, 0, 0, 300, 300);
        } else {
            for ( int i = 0; i < (width() / 300) + 2; ++i ) {
                for ( int j = 0; j < (height() / 300) + 2; ++j ) {
                    painter.drawImage(300 * i, 300 * j, img, 0, 0, 300, 300);
                }
            }
        }
    }

    mpPnlGuide->update();
}

// BsMainQuickPanel ============================================================================
BsMainQuickPanel::BsMainQuickPanel(QWidget *parent) : QWidget(parent)
{
    //  bsgbsSheet, bsgbsNeat, bsgbsCash, bsgbsRest, bsgbsStock
    mpBtnCgd = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("采购订"), QStringLiteral("cgd"));
    mpBtnCgj = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("采购进"), QStringLiteral("cgj"));
    mpBtnCgt = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("采购退"), QStringLiteral("cgt"));
    mpBtnDbd = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("调拨"), QStringLiteral("dbd"));

    mpBtnPfd = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("批发订"), QStringLiteral("pfd"));
    mpBtnPff = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("批发发"), QStringLiteral("pff"));
    mpBtnPft = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("批发退"), QStringLiteral("pft"));
    mpBtnLsd = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("零售"), QStringLiteral("lsd"));

    mpBtnSzd = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("收支"), QStringLiteral("szd"));
    mpBtnSyd = new BsMainQuickButton(this, bsgbsSheet, QStringLiteral("损益"), QStringLiteral("syd"));

    mpBtnJcg = new BsMainQuickButton(this, bsgbsNeat, QStringLiteral("净采购"), QStringLiteral("jcg"));
    mpBtnJpf = new BsMainQuickButton(this, bsgbsNeat, QStringLiteral("净批发"), QStringLiteral("jpf"));
    mpBtnJxs = new BsMainQuickButton(this, bsgbsNeat, QStringLiteral("净销售"), QStringLiteral("jxs"));

    mpBtnCgRest = new BsMainQuickButton(this, bsgbsRest, QStringLiteral("采购欠货"), QStringLiteral("cgrest"));
    mpBtnCgCash = new BsMainQuickButton(this, bsgbsCash, QStringLiteral("采购欠款"), QStringLiteral("cgcash"));

    mpBtnPfRest = new BsMainQuickButton(this, bsgbsRest, QStringLiteral("批发欠货"), QStringLiteral("pfrest"));
    mpBtnPfCash = new BsMainQuickButton(this, bsgbsCash, QStringLiteral("批发欠款"), QStringLiteral("pfcash"));
    mpBtnXsCash = new BsMainQuickButton(this, bsgbsCash, QStringLiteral("销售欠款"), QStringLiteral("xscash"));

    mpBtnStock = new BsMainQuickButton(this, bsgbsStock, QStringLiteral("库存"), QStringLiteral("stock"));
    mpBtnViall = new BsMainQuickButton(this, bsgbsStock, QStringLiteral("进销存"), QStringLiteral("viall"));

    hideAllButtons();

    connect(mpBtnCgd, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnCgj, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnCgt, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnDbd, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));

    connect(mpBtnPfd, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnPff, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnPft, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnLsd, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));

    connect(mpBtnSzd, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnSyd, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));

    connect(mpBtnJcg, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnJpf, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnJxs, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));

    connect(mpBtnCgRest, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnCgCash, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));

    connect(mpBtnPfRest, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnPfCash, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnXsCash, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));

    connect(mpBtnStock, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
    connect(mpBtnViall, SIGNAL(clickButton(QString,bool)), this, SIGNAL(clickButton(QString,bool)));
}

void BsMainQuickPanel::showAllButtons()
{
    mpBtnCgd->show();
    mpBtnCgj->show();
    mpBtnCgt->show();
    mpBtnDbd->show();
    mpBtnPfd->show();
    mpBtnPff->show();
    mpBtnPft->show();
    mpBtnLsd->show();
    mpBtnSzd->show();
    mpBtnSyd->show();
    mpBtnJcg->show();
    mpBtnJpf->show();
    mpBtnJxs->show();
    mpBtnCgRest->show();
    mpBtnCgCash->show();
    mpBtnPfRest->show();
    mpBtnPfCash->show();
    mpBtnXsCash->show();
    mpBtnStock->show();
    mpBtnViall->show();
}

void BsMainQuickPanel::hideAllButtons()
{
    mpBtnCgd->hide();
    mpBtnCgj->hide();
    mpBtnCgt->hide();
    mpBtnDbd->hide();
    mpBtnPfd->hide();
    mpBtnPff->hide();
    mpBtnPft->hide();
    mpBtnLsd->hide();
    mpBtnSzd->hide();
    mpBtnSyd->hide();
    mpBtnJcg->hide();
    mpBtnJpf->hide();
    mpBtnJxs->hide();
    mpBtnCgRest->hide();
    mpBtnCgCash->hide();
    mpBtnPfRest->hide();
    mpBtnPfCash->hide();
    mpBtnXsCash->hide();
    mpBtnStock->hide();
    mpBtnViall->hide();
}

void BsMainQuickPanel::updateButtonRights()
{
    mpBtnCgd->mAllow = canDo("cgd");
    mpBtnCgj->mAllow = canDo("cgj");
    mpBtnCgt->mAllow = canDo("cgt");
    mpBtnPfd->mAllow = canDo("pfd");
    mpBtnPff->mAllow = canDo("pff");
    mpBtnPft->mAllow = canDo("pft");
    mpBtnLsd->mAllow = canDo("lsd");
    mpBtnDbd->mAllow = canDo("dbd");
    mpBtnSyd->mAllow = canDo("syd");
    mpBtnSzd->mAllow = loginAsBoss;

    mpBtnCgd->mStatAllow = canDo("vicgd");
    mpBtnCgj->mStatAllow = canDo("vicgj");
    mpBtnCgt->mStatAllow = canDo("vicgt");
    mpBtnPfd->mStatAllow = canDo("vipfd");
    mpBtnPff->mStatAllow = canDo("vipff");
    mpBtnPft->mStatAllow = canDo("vipft");
    mpBtnLsd->mStatAllow = canDo("vilsd");
    mpBtnDbd->mStatAllow = canDo("vidbd");
    mpBtnSyd->mStatAllow = canDo("visyd");
    mpBtnSzd->mStatAllow = loginAsBoss;

    mpBtnJcg->mAllow = canDo("vicg");
    mpBtnJpf->mAllow = canDo("vipf");
    mpBtnJxs->mAllow = canDo("vixs");

    mpBtnCgRest->mAllow = canDo("vicgrest");
    mpBtnCgCash->mAllow = canDo("vicgcash");

    mpBtnPfRest->mAllow = canDo("vipfrest");
    mpBtnPfCash->mAllow = canDo("vipfcash");
    mpBtnXsCash->mAllow = canDo("vixscash");

    mpBtnStock->mAllow = canDo("vistock");
    mpBtnViall->mAllow = canDo("viall");
}

void BsMainQuickPanel::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    if ( width() < 30 ) {
        hideAllButtons();
        return;
    }

    int totalWidth = width();
    int totalHeight = height();
    const int xPadding = totalWidth / 20;
    const int yPadding = totalHeight / 10;

    const double xUnit = (totalWidth  - 2 * xPadding) / 15.0;
    const double yUnit = (totalHeight - 2 * yPadding) / 33.0;
    int uw = int(3 * xUnit);
    int sh = int(3 * yUnit);
    int bh = int(5 * yUnit);

    mpBtnCgd->setGeometry(xPadding +  int(0 * xUnit),     yPadding + int(4 * yUnit),     uw, bh);
    mpBtnCgj->setGeometry(xPadding +  int(4 * xUnit),     yPadding + int(4 * yUnit),     uw, bh);
    mpBtnCgt->setGeometry(xPadding +  int(8 * xUnit),     yPadding + int(4 * yUnit),     uw, bh);
    mpBtnDbd->setGeometry(xPadding + int(12 * xUnit),     yPadding + int(4 * yUnit),     uw, bh);

    mpBtnPfd->setGeometry(xPadding +  int(0 * xUnit),    yPadding + int(24 * yUnit),     uw, bh);
    mpBtnPff->setGeometry(xPadding +  int(4 * xUnit),    yPadding + int(24 * yUnit),     uw, bh);
    mpBtnPft->setGeometry(xPadding +  int(8 * xUnit),    yPadding + int(24 * yUnit),     uw, bh);
    mpBtnLsd->setGeometry(xPadding + int(12 * xUnit),    yPadding + int(24 * yUnit),     uw, bh);

    mpBtnSzd->setGeometry(xPadding +  int(0 * xUnit),    yPadding + int(14 * yUnit),     uw, bh);
    mpBtnSyd->setGeometry(xPadding + int(12 * xUnit),    yPadding + int(14 * yUnit),     uw, bh);

    mpBtnJcg->setGeometry(xPadding +  int(6 * xUnit),    yPadding + int(10 * yUnit),     uw, sh);
    mpBtnJpf->setGeometry(xPadding +  int(6 * xUnit),    yPadding + int(20 * yUnit),     uw, sh);
    mpBtnJxs->setGeometry(xPadding + int(10 * xUnit),    yPadding + int(20 * yUnit),     uw, sh);

    mpBtnCgRest->setGeometry(xPadding +  int(2 * xUnit), yPadding +  int(0 * yUnit),     uw, sh);
    mpBtnCgCash->setGeometry(xPadding +  int(6 * xUnit), yPadding +  int(0 * yUnit),     uw, sh);

    mpBtnPfRest->setGeometry(xPadding +  int(2 * xUnit), yPadding + int(30 * yUnit),     uw, sh);
    mpBtnPfCash->setGeometry(xPadding +  int(6 * xUnit), yPadding + int(30 * yUnit),     uw, sh);
    mpBtnXsCash->setGeometry(xPadding + int(10 * xUnit), yPadding + int(30 * yUnit),     uw, sh);

    mpBtnStock->setGeometry(xPadding  +  int(4 * xUnit), yPadding + int(14 * yUnit),     uw, bh);
    mpBtnViall->setGeometry(xPadding  +  int(8 * xUnit), yPadding + int(14 * yUnit),     uw, bh);
}

void BsMainQuickPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height()));
    linearGrad.setColorAt(0, QColor(180, 180, 180));
    linearGrad.setColorAt(1, QColor(140, 140, 140));

    //布纹背景
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(linearGrad));
    p.drawRect(0, 0, width(), height());

    //公司名称
    QFont ft(font());
    ft.setBold(true);
    ft.setPointSize( 3 * ft.pointSize() / 2);
    p.setFont(ft);
    p.setPen(QPen(QColor(100, 100, 100)));
    p.drawText(0, 30, width() - 30, height() - 100, Qt::AlignRight | Qt::AlignTop, mapOption.value("app_company_name"));
}

void BsMainQuickPanel::enterEvent(QEvent *e)
{
    QWidget::enterEvent(e);
    emit mouseEntered();
}

void BsMainQuickPanel::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    hideAllButtons();
    emit mouseLeaved();
}


// BsMainQuickButton ============================================================================
BsMainQuickButton::BsMainQuickButton(QWidget *parent, const BsGuideBtnShap buttonShap, const QString &caption,
                                     const QString &winBaseName)
    : QWidget(parent), mAllow(false), mStatAllow(false), mppParent(parent), mButtonShap(buttonShap), mCaption(caption),
      mWinBaseName(winBaseName), mEntered(false), mAtRight(false)
{
    mColor = QColor(125, 125, 125);

    //bsgbsSheet, bsgbsNeat, bsgbsCash, bsgbsRest, bsgbsStock
    switch ( buttonShap ) {
    case bsgbsSheet:
        mColor = QColor(15, 100, 0);
        setMouseTracking(true);
        break;
    case bsgbsNeat:
        mColor = QColor(120, 120, 0);
        break;
    case bsgbsCash:
        mColor = QColor(0, 100, 100);
        break;
    case bsgbsRest:
        mColor = QColor(0, 90, 150);
        break;
    case bsgbsStock:
        mColor = QColor(200, 0, 0);
        break;
    }

    mFont.setBold(true);
    mFont.setPointSize(3 * font().pointSize() / 2);
}

void BsMainQuickButton::enterEvent(QEvent *e)
{
    QWidget::enterEvent(e);
    mEntered = true;
    update();
}

void BsMainQuickButton::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    mEntered = false;
    update();
}

void BsMainQuickButton::mouseMoveEvent(QMouseEvent *e)
{
    QWidget::mouseMoveEvent(e);
    bool drawStat = (e->pos().x() > width() / 2);
    if ( mAtRight != drawStat ) {
        mAtRight = drawStat;
        update();
    }
}

void BsMainQuickButton::mouseReleaseEvent(QMouseEvent *e)
{
    QWidget::mouseReleaseEvent(e);

    //以下if嵌套不能合并
    if ( mButtonShap == bsgbsSheet ) {
        if ( mAtRight ) {
            if ( mStatAllow ) {
                mppParent->hide();
                emit clickButton(mWinBaseName, true);
            }
        }
        else {
            if ( mAllow ) {
                mppParent->hide();
                emit clickButton(mWinBaseName, false);
            }
        }
    }
    else {
        if ( mAllow ) {
            mppParent->hide();
            emit clickButton(mWinBaseName, true);
        }
    }
}

void BsMainQuickButton::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);  //加了明显细腻

    const int penw = 2;

    //鼠标进入
    if ( mEntered ) {

        //无权先处理
        if ( (mButtonShap == bsgbsSheet && !mAllow && !mStatAllow) ||  //sheet录单统计皆无权。如有一权后面文字禁止
             (mButtonShap != bsgbsSheet && !mAllow) ) {

            //drawRoundedRect
            p.setPen(Qt::NoPen);
            p.setBrush(QBrush(QColor(125, 125, 125)));
            p.drawRoundedRect(penw / 2, penw / 2, width() - penw, height() - penw, 10, 10);

            //drawText
            QFont ft = mFont;
            if ( mButtonShap == bsgbsStock ) ft.setPointSize(3 * ft.pointSize() / 2);
            p.setFont(ft);
            p.setPen(QPen(QColor(200, 200, 200)));
            p.drawText(rect(), Qt::AlignCenter, QStringLiteral("无权限"));

            return;
        }

        //drawRoundedRect
        QPen pen(QBrush(Qt::white), penw);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(QBrush(mColor.lighter(150)));
        p.drawRoundedRect(penw / 2, penw / 2, width() - penw, height() - penw, 15, 15);

        //drawText
        if ( mButtonShap == bsgbsSheet ) {

            //共同文字
            QFont ft(font());
            ft.setBold(true);
            ft.setPointSize(4 * font().pointSize() / 3);
            p.setFont(ft);
            p.setPen(QPen(mColor.darker(180)));
            p.drawText(0, penw + 4, width(), height() / 2, Qt::AlignCenter, mCaption);

            //左文字（录单）
            QString leftTxt = ( mAtRight || mAllow ) ? QStringLiteral("录单") : QStringLiteral("无权限");
            p.setPen(QPen(( !mAtRight && mAllow ) ? QColor(Qt::white) : mColor.darker(180)));
            p.setFont(mFont);
            p.drawText(penw,
                       height() / 3,
                       width() / 2,
                       2 * height() / 3 - 4 * penw,
                       Qt::AlignCenter, leftTxt);

            //右文字（统计）
            QString rightTxt = ( !mAtRight || mStatAllow ) ? QStringLiteral("统计") : QStringLiteral("无权限");
            p.setPen(QPen(( mAtRight && mStatAllow ) ? QColor(Qt::white) : mColor.darker(180)));
            p.setFont(mFont);
            p.drawText(penw + width() / 2,
                       height() / 3,
                       width() / 2,
                       2 * height() / 3 - 4 * penw,
                       Qt::AlignCenter, rightTxt);
        }
        else {
            QFont ft = mFont;
            if ( mButtonShap == bsgbsStock ) ft.setPointSize(3 * ft.pointSize() / 2);
            p.setFont(ft);
            p.drawText(rect(), Qt::AlignCenter, mCaption);
        }
    }
    //无鼠标进入普通态
    else {
        //drawRoundedRect
        p.setPen(Qt::NoPen);
        p.setBrush(QBrush(mColor));
        p.drawRoundedRect(penw / 2, penw / 2, width() - penw, height() - penw, 15, 15);

        if ( mButtonShap == bsgbsSheet ) {
            p.setPen(QPen(mColor.darker(150)));
            p.drawText(0, 0, width(), height() / 2, Qt::AlignCenter, QStringLiteral("基础单据"));
        }

        //drawText
        QFont ft = mFont;
        if ( mButtonShap == bsgbsStock ) ft.setPointSize(3 * ft.pointSize() / 2);
        p.setFont(ft);
        p.setPen(QPen(mColor.darker(180)));
        p.drawText(rect(), Qt::AlignCenter, mCaption);
    }
}

}
