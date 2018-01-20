#ifndef QTAWESOMEANIMATION_H
#define QTAWESOMEANIMATION_H

#include <QObject>

class QPainter;
class QRect;
class QTimer;
class QWidget;

///
/// Basic Animation Support for QtAwesome (Inspired by https://github.com/spyder-ide/qtawesome)
///
class QtAwesomeAnimation : public QObject
{
Q_OBJECT

public:
    QtAwesomeAnimation( QWidget* parentWidget, int interval=20, double step=0.01);

    void setup( QPainter& painter, const QRect& rect );

public slots:
    void update();

private:
    QWidget* parentWidgetRef_;
    QTimer* timer_;
    int interval_;
    double step_;
    double angle_;

};


#endif // QTAWESOMEANIMATION_H
