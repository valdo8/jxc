#ifndef BSTOOLSTOCKRESET_H
#define BSTOOLSTOCKRESET_H

#include <QtWidgets>

namespace BailiSoft {

class BsField;
class BsFldEditor;

class BsToolStockReset : public QDialog
{
    Q_OBJECT
public:
    explicit BsToolStockReset(QWidget *parent);

    BsField*        mpFldShop;
    BsField*        mpFldDate;

    BsFldEditor*    mpEdtShop;
    BsFldEditor*    mpEdtDate;

    QPushButton*    mpBtnExec;

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:
    void checkReady();
    void doExec();
};

}

#endif // BSTOOLSTOCKRESET_H
