#ifndef BSBATCHRECHECK_H
#define BSBATCHRECHECK_H

#include <QtWidgets>

namespace BailiSoft {

class BsBatchReCheck : public QDialog
{
    Q_OBJECT
public:
    explicit BsBatchReCheck(QWidget *parent);

    QComboBox*      mpOptTable;

    QRadioButton*   mpOptCheckAll;
    QRadioButton*   mpOptUnCheckAll;

    QPushButton*    mpBtnExec;

protected:
    void keyPressEvent(QKeyEvent *e);

private:
    void checkReady();
    void doExec();
};

}

#endif // BSBATCHRECHECK_H
