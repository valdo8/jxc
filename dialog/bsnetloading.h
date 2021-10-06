#ifndef BSNETLOADING_H
#define BSNETLOADING_H

#include <QDialog>
#include <QColor>
#include <QWidget>

namespace BailiSoft {

class BsNetLoading : public QDialog
{
    Q_OBJECT
public:
    explicit BsNetLoading(const QString &waitHint);
    void setVisible(bool visible);
    void setError(const QString &err);

protected:
    void mousePressEvent(QMouseEvent *e);
    void timerEvent(QTimerEvent*);
    void paintEvent(QPaintEvent*);
private:
    bool        m_return_error;
    QString     m_text;
    int         m_step;
    int         m_timer_id;
};

/**********************************************************************************************/

//此类未用到，但可做编码规范参考
class QProgressIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int delay READ animationDelay WRITE setAnimationDelay)
    Q_PROPERTY(bool displayedWhenStopped READ isDisplayedWhenStopped WRITE setDisplayedWhenStopped)
    Q_PROPERTY(QColor color READ color WRITE setColor)
public:
    QProgressIndicator(QWidget* parent = nullptr);

    int animationDelay() const { return m_delay; }
    bool isAnimated() const;
    bool isDisplayedWhenStopped() const;
    const QColor& color() const { return m_color; }
    virtual QSize sizeHint() const;
    int heightForWidth(int) const;

public slots:
    void startAnimation();
    void stopAnimation();
    void setAnimationDelay(int delay);
    void setDisplayedWhenStopped(bool state);
    void setColor(const QColor& color);

protected:
    virtual void timerEvent(QTimerEvent* event);
    virtual void paintEvent(QPaintEvent* event);

private:
    int m_step;
    int m_timerId;
    int m_delay;
    bool m_displayedWhenStopped;
    QColor m_color;
};

}

#endif // BSNETLOADING_H
