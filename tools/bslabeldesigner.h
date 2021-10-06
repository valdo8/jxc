#ifndef BSLABELDESIGNER_H
#define BSLABELDESIGNER_H

#include <QtWidgets>
#include <QtSql>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include "main/baililabel.h"

namespace BailiSoft {

class BsLabelPage;
class BsLabelWinUnit;
class BsLabelPart;
class BsLabelDefineDlg;

// BsLabelDesigner ===========================================================================
class BsLabelDesigner : public QWidget
{
    Q_OBJECT
public:
    explicit BsLabelDesigner(QWidget *parent, const QString &winName);
    void setDirty(const bool dirty) { mIsDirty = dirty; updateActions(); }
    inline int getPixelsByMMs(const int prMMs) { return int((prMMs * mScreenDpi) / 25.4); }
    inline int getMMsByPixels(const int prPixels) { return  int(25.4 * (prPixels / mScreenDpi)); }

    QLineEdit*          mpFldLabelName;
    QLineEdit*          mpFldCargoExp;
    QComboBox*          mpFldPrinterName;
    QSpinBox*           mpFldFromX;
    QSpinBox*           mpFldFromY;
    QSpinBox*           mpFldUnitCount;
    QSpinBox*           mpFldUnitWidth;
    QSpinBox*           mpFldUnitHeight;
    QSpinBox*           mpFldSpaceX;
    QSpinBox*           mpFldSpaceY;
    QLineEdit*          mpFldFlowNum;

protected:
    void resizeEvent(QResizeEvent *e);

private slots:
    void onPatternSelected(int idx);
    void onMainValueChanged(int val);
    void onMainPrinterChanged(int);
    void onMainTextChanged();

private:
    void reloadPrinterSelections();
    void reloadPatternSelections();
    void openPattern(const int labelId);
    void layoutDefineButtons();
    void updateActions();
    void resetPatternComboWithCurrent();

    void clickPatternNew();
    void clickPatternDel();
    void clickEditSave();
    void clickEditCancel();
    void clickEditTest();

    qreal           mScreenDpi;

    QComboBox*      mpComboPatterns;

    QAction*        mpAcSelect;
    QAction*        mpAcNew;
    QAction*        mpAcDel;
    QAction*        mpAcSave;
    QAction*        mpAcCancel;
    QAction*        mpAcTest;

    QWidget*        mpPnlParams = nullptr;
    QWidget*        mpPnlPrinterPad;
    QVBoxLayout*    mpLayPrinterPad;
    BsLabelPage*    mpLabelPage;
    QWidget*        mpPnlButtons;

    int             mCurrentId = 0;
    bool            mIsDirty;
};


// BsLabelPage ================================================================================
class BsLabelPage : public QWidget
{
    Q_OBJECT
public:
    explicit BsLabelPage(BsLabelDesigner *parent);
    void openPattern(const int labelId);
    QStringList getSaveSqls(const int saveId);
    void updateUnits(const int labelId, const bool recreate);

    void clickAddText();
    void clickAddField();
    void clickAddBarcode();
    void clickAddQrcode();

    void onDeleteDef();
    void onUpdateDef();
    void onDraggedDef();

    BsLabelDesigner*        mppDesigner;
    QHBoxLayout*            mpLayRow;
    QList<BsLabelDef*>      mDefines;
    QList<BsLabelWinUnit*>  mUnits;
    int                     mCurrentId = 0;
    bool                    mIsDirty;

signals:
    void mouseClicked();

protected:
    void mouseReleaseEvent(QMouseEvent *e) {
        QWidget::mouseReleaseEvent(e);
        emit mouseClicked();
    }
};


// BsLabelWinUnit ================================================================================
class BsLabelWinUnit : public QWidget
{
    Q_OBJECT
public:
    explicit BsLabelWinUnit(BsLabelPage *parent);
    ~BsLabelWinUnit();
    void createParts();
    void layoutParts();
    void addPartByDef(BsLabelDef *def);
    void delPartByDef(BsLabelDef *def);
    void updPartByDef(BsLabelDef *def);

    BsLabelPage*            mppPage;
    QList<BsLabelPart*>     mParts;

protected:
    void resizeEvent(QResizeEvent *e);
    void enterEvent(QEvent *e);
    void leaveEvent(QEvent *e);

private:
    void createOnePart(BsLabelDef *def);

};


// BsLabelDefineDlg ==============================================================================
class BsLabelDefineDlg : public QDialog
{
    Q_OBJECT
public:
    explicit BsLabelDefineDlg(QWidget *parent, const int defType, BsLabelDef* editDef);
    QString getExp() const;

    int                         mDefType;
    BsLabelDef*                 mppDefine;

    QFormLayout*    mpLayForm;
    QComboBox*          mpEdtValue;
    QSpinBox*           mpEdtPosX;
    QSpinBox*           mpEdtPosY;
    QSpinBox*           mpEdtWidth;
    QSpinBox*           mpEdtHeight = nullptr;
    QComboBox*          mpEdtFontName = nullptr;
    QSpinBox*           mpEdtFontPoint = nullptr;
    QComboBox*          mpEdtFontAlign = nullptr;

private slots:
    void onValueIndexChanged(int index);
    void onValueTextEdited(const QString &text);

private:
    QPushButton*            mpBtnOk;
};


// BsLabelPart ==================================================================================
class BsLabelPart : public QLabel
{
    Q_OBJECT
public:
    explicit BsLabelPart(QWidget *parent, BsLabelDef* def);
    void setContent(const BsLabelDef *def);

    QWidget*        mppParent = nullptr;
    BsLabelDef*     mppDefine = nullptr;

signals:
    void reqDeleteDef();
    void reqUpdateDef();
    void reqDragMoveDef();

protected:
    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    void clickDelete();
    void clickUpdate();

    QMenu*          mpMenu;
    QPoint          mPtStart;
    bool            mMoved;
};

}

#endif // BSLABELDESIGNER_H
