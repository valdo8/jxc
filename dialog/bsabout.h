#ifndef BSABOUT_H
#define BSABOUT_H

#include <QLabel>
#include <QWidget>

class BsAbout : public QLabel
{
    Q_OBJECT
public:
    BsAbout(QWidget *parent);

private:
    QLabel *mpBuildNum;

protected:
    void mouseReleaseEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *e);

};

#endif // BSABOUT_H
