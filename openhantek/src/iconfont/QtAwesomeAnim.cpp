#include "QtAwesomeAnim.h"

#include <QPainter>
#include <QRect>
#include <QTimer>
#include <QWidget>
#include <cmath>

QtAwesomeAnimation::QtAwesomeAnimation(QWidget *parentWidget, int interval, double step)
    : parentWidgetRef_(parentWidget), timer_(0), interval_(interval), step_(step), angle_(0.0f) {}

void QtAwesomeAnimation::setup(QPainter &painter, const QRect &rect) {
    // first time set the timer
    if (!timer_) {
        timer_ = new QTimer();
        connect(timer_, SIGNAL(timeout()), this, SLOT(update()));
        timer_->start(interval_);
    } else {
        QPen pen = painter.pen();
        pen.setWidth(2);
        pen.setColor(QColor(Qt::gray));
        painter.setPen(pen);
        double val = 1 + sin(angle_) / 2;
        if (val >= 0.5)
            painter.drawArc(rect, 0 * 16, 16 * (360 - (val - 0.5) * 2 * 360));
        else
            painter.drawArc(rect, 0 * 16, 16 * (val * 2) * 360);
    }
}

void QtAwesomeAnimation::update() {
    angle_ += step_;
    parentWidgetRef_->update();
}
