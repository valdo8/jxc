#ifndef BSDIALOG_H
#define BSDIALOG_H

#include <QtWidgets>

namespace BailiSoft {

class BsField;
class BsFldBox;
class BsListModel;

class BsDialog : public QDialog
{
public:
    explicit BsDialog(QWidget *parent, const bool addMode, const QString &table, const QList<BsField*> &fields);
    ~BsDialog();
    BsFldBox* getEditorByField(const QString &fieldName);
    bool eventFilter(QObject *watched, QEvent *event);

    bool                mAddMode;
    QString             mTable;
    QList<BsField*>     mFields;
    QList<BsFldBox*>    mEditors;

protected:
    void showEvent(QShowEvent *event);

private:
    void openHelpPage();
    void updateButtonState();
    QString getShortName(const QString &longName);
    BsField *getFieldByName(const QString &name);

    QLabel*             mpLblTitle;
    QPushButton*        mpBtnOk;
    QPushButton*        mpBtnHelp;

    QList<BsListModel*>     mAttrModels;
};

}

#endif // BSDIALOG_H
