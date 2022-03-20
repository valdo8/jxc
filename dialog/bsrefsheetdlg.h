#ifndef BSREFSHEETDLG_H
#define BSREFSHEETDLG_H

#include <QDialog>
#include <QtWidgets>

namespace BailiSoft {

class BsRefSheetDlg : public QDialog
{
    Q_OBJECT
public:
    BsRefSheetDlg(const qint64 amount, QStringList &subjects, QWidget *parent);
    ~BsRefSheetDlg() {}

    QList<qint64>   mAssigns;

protected:
    void showEvent(QShowEvent *e);

private:
    void edtTextChanged(const QString &);
    void edtReturnPressed();
    void acceptOk();

    QDoubleValidator*   mpValidator;
    QGroupBox*          mpForm;
    QLabel*             mpLblRemaining;
    QPushButton*        mpBtnOk;

    qint64              mAmount;
    QStringList         mSubjects;

};

}

#endif // BSREFSHEETDLG_H
