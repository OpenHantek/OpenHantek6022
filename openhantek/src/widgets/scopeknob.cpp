// SPDX-License-Identifier: GPL-2.0-or-later

#include "scopeknob.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimerEvent>
#include <QWheelEvent>
#include <QtMath>

static const double DETENT_DEGREES = 18.0; // rotation per emitted step
static const int REPEAT_START_MS = 350;    // press and hold delay of the touch keys
static const int REPEAT_INTERVAL_MS = 120; // press and hold repeat rate

ScopeKnob::ScopeKnob( QWidget *parent ) : QWidget( parent ) {
    setCursor( Qt::PointingHandCursor );
    setFocusPolicy( Qt::NoFocus );
}


void ScopeKnob::setAccentColor( const QColor &color ) {
    accent = color;
    update();
}


QSize ScopeKnob::sizeHint() const {
    const int side = fontMetrics().height() * 6; // touch friendly big knob
    return QSize( side, side + fontMetrics().height() * 2 + 4 ); // knob plus the -/+ touch key row
}


QSize ScopeKnob::minimumSizeHint() const { return sizeHint(); }


QRectF ScopeKnob::knobRect() const {
    const double keyHeight = fontMetrics().height() * 2 + 4;
    return QRectF( 0, 0, width(), height() - keyHeight );
}


QRectF ScopeKnob::minusRect() const {
    const double keyHeight = fontMetrics().height() * 2 + 2;
    return QRectF( 1, height() - keyHeight, width() / 2.0 - 2, keyHeight - 1 );
}


QRectF ScopeKnob::plusRect() const {
    const double keyHeight = fontMetrics().height() * 2 + 2;
    return QRectF( width() / 2.0 + 1, height() - keyHeight, width() / 2.0 - 2, keyHeight - 1 );
}


static void paintTouchKey( QPainter &painter, const QRectF &rect, bool plus, bool pressed ) {
    painter.setPen( QPen( QColor( 0x15, 0x17, 0x1a ), 1 ) );
    painter.setBrush( pressed ? QColor( 0x20, 0x23, 0x28 ) : QColor( 0x37, 0x3b, 0x41 ) );
    painter.drawRoundedRect( rect, 3, 3 );
    painter.setPen( QPen( QColor( 0xd8, 0xda, 0xde ), 1.6, Qt::SolidLine, Qt::RoundCap ) );
    const QPointF center = rect.center();
    const double arm = qMin( rect.width(), rect.height() ) * 0.22;
    painter.drawLine( center - QPointF( arm, 0 ), center + QPointF( arm, 0 ) );
    if ( plus )
        painter.drawLine( center - QPointF( 0, arm ), center + QPointF( 0, arm ) );
}


void ScopeKnob::paintEvent( QPaintEvent * ) {
    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );
    const QRectF knob = knobRect();
    const double side = qMin( knob.width(), knob.height() );
    const QPointF center = knob.center();
    const double radius = side / 2.0 - 2.0;

    // twelve small tick dots around the knob like the detent marks of a HW knob
    painter.setPen( Qt::NoPen );
    painter.setBrush( QColor( 110, 115, 120 ) );
    for ( int tick = 0; tick < 12; ++tick ) {
        const double tickAngle = qDegreesToRadians( tick * 30.0 );
        const QPointF pos = center + QPointF( cos( tickAngle ), sin( tickAngle ) ) * radius;
        painter.drawEllipse( pos, 1.2, 1.2 );
    }

    // knob body: brushed metal look
    QRadialGradient body( center - QPointF( radius / 3, radius / 3 ), radius * 1.8 );
    body.setColorAt( 0.0, QColor( 0x63, 0x68, 0x6f ) );
    body.setColorAt( 0.6, QColor( 0x3a, 0x3e, 0x44 ) );
    body.setColorAt( 1.0, QColor( 0x24, 0x27, 0x2b ) );
    painter.setPen( QPen( QColor( 0x14, 0x16, 0x19 ), 1.2 ) );
    painter.setBrush( body );
    painter.drawEllipse( center, radius * 0.86, radius * 0.86 );

    // center cap
    QRadialGradient cap( center - QPointF( radius / 4, radius / 4 ), radius );
    cap.setColorAt( 0.0, QColor( 0x51, 0x56, 0x5d ) );
    cap.setColorAt( 1.0, QColor( 0x2c, 0x2f, 0x34 ) );
    painter.setPen( Qt::NoPen );
    painter.setBrush( cap );
    painter.drawEllipse( center, radius * 0.30, radius * 0.30 );

    // rotating indicator mark
    const double indicator = qDegreesToRadians( visualAngle - 90.0 ); // start at 12 o'clock
    const QPointF direction( cos( indicator ), sin( indicator ) );
    QPen markPen( accent.isValid() ? accent : QColor( 0xe8, 0xe8, 0xe8 ), radius * 0.14, Qt::SolidLine, Qt::RoundCap );
    painter.setPen( markPen );
    painter.drawLine( center + direction * radius * 0.42, center + direction * radius * 0.74 );

    // -/+ touch keys below the knob
    paintTouchKey( painter, minusRect(), false, pressedButton == -1 );
    paintTouchKey( painter, plusRect(), true, pressedButton == +1 );
}


void ScopeKnob::turn( int detents ) {
    if ( !detents || !isEnabled() )
        return;
    visualAngle += detents * DETENT_DEGREES;
    update();
    emit stepped( detents );
}


void ScopeKnob::wheelEvent( QWheelEvent *event ) {
    const int detents = event->angleDelta().y() / 120;
    if ( detents )
        turn( detents );
    event->accept();
}


double ScopeKnob::angleOfCursor( const QPointF &pos ) const {
    const QPointF center = knobRect().center();
    return qRadiansToDegrees( atan2( pos.y() - center.y(), pos.x() - center.x() ) );
}


void ScopeKnob::mousePressEvent( QMouseEvent *event ) {
    if ( minusRect().contains( event->position() ) )
        pressedButton = -1;
    else if ( plusRect().contains( event->position() ) )
        pressedButton = +1;
    else
        pressedButton = 0;
    if ( pressedButton ) { // touch key: step once, then auto repeat while held down
        turn( pressedButton );
        repeatTimer.start( REPEAT_START_MS, this );
    } else { // knob: start turning
        dragAngle = angleOfCursor( event->position() );
        dragRest = 0.0;
    }
    event->accept();
}


void ScopeKnob::mouseReleaseEvent( QMouseEvent *event ) {
    pressedButton = 0;
    repeatTimer.stop();
    update();
    event->accept();
}


void ScopeKnob::timerEvent( QTimerEvent *event ) {
    if ( event->timerId() != repeatTimer.timerId() ) {
        QWidget::timerEvent( event );
        return;
    }
    if ( pressedButton ) {
        turn( pressedButton );
        repeatTimer.start( REPEAT_INTERVAL_MS, this );
    } else
        repeatTimer.stop();
}


void ScopeKnob::mouseMoveEvent( QMouseEvent *event ) {
    if ( pressedButton ) { // finger stays on the touch key
        event->accept();
        return;
    }
    double delta = angleOfCursor( event->position() ) - dragAngle;
    while ( delta > 180.0 )
        delta -= 360.0;
    while ( delta < -180.0 )
        delta += 360.0;
    dragAngle = angleOfCursor( event->position() );
    dragRest += delta;
    const int detents = int( dragRest / DETENT_DEGREES );
    if ( detents ) {
        dragRest -= detents * DETENT_DEGREES;
        turn( detents );
    }
    event->accept();
}
