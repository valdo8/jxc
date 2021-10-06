#ifndef BSTAGSELECTDLG_H
#define BSTAGSELECTDLG_H

#include <QDialog>
#include <QtWidgets>

namespace BailiSoft {

class BsTagSelectDlg : public QDialog
{
    Q_OBJECT
public:
    BsTagSelectDlg(QWidget *parent, const QString &cargo, const QStringList &tags,
                   const QString tagged = QString());
    ~BsTagSelectDlg() {}

    QLabel*         mpResultLabel;
    QString         mResultImage;

private slots:
    void loadImage();
    void clickOk();

private:
    QString checkImageSize();

    QListWidget*    mpTagList;
    QLabel*         mpLblImage;

    QString         mCargo;
    QStringList     mTags;
    QString         mTagged;

    QString         mShowFile;
};

}

#endif // BSTAGSELECTDLG_H
