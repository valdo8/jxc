#ifndef QRLABEL_H
#define QRLABEL_H

#include <QtWidgets>

namespace BailiSoft {

class QrLabel : public QLabel
{
    Q_OBJECT
public:
    explicit QrLabel(const QString &code, const int blockSize, QWidget *parent);
    void setCode(const QString &code);
    void setSize(const int blockSize);
    static QPixmap generatePixmap(const QString &code, const int blockSize);

private:
    void updateShow();
    QString     mCode;
    int         mSize;
};

}

#endif // QRLABEL_H
