#ifndef BSCREATECOLORTYPEDLG_H
#define BSCREATECOLORTYPEDLG_H

#include <QtWidgets>

class BsFlowLayout;

namespace BailiSoft {

class BsCreateColorTypeDlg : public QDialog
{
    Q_OBJECT
public:
    BsCreateColorTypeDlg(const QString &cargo, QWidget *parent = nullptr);

    QString getColorTypeName() const { return mpEdtName->text(); }
    QStringList getPickeds() const { return mPickeds; }

private:
    void loadReady();
    void onClickNew();
    void onClickAccept();


    QLineEdit       *mpEdtName;

    QGroupBox       *mpGrpAll;
    BsFlowLayout    *mpLayAll;

    QLineEdit       *mpEdtNew;
    QToolButton     *mpBtnNew;

    QPushButton     *mpBtnOK;
    QPushButton     *mpBtnCancel;

    QString     mCargo;
    QStringList mPickeds;

    QMap<QString, QString>  mapColors;
};

}

#endif // BSCREATECOLORTYPEDLG_H

