#include "bsabout.h"
#include "main/bailicode.h"
#include <QtWidgets>

BsAbout::BsAbout(QWidget *parent) : QLabel(parent)
{
    QLabel *pImg = new QLabel(this);
    pImg->setPixmap(QPixmap(":/image/about.png"));
    pImg->setAlignment(Qt::AlignCenter);
    pImg->setFixedSize(640, 360);
    pImg->setStyleSheet("QLabel {background:white; }");

    mpBuildNum = new QLabel(this);
    mpBuildNum->setText(QStringLiteral("%1.%2.%3")
                        .arg(BailiSoft::lxapp_version_major)
                        .arg(BailiSoft::lxapp_version_minor)
                        .arg(BailiSoft::lxapp_version_patch));
    mpBuildNum->setStyleSheet("QLabel {color:white; }");

    setFixedSize(640, 360);
}

void BsAbout::mouseReleaseEvent(QMouseEvent *e)
{
    QLabel::mouseReleaseEvent(e);
    close();
}

void BsAbout::resizeEvent(QResizeEvent *e)
{
    QLabel::resizeEvent(e);
    mpBuildNum->setGeometry(550, 95, 40, 20);
}

