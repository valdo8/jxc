#ifndef BSPRICEBATCH_H
#define BSPRICEBATCH_H

#include <QtWidgets>

namespace BailiSoft {

// BsPriceBatchDlg ==========================================================================
class BsPriceBatchDlg : public QDialog
{
    Q_OBJECT
public:
    BsPriceBatchDlg(QWidget *parent, const QString &sheetPrice, const QString &trader, const double regDis);
    QString getPriceType();

    QRadioButton*       mpOptPolicy;
    QRadioButton*       mpOptRegDis;
    QRadioButton*       mpOptValDis;
    QRadioButton*       mpOptSetPrice;
    QRadioButton*       mpOptRetPrice;
    QRadioButton*       mpOptLotPrice;
    QRadioButton*       mpOptBuyPrice;

    QLineEdit*          mpEdtDiscount;

    QLabel*             mpLblRemark;

private slots:
    void optionClicked();

private:

    QString     mSheetPrice;
    QString     mTrader;
    double      mRegDis;

};

}

#endif // BSPRICEBATCH_H
