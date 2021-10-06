#include "bsnetloading.h"

#include <QtWidgets>

namespace BailiSoft {

BsNetLoading::BsNetLoading(const QString &waitHint)
    : QDialog(nullptr), m_return_error(false), m_text(waitHint), m_step(0), m_timer_id(0)
{
    setFixedSize(500, 75);
}

void BsNetLoading::setVisible(bool visible)
{
    QDialog::setVisible(visible);
    if ( visible ) {
        m_return_error = false;
        m_timer_id = startTimer(100);
    }
    else if ( m_timer_id ) {
        killTimer(m_timer_id);
        m_timer_id = 0;
    }
}

void BsNetLoading::setError(const QString &err)
{
    m_return_error = true;

    m_text = err;

    if ( m_timer_id ) {
        killTimer(m_timer_id);
        m_timer_id = 0;
    }

    update();
}

void BsNetLoading::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);

    if ( m_return_error ) {
        reject();
    }
}

void BsNetLoading::timerEvent(QTimerEvent *)
{
    m_step++;

    if ( m_step > 20 )
        m_step = 0;

    update();
}

void BsNetLoading::paintEvent(QPaintEvent *)
{
    const int grd_height = 9;
    const int border_width = 5;

    QPainter p(this);

    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    p.drawRect(-1, -1, width() + 1, height() + 1);

    QLinearGradient grd(0, 0, 2 * width(), 0);
    grd.setColorAt(0,               Qt::lightGray);
    grd.setColorAt(m_step / 20.0,   Qt::white);
    grd.setColorAt(1,               Qt::lightGray);

    if ( m_timer_id ) {
        p.setBrush(grd);
    } else {
        p.setBrush(Qt::lightGray);
    }
    p.drawRect(0, height() - grd_height - border_width, width(), height());

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(Qt::darkGreen), border_width));
    p.drawRect(-1, -1, width() + 1, height() + 1);

    QFont f(font());
    if ( m_timer_id ) {
        f.setPointSize( 3 * f.pointSize() / 2 );
        f.setBold(true);
    }
    p.setFont(f);
    p.drawText(rect().adjusted(8, 0, -8, -grd_height), Qt::AlignCenter | Qt::TextWordWrap, m_text);
}

/**********************************************************************************************/
//此类未用到，但可做编码规范参考
QProgressIndicator::QProgressIndicator(QWidget* parent)
    : QWidget(parent)
    , m_step(0)
    , m_timerId(-1)
    , m_delay(100)
    , m_displayedWhenStopped(false)
    , m_color(Qt::black)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
}

bool
QProgressIndicator::isAnimated() const
{
    return (m_timerId != -1);
}

void
QProgressIndicator::setDisplayedWhenStopped(bool state)
{
    m_displayedWhenStopped = state;

    update();
}

bool
QProgressIndicator::isDisplayedWhenStopped() const
{
    return m_displayedWhenStopped;
}

void
QProgressIndicator::startAnimation()
{
    m_step = 0;

    if (m_timerId == -1)
        m_timerId = startTimer(m_delay);
}

void
QProgressIndicator::stopAnimation()
{
    if (m_timerId != -1)
        killTimer(m_timerId);

    m_timerId = -1;

    update();
}

void
QProgressIndicator::setAnimationDelay(int delay)
{
    if (m_timerId != -1)
        killTimer(m_timerId);

    m_delay = delay;

    if (m_timerId != -1)
        m_timerId = startTimer(m_delay);
}

void
QProgressIndicator::setColor(const QColor& color)
{
    m_color = color;

    update();
}

QSize
QProgressIndicator::sizeHint() const
{
    return QSize(500, 30);
}

int
QProgressIndicator::heightForWidth(int) const
{
    return 30;
}

void
QProgressIndicator::timerEvent(QTimerEvent*)
{
    m_step++;

    if ( m_step > 20 )
        m_step = 0;

    update();
}

void
QProgressIndicator::paintEvent(QPaintEvent*)
{
    if (!m_displayedWhenStopped && !isAnimated())
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    qreal step = m_step / 20.0;

    QLinearGradient grd(0, 0, 3 * width(), 0);
    grd.setColorAt(0,      Qt::darkGreen);
    grd.setColorAt(step,   Qt::green);
    grd.setColorAt(1,      Qt::darkGreen);

    p.setBrush(grd);
    p.setPen(Qt::darkGreen);
    p.drawRect(0, 0, width(), height());
}

}
