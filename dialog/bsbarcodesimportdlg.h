#ifndef BSBARCODESIMPORTDLG_H
#define BSBARCODESIMPORTDLG_H

#include <QDialog>
#include <QtWidgets>

namespace BailiSoft {

class BsBarcodesImportDlg : public QDialog
{
    Q_OBJECT
public:
    BsBarcodesImportDlg(QWidget *parent, const QString &fileData);
    ~BsBarcodesImportDlg(){}
    int textMaxColCount();

    QTextEdit*  mpText;
    QLineEdit*  mpBarCol;
    QLineEdit*  mpQtyCol;

private slots:
    void fixupInteger();
};

}

#endif // BSBARCODESIMPORTDLG_H
