#ifndef BSHOPLOCDLG_H
#define BSHOPLOCDLG_H

#include <QDialog>
#include <QtWidgets>

namespace BailiSoft {

class BsShopLocDlg : public QDialog
{
    Q_OBJECT
public:
    BsShopLocDlg(QString &shop, QWidget *parent);
    ~BsShopLocDlg() {}

    QLineEdit*  mpLat;
    QLineEdit*  mpLng;

private slots:
    void openMapUrl(QString url);

};

}

#endif // BSHOPLOCDLG_H
