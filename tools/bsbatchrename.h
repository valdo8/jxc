#ifndef BSBATCHEDIT_H
#define BSBATCHEDIT_H

#include <QtWidgets>

namespace BailiSoft {

class BsField;
class BsFldEditor;

class BsBatchRename : public QDialog
{
    Q_OBJECT
public:
    explicit BsBatchRename(QWidget *parent);
    ~BsBatchRename();

    BsField*        mpCargoField;

    QComboBox*      mpRegTable;
    QLineEdit*      mpEdtOld;
    QLineEdit*      mpEdtNew;
    BsFldEditor*    mpConCargo;
    QPushButton*    mpBtnExec;

    QWidget*        mpLblConCargo;

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:
    void checkReady();
    void doExec();
};

}

#endif // BSBATCHEDIT_H
