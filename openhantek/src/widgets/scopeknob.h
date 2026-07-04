// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QBasicTimer>
#include <QColor>
#include <QWidget>

/// \brief A rotary knob like on a HW scope front panel (SCALE / TIME knobs).
/// Turn it by dragging with the mouse or by scrolling the mouse wheel,
/// or tap the "-" / "+" touch keys below it (press and hold repeats).
/// Each detent emits stepped( +1 ) (clockwise) or stepped( -1 ).
class ScopeKnob : public QWidget {
    Q_OBJECT

  public:
    explicit ScopeKnob( QWidget *parent = nullptr );
    void setAccentColor( const QColor &color );
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  signals:
    void stepped( int delta ); ///< +1 = clockwise detent, -1 = counter clockwise

  protected:
    void paintEvent( QPaintEvent *event ) override;
    void wheelEvent( QWheelEvent *event ) override;
    void mousePressEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void timerEvent( QTimerEvent *event ) override;

  private:
    QRectF knobRect() const;
    QRectF minusRect() const;
    QRectF plusRect() const;
    double angleOfCursor( const QPointF &pos ) const;
    void turn( int detents );

    QColor accent;
    double visualAngle = 0.0; ///< accumulated rotation of the indicator mark in degrees
    double dragAngle = 0.0;   ///< cursor angle at the last drag event
    double dragRest = 0.0;    ///< rotation collected since the last emitted detent
    int pressedButton = 0;    ///< -1 / +1 while the corresponding touch key is held down
    QBasicTimer repeatTimer;  ///< press and hold auto repeat for the touch keys
};
