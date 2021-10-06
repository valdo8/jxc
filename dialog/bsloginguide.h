#ifndef BSLOGINGUIDE_H
#define BSLOGINGUIDE_H

#include <QtWidgets>
#include <QSqlDatabase>
#include <QUdpSocket>

namespace BailiSoft {

class BsLoginGuide : public QDialog
{
    Q_OBJECT
public:
    explicit BsLoginGuide(QWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());
    void setVisible(bool visible);
    QString getCurrentBook();

protected:
    void resizeEvent(QResizeEvent *e);

private slots:
    void bookPicked(int);
    void bookDoubleClicked(QListWidgetItem *item);
    void doPrev();
    void doNext();
    void doCreateBook();
    void doOpenBook();
    void doDelBook();
    void importR15Book();
    void importR16Book();
    void doHelp();

private:
    QString sqlInitBook(const QString &bookName, QSqlDatabase &sdb);
    QString createNewBook(const QString &bookName, const QString &bookFile);
    QString calclatePassHash(const QString &pwd, const QString &salt);
    void reloadBookList();
    QString loadUserInfo();
    void upgradeSqlCheck();

    QWidget         *mpImageSide;
    QListWidget     *mpBooks;
    QComboBox       *mpUsers;
    QLineEdit       *mpPassword;
    QPushButton     *mpPrev;
    QPushButton     *mpNext;
    QPushButton     *mpCancel;
    QStackedLayout  *mpStack;
    QHBoxLayout     *mpLayDlgBtns;
    QAction         *mpAcDelBook;

    QLabel          *mpVerName;
};

}

#endif // BSLOGINGUIDE_H
