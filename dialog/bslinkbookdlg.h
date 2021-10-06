#ifndef BSLINKBOOKDLG_H
#define BSLINKBOOKDLG_H

#include <QtWidgets>

namespace BailiSoft {

class BsLinkBookDlg : public QDialog
{
    Q_OBJECT
public:
    BsLinkBookDlg(QWidget *parent);
    ~BsLinkBookDlg(){}

    QLineEdit*      mpBacker;
    QLineEdit*      mpFront;
    QLineEdit*      mpPassword;

private slots:
    void updateOkButtonState();

private:
    QPushButton*    mpBtnOk;
};

}

#endif // BSLINKBOOKDLG_H
