////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  levelslider.cpp
//
//  Copyright (C) 2010  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QWidget>

#include "levelslider.h"

////////////////////////////////////////////////////////////////////////////////
// class LevelSlider
/// \brief Initializes the slider container.
/// \param direction The side on which the sliders are shown.
/// \param parent The parent widget.
LevelSlider::LevelSlider( Qt::ArrowType direction, QWidget *parent ) : QWidget( parent ) {
    // Set pixel values based on the current dpi scaling
    needleWidth = ( int( 0.55 * fontMetrics().height() ) ) + 1; // always an odd number
    sliderWidth = ( int( 1.2 * fontMetrics().height() ) );

    pressedSlider = -1;

    calculateWidth();
    setDirection( direction );
}

/// \brief Cleans up the widget.
LevelSlider::~LevelSlider() {}

/// \brief Return the margin before the slider.
/// \return The margin the Slider has at the top/left.
int LevelSlider::preMargin() const { return _preMargin; }

/// \brief Return the margin after the slider.
/// \return The margin the Slider has at the bottom/right.
int LevelSlider::postMargin() const { return _postMargin; }

/// \brief Add a new slider to the slider container.
/// \param index The index where the slider should be inserted, 0 to append.
/// \return The index of the slider, -1 on error.
int LevelSlider::addSlider( int index ) { return addSlider( "", index ); }

/// \brief Add a new slider to the slider container.
/// \param text The text that will be shown next to the slider.
/// \param index The index where the slider should be inserted, 0 to append.
/// \return The index of the slider, -1 on error.
int LevelSlider::addSlider( const QString &text, int index ) {
    if ( index < -1 )
        return -1;

    LevelSliderParameters *parameters = new LevelSliderParameters;
    parameters->color = Qt::white;
    parameters->minimum = 0x00;
    parameters->maximum = 0xff;
    parameters->value = 0x00;
    parameters->visible = false;

    if ( index == -1 ) {
        slider.append( parameters );
        index = slider.count() - 1;
    } else
        slider.insert( index, parameters );

    setText( index, text );

    return index;
}

/// \brief Remove a slider from the slider container.
/// \param index The index of the slider that should be removed.
/// \return The index of the removed slider, -1 on error.
int LevelSlider::removeSlider( int index ) {
    if ( index < -1 )
        return -1;

    if ( index == -1 ) {
        slider.removeLast();
        index = slider.count();
    } else {
        slider.removeAt( index );
    }

    calculateWidth();

    return index;
}

/// \brief Size hint for the widget.
/// \return The recommended size for the widget.
QSize LevelSlider::sizeHint() const {
    if ( _direction == Qt::RightArrow || _direction == Qt::LeftArrow )
        return QSize( sliderWidth, 16 );
    else
        return QSize( 16, sliderWidth );
}

/// \brief Return the color of a slider.
/// \param index The index of the slider whose color should be returned.
/// \return The current color of the slider.
const QColor LevelSlider::color( int index ) const {
    if ( index < 0 || index >= slider.count() )
        return Qt::black;

    return slider[ index ]->color;
}

/// \brief Set the color of the slider.
/// \param index The index of the slider whose color should be set.
/// \param color The new color for the slider.
/// \return The index of the slider, -1 on error.
void LevelSlider::setColor( unsigned index, QColor color ) {
    if ( int( index ) >= slider.count() )
        return;

    slider[ int( index ) ]->color = color;
    repaint();
}

/// \brief Return the text shown beside a slider.
/// \param index The index of the slider whose text should be returned.
/// \return The current text of the slider.
const QString LevelSlider::text( int index ) const {
    if ( index < 0 || index >= slider.count() )
        return QString();

    return slider[ index ]->text;
}

/// \brief Set the text for a slider.
/// \param index The index of the slider whose text should be set.
/// \param text The text shown next to the slider.
/// \return The index of the slider, -1 on error.
int LevelSlider::setText( int index, const QString &text ) {
    if ( index < 0 || index >= slider.count() )
        return -1;

    slider[ index ]->text = text;
    calculateWidth();

    return index;
}

/// \brief Return the visibility of a slider.
/// \param index The index of the slider whose visibility should be returned.
/// \return true if the slider is visible, false if it's hidden.
bool LevelSlider::visible( int index ) const {
    if ( index < 0 || index >= slider.count() )
        return false;

    return slider[ index ]->visible;
}

/// \brief Set the visibility of a slider.
/// \param index The index of the slider whose visibility should be set.
/// \param visible true to show the slider, false to hide it.
/// \return The index of the slider, -1 on error.
void LevelSlider::setIndexVisible( unsigned index, bool visible ) {
    if ( int( index ) >= slider.count() )
        return;

    slider[ int( index ) ]->visible = visible;
    repaint();
}

/// \brief Return the minimal value of the sliders.
/// \return The value a slider has at the bottommost/leftmost position.
double LevelSlider::minimum( int index ) const {
    if ( index < 0 || index >= slider.count() )
        return -1;

    return slider[ index ]->minimum;
}

/// \brief Return the maximal value of the sliders.
/// \return The value a slider has at the topmost/rightmost position.
double LevelSlider::maximum( int index ) const {
    if ( index < 0 || index >= slider.count() )
        return -1;

    return slider[ index ]->maximum;
}

/// \brief Set the min-max values of the sliders, correct the value if changed.
/// \param index The index of the slider whose limits should be set.
/// \param minimum The value a slider has at the bottommost/leftmost position.
/// \param maximum The value a slider has at the topmost/rightmost position.
void LevelSlider::setLimits( int index, double minimum, double maximum ) {
    if ( index < 0 || index >= slider.count() )
        return;
    double lastValue = slider[ index ]->value;
    slider[ index ]->minimum = minimum;
    slider[ index ]->maximum = maximum;
    fixValue( index );
    calculateRect( index );
    repaint();
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    if ( lastValue != slider[ index ]->value )
        emit valueChanged( index, slider[ index ]->value );
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

/// \brief Return the step width of the sliders.
/// \param index The index of the slider whose step width should be returned.
/// \return The distance between the selectable slider positions.
double LevelSlider::step( int index ) const {
    if ( index < 0 || index >= slider.count() )
        return -1;

    return slider[ index ]->step;
}

/// \brief Set the step width of the sliders.
/// \param index The index of the slider whose step width should be set.
/// \param step The distance between the selectable slider positions.
/// \return The new step width.
double LevelSlider::setStep( int index, double step ) {
    if ( index < 0 || index >= slider.count() )
        return -1;

    if ( step > 0 )
        slider[ index ]->step = step;

    return slider[ index ]->step;
}

/// \brief Return the current position of a slider.
/// \param index The index of the slider whose value should be returned.
/// \return The value of the slider.
double LevelSlider::value( int index ) const {
    if ( index < 0 || index >= slider.count() )
        return -1;

    return slider[ index ]->value;
}

/// \brief Set the current position of a slider.
/// \param index The index of the slider whose value should be set.
/// \param value The new value of the slider.
/// \return The new value of the slider.
void LevelSlider::setValue( int index, double value ) {
    if ( index < 0 || index >= slider.count() )
        return;

    // Apply new value
    slider[ index ]->value = value;
    fixValue( index );

    calculateRect( index );
    repaint();

    if ( pressedSlider < 0 )
        emit valueChanged( index, value );
}

/// \brief Return the direction of the sliders.
/// \return The side on which the sliders are shown.
Qt::ArrowType LevelSlider::direction() const { return _direction; }

/// \brief Set the direction of the sliders.
/// \param direction The side on which the sliders are shown.
/// \return The index of the direction, -1 on error.
int LevelSlider::setDirection( Qt::ArrowType direction ) {
    if ( direction < Qt::UpArrow || direction > Qt::RightArrow )
        return -1;

    _direction = direction;

    if ( _direction == Qt::RightArrow || _direction == Qt::LeftArrow ) {
        _preMargin = fontMetrics().lineSpacing();
        _postMargin = ( needleWidth );
    } else {
        _preMargin = fontMetrics().averageCharWidth() * 5;
        _postMargin = (needleWidth)*2;
    }

    return _direction;
}

/// \brief Move the slider if it's pressed.
/// \param event The mouse event that should be handled.
void LevelSlider::mouseMoveEvent( QMouseEvent *event ) {
    if ( pressedSlider < 0 ) {
        event->ignore();
        return;
    }

    // Get new value
    double value;
    if ( _direction == Qt::RightArrow || _direction == Qt::LeftArrow )
        value = slider[ pressedSlider ]->maximum - ( slider[ pressedSlider ]->maximum - slider[ pressedSlider ]->minimum ) *
                                                       ( double( event->y() ) - _preMargin + 0.5 ) /
                                                       ( height() - _preMargin - _postMargin - 1 );
    else
        value = slider[ pressedSlider ]->minimum + ( slider[ pressedSlider ]->maximum - slider[ pressedSlider ]->minimum ) *
                                                       ( double( event->x() ) - _preMargin + 0.5 ) /
                                                       ( width() - _preMargin - _postMargin - 1 );

    // Move the slider
    if ( event->modifiers() & Qt::AltModifier )
        // Alt allows every position
        setValue( pressedSlider, value );
    else
        // Set to nearest possible position
        setValue( pressedSlider, floor( value / slider[ pressedSlider ]->step + 0.5 ) * slider[ pressedSlider ]->step );

    emit valueChanged( pressedSlider, slider[ pressedSlider ]->value, true, event->globalPos() );
    event->accept();
}

/// \brief Prepare slider for movement if the left mouse button is pressed.
/// \param event The mouse event that should be handled.
void LevelSlider::mousePressEvent( QMouseEvent *event ) {
    if ( !( event->button() & Qt::LeftButton ) ) {
        event->ignore();
        return;
    }

    pressedSlider = -1;
    for ( int sliderId = 0; sliderId < slider.count(); ++sliderId ) {
        if ( slider[ sliderId ]->visible && slider[ sliderId ]->rect.contains( event->pos() ) ) {
            pressedSlider = sliderId;
            break;
        }
    }
    if ( pressedSlider >= 0 )
        emit valueChanged( pressedSlider, slider[ pressedSlider ]->value, true, event->globalPos() );

    // Accept event if a slider was pressed
    event->setAccepted( pressedSlider >= 0 );
}

/// \brief Movement is done if the left mouse button is released.
/// \param event The mouse event that should be handled.
void LevelSlider::mouseReleaseEvent( QMouseEvent *event ) {
    if ( !( event->button() & Qt::LeftButton ) || pressedSlider == -1 ) {
        event->ignore();
        return;
    }
    emit valueChanged( pressedSlider, slider[ pressedSlider ]->value, false, event->globalPos() );
    pressedSlider = -1;

    event->accept();
}

/// \brief Paint the widget.
/// \param event The paint event that should be handled.
void LevelSlider::paintEvent( QPaintEvent *event ) {
    QPainter painter( this );

    Qt::Alignment alignment;
    switch ( _direction ) {
    case Qt::LeftArrow:
        alignment = Qt::AlignLeft | Qt::AlignBottom;
        break;
    case Qt::UpArrow:
        alignment = Qt::AlignTop | Qt::AlignHCenter;
        break;
    case Qt::DownArrow:
        alignment = Qt::AlignBottom | Qt::AlignHCenter;
        break;
    default:
        alignment = Qt::AlignRight | Qt::AlignBottom;
    }

    QList< LevelSliderParameters * >::iterator sliderIt = slider.end();
    while ( sliderIt != slider.begin() ) {
        --sliderIt;

        if ( !( *sliderIt )->visible )
            continue;

        painter.setPen( ( *sliderIt )->color );

        if ( ( *sliderIt )->text.isEmpty() ) {
            QVector< QPoint > needlePoints;
            QRect &needleRect = ( *sliderIt )->rect;
            const int peak = 1;                          // distance from slider to the tip of the needle
            const int shoulder = peak + needleWidth / 2; // distance from slider to the straight part of the needle

            switch ( _direction ) {
            case Qt::LeftArrow:
                needlePoints << QPoint( needleRect.left() + shoulder, needleRect.top() )
                             << QPoint( needleRect.left() + peak, needleRect.top() + needleWidth / 2 )
                             << QPoint( needleRect.left() + shoulder, needleRect.bottom() )
                             << QPoint( needleRect.right(), needleRect.bottom() ) << QPoint( needleRect.right(), needleRect.top() );
                break;
            case Qt::UpArrow:
                needlePoints << QPoint( needleRect.left(), needleRect.top() + shoulder )
                             << QPoint( needleRect.left() + needleWidth / 2, needleRect.top() + peak )
                             << QPoint( needleRect.right(), needleRect.top() + shoulder )
                             << QPoint( needleRect.right(), needleRect.bottom() )
                             << QPoint( needleRect.left(), needleRect.bottom() );
                break;
            case Qt::DownArrow:
                needlePoints << QPoint( needleRect.left(), needleRect.bottom() - shoulder )
                             << QPoint( needleRect.left() + needleWidth / 2, needleRect.bottom() - peak )
                             << QPoint( needleRect.right(), needleRect.bottom() - shoulder )
                             << QPoint( needleRect.right(), needleRect.top() ) << QPoint( needleRect.left(), needleRect.top() );
                break;
            case Qt::RightArrow:
                needlePoints << QPoint( needleRect.right() - shoulder, needleRect.top() )
                             << QPoint( needleRect.right() - peak, needleRect.top() + needleWidth / 2 )
                             << QPoint( needleRect.right() - shoulder, needleRect.bottom() )
                             << QPoint( needleRect.left(), needleRect.bottom() ) << QPoint( needleRect.left(), needleRect.top() );
                break;
            default:
                break;
            }

            painter.setBrush( QBrush( ( *sliderIt )->color, isEnabled() ? Qt::SolidPattern : Qt::NoBrush ) );
            painter.drawPolygon( QPolygon( needlePoints ) );
            painter.setBrush( Qt::NoBrush );
        } else {
            // Get rect for text and draw needle
            QRect textRect = ( *sliderIt )->rect;
            if ( _direction == Qt::UpArrow || _direction == Qt::DownArrow ) {
                textRect.setRight( textRect.right() - 1 );
                if ( _direction == Qt::UpArrow ) {
                    textRect.setTop( textRect.top() + 1 );
                    painter.drawLine( ( *sliderIt )->rect.right(), 0, ( *sliderIt )->rect.right(), 7 );
                } else {
                    textRect.setBottom( textRect.bottom() - 1 );
                    painter.drawLine( ( *sliderIt )->rect.right(), sliderWidth - 8, ( *sliderIt )->rect.right(), sliderWidth - 1 );
                }
            } else {
                textRect.setBottom( textRect.bottom() - 1 );
                if ( _direction == Qt::LeftArrow ) {
                    textRect.setLeft( textRect.left() + 1 );
                    painter.drawLine( 0, ( *sliderIt )->rect.bottom(), 7, ( *sliderIt )->rect.bottom() );
                } else {
                    textRect.setRight( textRect.right() - 1 );
                    painter.drawLine( sliderWidth - 8, ( *sliderIt )->rect.bottom(), sliderWidth - 1,
                                      ( *sliderIt )->rect.bottom() );
                }
            }
            // Draw text
            painter.drawText( textRect, int( alignment ), ( *sliderIt )->text );
        }
    }

    event->accept();
}

/// \brief Resize the widget and adapt the slider positions.
/// \param event The resize event that should be handled.
void LevelSlider::resizeEvent( QResizeEvent *event ) {
    Q_UNUSED( event )

    for ( int sliderId = 0; sliderId < slider.count(); ++sliderId )
        calculateRect( sliderId );

    repaint();
}

/// \brief Calculate the drawing area for the slider for it's current value.
/// \param sliderId The id of the slider whose rect should be calculated.
/// \return The calculated rect.
QRect LevelSlider::calculateRect( int sliderId ) {
    // Is it a vertical slider?
    if ( _direction == Qt::RightArrow || _direction == Qt::LeftArrow ) {
        // Is it a triangular needle?
        if ( slider[ sliderId ]->text.isEmpty() ) {
            slider[ sliderId ]->rect = QRect(
                0, // Start at the left side
                   // The needle should be center-aligned, 0.5 pixel offset for
                   // exact pixelization
                int( ( height() - _preMargin - _postMargin - 1 ) * ( slider[ sliderId ]->maximum - slider[ sliderId ]->value ) /
                         ( slider[ sliderId ]->maximum - slider[ sliderId ]->minimum ) +
                     0.5 ) +
                    _preMargin - ( needleWidth / 2 ),
                sliderWidth, // Fill the whole width
                needleWidth  // one needle width high
            );
        }
        // Or a thin needle with text?
        else {
            slider[ sliderId ]->rect = QRect(
                0, // Start at the left side
                   // The needle is at the bottom, the text above it, 0.5 pixel
                   // offset for exact pixelization
                int( ( height() - _preMargin - _postMargin - 1 ) * ( slider[ sliderId ]->maximum - slider[ sliderId ]->value ) /
                         ( slider[ sliderId ]->maximum - slider[ sliderId ]->minimum ) +
                     0.5 ),
                sliderWidth,    // Fill the whole width
                preMargin() + 1 // Use the full margin
            );
        }
    }
    // Or a horizontal slider?
    else {
        // Is it a triangular needle?
        if ( slider[ sliderId ]->text.isEmpty() ) {
            slider[ sliderId ]->rect = QRect(
                // The needle should be center-aligned, 0.5 pixel offset for exact
                // pixelization
                int( ( width() - _preMargin - _postMargin - 1 ) * ( slider[ sliderId ]->value - slider[ sliderId ]->minimum ) /
                         ( slider[ sliderId ]->maximum - slider[ sliderId ]->minimum ) +
                     0.5 ) +
                    _preMargin - ( needleWidth / 2 ),
                0, // Start at the top
                needleWidth,
                sliderWidth // As high as the slider
            );
        }
        // Or a thin needle with text?
        else {
            int sliderLength = fontMetrics().size( 0, slider[ sliderId ]->text ).width() + 2;
            slider[ sliderId ]->rect = QRect(
                // The needle is at the right side, the text before it, 0.5 pixel
                // offset for exact pixelization
                int( ( width() - _preMargin - _postMargin - 1 ) * ( slider[ sliderId ]->value - slider[ sliderId ]->minimum ) /
                         ( slider[ sliderId ]->maximum - slider[ sliderId ]->minimum ) +
                     0.5 ) +
                    _preMargin - sliderLength + 1,
                0,            // Start at the top
                sliderLength, // The width depends on the text
                sliderWidth   // Fill the whole height
            );
        }
    }

    return slider[ sliderId ]->rect;
}

/// \brief Search for the widest slider element.
/// \return The calculated width of the slider.
int LevelSlider::calculateWidth() {
    // Is it a vertical slider?
    if ( _direction == Qt::RightArrow || _direction == Qt::LeftArrow ) {
        for ( QList< LevelSliderParameters * >::iterator sliderIt = slider.begin(); sliderIt != slider.end(); ++sliderIt ) {
            int newSliderWidth = fontMetrics().size( 0, ( *sliderIt )->text ).width();
            if ( newSliderWidth > sliderWidth )
                sliderWidth = newSliderWidth;
        }
    }
    // Or a horizontal slider?
    else {
        for ( QList< LevelSliderParameters * >::iterator sliderIt = slider.begin(); sliderIt != slider.end(); ++sliderIt ) {
            int newSliderWidth = fontMetrics().size( 0, ( *sliderIt )->text ).height();
            if ( newSliderWidth > sliderWidth )
                sliderWidth = newSliderWidth;
        }
    }

    return sliderWidth;
}

/// \brief Fix the value if it's outside the limits.
/// \param index The index of the slider who should be fixed.
void LevelSlider::fixValue( int index ) {
    if ( index < 0 || index >= slider.count() )
        return;

    double lowest = qMin( slider[ index ]->minimum, slider[ index ]->maximum );
    double highest = qMax( slider[ index ]->minimum, slider[ index ]->maximum );
    if ( slider[ index ]->value < lowest ) {
        slider[ index ]->value = lowest;
    } else if ( slider[ index ]->value > highest ) {
        slider[ index ]->value = highest;
    }
}
