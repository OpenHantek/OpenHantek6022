////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file levelslider.h
/// \brief Declares the LevelSlider class.
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


#ifndef LEVELSLIDER_H
#define LEVELSLIDER_H


#include <QWidget>


class QColor;


////////////////////////////////////////////////////////////////////////////////
/// \struct LevelSliderParameters                                  levelslider.h
/// \brief Contains the color, text and value of one slider.
struct LevelSliderParameters {
	QColor color;
	QString text;
	bool visible;
	
	double minimum, maximum;
	double step;
	double value;
	
	// Needed for moving and drawing
	QRect rect;
};

////////////////////////////////////////////////////////////////////////////////
/// \class LevelSlider                                             levelslider.h
/// \brief Slider widget for multiple level sliders.
/// These are used for the trigger levels, offsets and so on.
class LevelSlider : public QWidget {
	Q_OBJECT
	
	public:
		LevelSlider(Qt::ArrowType direction = Qt::RightArrow, QWidget *parent = 0);
		~LevelSlider();
		
		QSize sizeHint() const;
		
		int preMargin() const;
		int postMargin() const;
		
		int addSlider(int index = -1);
		int addSlider(QString text, int index = -1);
		int removeSlider(int index = -1);
		
		// Parameters for a specific slider
		const QColor color(int index) const;
		int setColor(int index, QColor color);
		const QString text(int index) const;
		int setText(int index, QString text);
		bool visible(int index) const;
		int setVisible(int index, bool visible);
		
		double minimum(int index) const;
		double maximum(int index) const;
		int setLimits(int index, double minimum, double maximum);
		double step(int index) const;
		double setStep(int index, double step);
		double value(int index) const;
		double setValue(int index, double value);
		
		// Parameters for all sliders
		Qt::ArrowType direction() const;
		int setDirection(Qt::ArrowType direction);
		
	protected:
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent *event);
		void mouseReleaseEvent(QMouseEvent *event);
		
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		
		QRect calculateRect(int sliderId);
		int calculateWidth();
		int fixValue(int index);
		
		QList<LevelSliderParameters *> slider;
		int pressedSlider;
		int sliderWidth;
		
		Qt::ArrowType _direction;
		int _preMargin, _postMargin;
	
	signals:
		void valueChanged(int index, double value);
};


#endif
