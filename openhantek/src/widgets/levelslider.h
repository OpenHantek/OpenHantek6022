// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

class QColor;

/// \brief Contains the color, text and value of one slider.
struct LevelSliderParameters {
    QColor color; ///< The color of the slider and font
    QString text; ///< The text beside the slider, a empty string disables text
    bool visible; ///< Visibility of the slider

    double minimum; ///< Minimum (left/top) value for the slider
    double maximum; ///< Maximum (right/bottom) value for the slider
    double step;    ///< The distance between selectable slider positions
    double value;   ///< The current value of the slider

    // Needed for moving and drawing
    QRect rect; ///< The area where the slider is drawn
};

////////////////////////////////////////////////////////////////////////////////
/// \class LevelSlider                                             levelslider.h
/// \brief Slider widget for multiple level sliders.
/// These are used for the trigger levels, offsets and so on.
class LevelSlider : public QWidget {
    Q_OBJECT

  public:
    LevelSlider( Qt::ArrowType direction = Qt::RightArrow, QWidget *parent = nullptr );
    ~LevelSlider() override;

    QSize sizeHint() const override;

    int preMargin() const;
    int postMargin() const;

    int addSlider( int index = -1 );
    int addSlider( const QString &text, int index = -1 );
    int removeSlider( int index = -1 );

    // Parameters for a specific slider
    const QColor color( int index ) const;
    void setColor( unsigned index, QColor color );
    const QString text( int index ) const;
    int setText( int index, const QString &text );
    bool visible( int index ) const;
    void setIndexVisible( unsigned index, bool visible );

    double minimum( int index ) const;
    double maximum( int index ) const;
    void setLimits( int index, double minimum, double maximum );
    double step( int index ) const;
    double setStep( int index, double step );
    double value( int index ) const;
    void setValue( int index, double value );

    // Parameters for all sliders
    Qt::ArrowType direction() const;
    int setDirection( Qt::ArrowType direction );

  protected:
    void mouseMoveEvent( QMouseEvent *event ) override;
    void mousePressEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;

    void paintEvent( QPaintEvent *event ) override;
    void resizeEvent( QResizeEvent *event ) override;

    QRect calculateRect( int sliderId );
    int calculateWidth();
    void fixValue( int index );

    QList< LevelSliderParameters * > slider; ///< The parameters for each slider
    int pressedSlider;                       ///< The currently pressed (moved) slider
    int sliderWidth;                         ///< The slider width (dimension orthogonal to the sliding
                                             /// direction)
    int needleWidth;                         ///< Width of the needle (parallel to sliding direction)

    Qt::ArrowType _direction; ///< The direction the sliders point to
    int _preMargin;           ///< The margin before the minimum slider position
    int _postMargin;          ///< The margin after the maximum slider position

  signals:
    void valueChanged( int index, double value, bool pressed = false,
                       QPoint globalPos = QPoint() ); ///< The value of a slider has changed
};
