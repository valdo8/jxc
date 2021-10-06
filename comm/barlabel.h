#ifndef BARLABEL_H
#define BARLABEL_H

#include <QLabel>

#include "third/code128/code128.h"

namespace BailiSoft {

class BarLabel : public QLabel
{
    Q_OBJECT
public:
    explicit BarLabel(const QString &text, const int width, const int height,
                      const bool textVisible, QWidget *parent);

    void setCode(const QString &text);
    void setSize(const int width, const int height);
    void setTextVisible(const bool visible);

    static QPixmap generatePixmap(const QString &text, const int width, const int height, const bool textVisible);

private:
    void updateShow();

    QString             mText;
    int                 mWidth;
    int                 mHeight;
    bool                mTextVisible;
};

}

#endif // BARLABEL_H
