#include "qrlabel.h"
#include "third/codeqr/codeqr.hpp"

using namespace qrcodegen;

namespace BailiSoft {

QrLabel::QrLabel(const QString &code, const int blockSize, QWidget *parent)
    : QLabel(parent), mCode(code), mSize(blockSize)
{
    QPalette pal(palette());
    pal.setBrush(QPalette::Window, QColor("#fff"));
    setAutoFillBackground(true);
    setPalette(pal);
    setAlignment(Qt::AlignCenter);

    updateShow();
}

void QrLabel::setCode(const QString &code)
{
    mCode = code;
    updateShow();
}

void QrLabel::setSize(const int blockSize)
{
    mSize = blockSize;
    updateShow();
}

QPixmap QrLabel::generatePixmap(const QString &code, const int blockSize)
{
    QrCode qrCode = QrCode::encodeText(code.toUtf8().constData(), QrCode::Ecc::MEDIUM);
    int qrSize = qrCode.getSize();
    int rate = blockSize / qrSize;

    QPixmap pixmap(rate * qrSize, rate * qrSize);
    QPainter painter(&pixmap);

    for (int y = 0; y < qrSize; y++)
    {
        for (int x = 0; x < qrSize; x++)
        {
            QColor color = (qrCode.getModule(x, y) == 0) ? Qt::white : Qt::black;
            painter.fillRect(rate * x, rate * y, rate, rate, color);
        }
    }

    return pixmap;
}

void QrLabel::updateShow()
{
    setPixmap(generatePixmap(mCode, mSize));
    setFixedSize(mSize, mSize);
}

}
