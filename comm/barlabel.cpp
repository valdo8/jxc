#include "barlabel.h"

#include <QPainter>
#include <QDebug>

namespace BailiSoft {

BarLabel::BarLabel(const QString &text, const int width, const int height,
                   const bool textVisible, QWidget *parent) :
    QLabel(parent),
    mText(text),
    mWidth(width),
    mHeight(height),
    mTextVisible(textVisible)
{
    QPalette pal(palette());
    pal.setBrush(QPalette::Window, QColor("#fff"));
    setAutoFillBackground(true);
    setPalette(pal);

    setAlignment(Qt::AlignCenter);

    updateShow();
}

void BarLabel::setCode(const QString &text)
{
    mText = text;
    updateShow();
}

void BarLabel::setSize(const int width, const int height)
{
    mWidth = width;
    mHeight = height;
    updateShow();
}

void BarLabel::setTextVisible(const bool visible)
{
    mTextVisible = visible;
    updateShow();
}

QPixmap BarLabel::generatePixmap(const QString &text, const int width, const int height, const bool textVisible)
{
    //画布
    QPixmap pixmap(width, height);
    QPainter painter(&pixmap);
    painter.fillRect(0, 0, width, height, Qt::white);

    //变量
    Code128::BarCode barcode = Code128::encode(text);
    int codeLength = 0;
    for (int i = 0; i < barcode.length(); i++)
    {
        codeLength += barcode[i];
    }

    int txtHeight = painter.fontMetrics().height();
    int barHeight = height - txtHeight;

    int lineWidth = width / codeLength;
    if ( lineWidth < 1 )
    {
        lineWidth = 1;
    }

    //绘制
    int left = 1;
    for (int i = 0; i < barcode.length(); i++)
    {
        int w = barcode[i] * lineWidth;

        if ( i % 2 == 0 )
        {
            painter.fillRect(left, 0, w, barHeight, Qt::black);
        }

        left += w;
    }

    //文字
    if ( textVisible )
    {
        const QRect tr = QRect(0, barHeight , left, txtHeight);  //barcode actual width
        painter.drawText(tr, Qt::AlignCenter, text);
    }

    //超出提示
    if ( left > width ) {
        QString tip = QStringLiteral("宽度不够");
        int tw = painter.fontMetrics().horizontalAdvance(tip);
        QRect tr = QRect(width - tw, (barHeight - txtHeight) / 2, tw, txtHeight);
        painter.fillRect(tr, Qt::white);
        painter.drawText(tr, Qt::AlignRight | Qt::AlignVCenter, tip);
    }

    //返回
    return pixmap;
}

void BarLabel::updateShow()
{
    setPixmap(generatePixmap(mText, mWidth, mHeight, true));
    setFixedSize(mWidth, mHeight);
}

}
