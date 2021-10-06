#ifndef BSMDIAREA_H
#define BSMDIAREA_H

#include <QtWidgets>

namespace BailiSoft {

class BsMain;
class BsMainQuickPanel;

enum BsGuideBtnShap {bsgbsSheet, bsgbsNeat, bsgbsCash, bsgbsRest, bsgbsStock};

// BsMdiArea ==================================================================================
class BsMdiArea : public QMdiArea
{
    Q_OBJECT
public:
    explicit BsMdiArea(QWidget *parent, const QColor &clr);
    ~BsMdiArea(){}
    void shrinkGuide();
    void expandGuide();
    BsMainQuickPanel*   mpPnlGuide;
signals:
    void backgroundChanged(const QColor &clr);
protected:
    void paintEvent(QPaintEvent *e);
private:
    BsMain*    mppMain;
    QColor          mColor;
};

class BsMainQuickButton;

// BsMainQuickPanel ============================================================================
class BsMainQuickPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BsMainQuickPanel(QWidget *parent);
    ~BsMainQuickPanel(){}
    void showAllButtons();
    void hideAllButtons();
    void updateButtonRights();

signals:
    void clickButton(const QString &winBaseName, const bool statt);
    void mouseEntered();
    void mouseLeaved();

protected:
    void resizeEvent(QResizeEvent *e);
    void paintEvent(QPaintEvent *);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *e);

private:
    BsMainQuickButton*  mpBtnCgd;
    BsMainQuickButton*  mpBtnCgj;
    BsMainQuickButton*  mpBtnCgt;
    BsMainQuickButton*  mpBtnSyd;

    BsMainQuickButton*  mpBtnPfd;
    BsMainQuickButton*  mpBtnPff;
    BsMainQuickButton*  mpBtnPft;
    BsMainQuickButton*  mpBtnLsd;

    BsMainQuickButton*  mpBtnSzd;
    BsMainQuickButton*  mpBtnDbd;

    BsMainQuickButton*  mpBtnJcg;
    BsMainQuickButton*  mpBtnJpf;
    BsMainQuickButton*  mpBtnJxs;

    BsMainQuickButton*  mpBtnCgRest;
    BsMainQuickButton*  mpBtnCgCash;

    BsMainQuickButton*  mpBtnPfRest;
    BsMainQuickButton*  mpBtnPfCash;
    BsMainQuickButton*  mpBtnXsCash;

    BsMainQuickButton*  mpBtnStock;
    BsMainQuickButton*  mpBtnViall;
};


// BsMainQuickButton ============================================================================
class BsMainQuickButton : public QWidget
{
    Q_OBJECT
public:
    explicit BsMainQuickButton(QWidget *parent, const BsGuideBtnShap buttonShap, const QString &caption,
                               const QString &winBaseName);
    ~BsMainQuickButton(){}

    bool    mAllow;
    bool    mStatAllow;

signals:
    void clickButton(const QString &winBaseName, const bool statt);

protected:
    void enterEvent(QEvent *e);
    void leaveEvent(QEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    QWidget*        mppParent;

    BsGuideBtnShap  mButtonShap;
    QString         mCaption;
    QString         mWinBaseName;

    QColor          mColor;
    QFont           mFont;
    bool            mEntered;
    bool            mAtRight;
};

}

#endif // BSMDIAREA_H
