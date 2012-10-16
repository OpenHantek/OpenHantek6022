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
LevelSlider::LevelSlider(Qt::ArrowType direction, QWidget *parent) : QWidget(parent) {
	QFont font = this->font();
	font.setPointSize(font.pointSize() * 0.8);
	this->setFont(font);
	
	this->pressedSlider = -1;
	this->sliderWidth = 8;
	
	this->setDirection(direction);
}

/// \brief Cleans up the widget.
LevelSlider::~LevelSlider() {
}

/// \brief Return the margin before the slider.
/// \return The margin the Slider has at the top/left.
int LevelSlider::preMargin() const {
	return this->_preMargin;
}

/// \brief Return the margin after the slider.
/// \return The margin the Slider has at the bottom/right.
int LevelSlider::postMargin() const {
	return this->_postMargin;
}

/// \brief Add a new slider to the slider container.
/// \param index The index where the slider should be inserted, 0 to append.
/// \return The index of the slider, -1 on error.
int LevelSlider::addSlider(int index) {
	return this->addSlider(0, index);
}

/// \brief Add a new slider to the slider container.
/// \param text The text that will be shown next to the slider.
/// \param index The index where the slider should be inserted, 0 to append.
/// \return The index of the slider, -1 on error.
int LevelSlider::addSlider(QString text, int index) {
	if(index < -1)
		return -1;
	
	LevelSliderParameters *parameters = new LevelSliderParameters;
	parameters->color = Qt::white;
	parameters->minimum = 0x00;
	parameters->maximum = 0xff;
	parameters->value = 0x00;
	parameters->visible = false;
	
	if(index == -1) {
		this->slider.append(parameters);
		index = this->slider.count() - 1;
	}
	else
		this->slider.insert(index, parameters);
	
	this->setText(index, text);
	
	return index;
}

/// \brief Remove a slider from the slider container.
/// \param index The index of the slider that should be removed.
/// \return The index of the removed slider, -1 on error.
int LevelSlider::removeSlider(int index) {
	if(index < -1)
		return -1;
	
	if(index == -1) {
		this->slider.removeLast();
		index = this->slider.count();
	}
	else {
		this->slider.removeAt(index);
	}
	
	this->calculateWidth();
	
	return index;
}

/// \brief Size hint for the widget.
/// \return The recommended size for the widget.
QSize LevelSlider::sizeHint() const {
	if(this->_direction == Qt::RightArrow || this->_direction == Qt::LeftArrow)
		return QSize(this->sliderWidth, 16);
	else
		return QSize(16, this->sliderWidth);
}

/// \brief Return the color of a slider.
/// \param index The index of the slider whose color should be returned.
/// \return The current color of the slider.
const QColor LevelSlider::color(int index) const {
	if(index < 0 || index >= this->slider.count())
		return Qt::black;
	
	return this->slider[index]->color;
}

/// \brief Set the color of the slider.
/// \param index The index of the slider whose color should be set.
/// \param color The new color for the slider.
/// \return The index of the slider, -1 on error.
int LevelSlider::setColor(int index, QColor color) {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	this->slider[index]->color = color;
	this->repaint();
	
	return index;
}

/// \brief Return the text shown beside a slider.
/// \param index The index of the slider whose text should be returned.
/// \return The current text of the slider.
const QString LevelSlider::text(int index) const {
	if(index < 0 || index >= this->slider.count())
		return QString();
	
	return this->slider[index]->text;
}

/// \brief Set the text for a slider.
/// \param index The index of the slider whose text should be set.
/// \param text The text shown next to the slider.
/// \return The index of the slider, -1 on error.
int LevelSlider::setText(int index, QString text) {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	this->slider[index]->text = text;
	this->calculateWidth();
	
	return index;
}

/// \brief Return the visibility of a slider.
/// \param index The index of the slider whose visibility should be returned.
/// \return true if the slider is visible, false if it's hidden.
bool LevelSlider::visible(int index) const {
	if(index < 0 || index >= this->slider.count())
		return false;
	
	return this->slider[index]->visible;
}

/// \brief Set the visibility of a slider.
/// \param index The index of the slider whose visibility should be set.
/// \param visible true to show the slider, false to hide it.
/// \return The index of the slider, -1 on error.
int LevelSlider::setVisible(int index, bool visible) {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	this->slider[index]->visible = visible;
	this->repaint();
	
	return index;
}

/// \brief Return the minimal value of the sliders.
/// \return The value a slider has at the bottommost/leftmost position.
double LevelSlider::minimum(int index) const {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	return this->slider[index]->minimum;
}

/// \brief Return the maximal value of the sliders.
/// \return The value a slider has at the topmost/rightmost position.
double LevelSlider::maximum(int index) const {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	return this->slider[index]->maximum;
}

/// \brief Set the maximal value of the sliders.
/// \param index The index of the slider whose limits should be set.
/// \param minimum The value a slider has at the bottommost/leftmost position.
/// \param maximum The value a slider has at the topmost/rightmost position.
/// \return -1 on error, fixValue result on success.
int LevelSlider::setLimits(int index, double minimum, double maximum) {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	this->slider[index]->minimum = minimum;
	this->slider[index]->maximum = maximum;
	int result = this->fixValue(index);
	
	this->calculateRect(index);
	this->repaint();
	
	return result;
}

/// \brief Return the step width of the sliders.
/// \param index The index of the slider whose step width should be returned.
/// \return The distance between the selectable slider positions.
double LevelSlider::step(int index) const {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	return this->slider[index]->step;
}

/// \brief Set the step width of the sliders.
/// \param index The index of the slider whose step width should be set.
/// \param step The distance between the selectable slider positions.
/// \return The new step width.
double LevelSlider::setStep(int index, double step) {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	if(step > 0)
		this->slider[index]->step = step;
	
	return this->slider[index]->step;
}

/// \brief Return the current position of a slider.
/// \param index The index of the slider whose value should be returned.
/// \return The value of the slider.
double LevelSlider::value(int index) const {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	return this->slider[index]->value;
}

/// \brief Set the current position of a slider.
/// \param index The index of the slider whose value should be set.
/// \param value The new value of the slider.
/// \return The new value of the slider.
double LevelSlider::setValue(int index, double value) {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	// Apply new value
	this->slider[index]->value = value;
	this->fixValue(index);
	
	this->calculateRect(index);
	this->repaint();
	
	if(this->pressedSlider < 0)
		emit valueChanged(index, value);
	
	return this->slider[index]->value;
}

/// \brief Return the direction of the sliders.
/// \return The side on which the sliders are shown.
Qt::ArrowType LevelSlider::direction() const {
	return this->_direction;
}

/// \brief Set the direction of the sliders.
/// \param direction The side on which the sliders are shown.
/// \return The index of the direction, -1 on error.
int LevelSlider::setDirection(Qt::ArrowType direction) {
	if(direction < Qt::UpArrow || direction > Qt::RightArrow)
		return -1;
	
	this->_direction = direction;
	
	if(this->_direction == Qt::RightArrow || this->_direction == Qt::LeftArrow) {
		this->_preMargin = this->fontMetrics().lineSpacing();
		this->_postMargin = 3;
	}
	else {
		this->_preMargin = this->fontMetrics().averageCharWidth() * 3;
		this->_postMargin = 3;
	}
	
	return this->_direction;
}

/// \brief Move the slider if it's pressed.
/// \param event The mouse event that should be handled.
void LevelSlider::mouseMoveEvent(QMouseEvent *event) {
	if(this->pressedSlider < 0) {
		event->ignore();
		return;
	}
	
	// Get new value
	double value;
	if(this->_direction == Qt::RightArrow || this->_direction == Qt::LeftArrow)
		value = this->slider[pressedSlider]->maximum - (this->slider[pressedSlider]->maximum - this->slider[pressedSlider]->minimum) * ((double) event->y() - this->_preMargin + 0.5) / (this->height() - this->_preMargin - this->_postMargin - 1);
	else
		value = this->slider[pressedSlider]->minimum + (this->slider[pressedSlider]->maximum - this->slider[pressedSlider]->minimum) * ((double) event->x() - this->_preMargin + 0.5) / (this->width() - this->_preMargin - this->_postMargin - 1);
	
	// Move the slider
	if(event->modifiers() & Qt::AltModifier)
		// Alt allows every position
		this->setValue(this->pressedSlider, value);
	else
		// Set to nearest possible position
		this->setValue(this->pressedSlider, floor(value / this->slider[pressedSlider]->step + 0.5) * this->slider[pressedSlider]->step);
	
	event->accept();
}

/// \brief Prepare slider for movement if the left mouse button is pressed.
/// \param event The mouse event that should be handled.
void LevelSlider::mousePressEvent(QMouseEvent *event) {
	if(!(event->button() & Qt::LeftButton)) {
		event->ignore();
		return;
	}
	
	this->pressedSlider = -1;
	for(int sliderId = 0; sliderId < this->slider.count(); ++sliderId) {
		if(this->slider[sliderId]->visible && this->slider[sliderId]->rect.contains(event->pos())) {
			this->pressedSlider = sliderId;
			break;
		}
	}
	
	// Accept event if a slider was pressed
	event->setAccepted(this->pressedSlider >= 0);
}

/// \brief Movement is done if the left mouse button is released.
/// \param event The mouse event that should be handled.
void LevelSlider::mouseReleaseEvent(QMouseEvent *event) {
	if(!(event->button() & Qt::LeftButton) || this->pressedSlider == -1) {
		event->ignore();
		return;
	}
	
	emit valueChanged(this->pressedSlider, this->slider[this->pressedSlider]->value);
	this->pressedSlider = -1;
	
	event->accept();
}

/// \brief Paint the widget.
/// \param event The paint event that should be handled.
void LevelSlider::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	
	Qt::Alignment alignment;
	switch(this->_direction) {
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
	
	QList<LevelSliderParameters *>::iterator slider = this->slider.end();
	while(slider != this->slider.begin()) {
		--slider;
		
		if(!(*slider)->visible)
			continue;
		
		painter.setPen((*slider)->color);
		
		if((*slider)->text.isEmpty()) {
			int needlePoints[6];
			
			switch(this->_direction) {
				case Qt::LeftArrow:
					needlePoints[0] = (*slider)->rect.left() + 4; needlePoints[1] = (*slider)->rect.top();
					needlePoints[2] = (*slider)->rect.left() + 1; needlePoints[3] = (*slider)->rect.top() + 3;
					needlePoints[4] = (*slider)->rect.left() + 4; needlePoints[5] = (*slider)->rect.top() + 6;
					break;
				case Qt::UpArrow:
					needlePoints[0] = (*slider)->rect.left(); needlePoints[1] = (*slider)->rect.top() + 4;
					needlePoints[2] = (*slider)->rect.left() + 3; needlePoints[3] = (*slider)->rect.top() + 1;
					needlePoints[4] = (*slider)->rect.left() + 6; needlePoints[5] = (*slider)->rect.top() + 4;
					break;
				case Qt::DownArrow:
					needlePoints[0] = (*slider)->rect.left(); needlePoints[1] = (*slider)->rect.top() + this->sliderWidth - 5;
					needlePoints[2] = (*slider)->rect.left() + 3; needlePoints[3] = (*slider)->rect.top() + this->sliderWidth - 2;
					needlePoints[4] = (*slider)->rect.left() + 6; needlePoints[5] = (*slider)->rect.top() + this->sliderWidth - 5;
					break;
				default:
					needlePoints[0] = (*slider)->rect.left() + this->sliderWidth - 5; needlePoints[1] = (*slider)->rect.top();
					needlePoints[2] = (*slider)->rect.left() + this->sliderWidth - 2; needlePoints[3] = (*slider)->rect.top() + 3;
					needlePoints[4] = (*slider)->rect.left() + this->sliderWidth - 5; needlePoints[5] =(*slider)->rect.top() + 6;
			}
			
			painter.setBrush(QBrush((*slider)->color, Qt::SolidPattern));
			painter.drawPolygon(QPolygon(3, needlePoints));
			painter.setBrush(Qt::NoBrush);
		}
		else {
			// Get rect for text and draw needle
			QRect textRect = (*slider)->rect;
			if(this->_direction == Qt::UpArrow || this->_direction == Qt::DownArrow) {
				textRect.setRight(textRect.right() - 1);
				if(this->_direction == Qt::UpArrow) {
					textRect.setTop(textRect.top() + 1);
					painter.drawLine((*slider)->rect.right(), 0, (*slider)->rect.right(), 7);
				}
				else {
					textRect.setBottom(textRect.bottom() - 1);
					painter.drawLine((*slider)->rect.right(), this->sliderWidth - 8, (*slider)->rect.right(), this->sliderWidth - 1);
				}
			}
			else
			{
				textRect.setBottom(textRect.bottom() - 1);
				if(this->_direction == Qt::LeftArrow) {
					textRect.setLeft(textRect.left() + 1);
					painter.drawLine(0, (*slider)->rect.bottom(), 7, (*slider)->rect.bottom());
				}
				else {
					textRect.setRight(textRect.right() - 1);
					painter.drawLine(this->sliderWidth - 8, (*slider)->rect.bottom(), this->sliderWidth - 1, (*slider)->rect.bottom());
				}
			}
			// Draw text
			painter.drawText(textRect, alignment, (*slider)->text);
		}
	}
	
	event->accept();
}

/// \brief Resize the widget and adapt the slider positions.
/// \param event The resize event that should be handled.
void LevelSlider::resizeEvent(QResizeEvent *event) {
	Q_UNUSED(event);
	
	for(int sliderId = 0; sliderId < this->slider.count(); ++sliderId)
		this->calculateRect(sliderId);
	
	this->repaint();
}

/// \brief Calculate the drawing area for the slider for it's current value.
/// \param sliderId The id of the slider whose rect should be calculated.
/// \return The calculated rect.
QRect LevelSlider::calculateRect(int sliderId) {
	// Is it a vertical slider?
	if(this->_direction == Qt::RightArrow || this->_direction == Qt::LeftArrow) {
		// Is it a triangular needle?
		if(this->slider[sliderId]->text.isEmpty()) {
			this->slider[sliderId]->rect = QRect(
				0,                              // Start at the left side
				// The needle should be center-aligned, 0.5 pixel offset for exact pixelization
				(long) ((double) (this->height() - this->_preMargin - this->_postMargin - 1) * (this->slider[sliderId]->maximum - this->slider[sliderId]->value) / (this->slider[sliderId]->maximum - this->slider[sliderId]->minimum) + 0.5f) + this->_preMargin - 3,
				this->sliderWidth,              // Fill the whole width
				7                               // The needle is 7 px wide
			);
		}
		// Or a thin needle with text?
		else {
			this->slider[sliderId]->rect = QRect(
				0,                              // Start at the left side
				// The needle is at the bottom, the text above it, 0.5 pixel offset for exact pixelization
				(long) ((double) (this->height() - this->_preMargin - this->_postMargin - 1) * (this->slider[sliderId]->maximum - this->slider[sliderId]->value) / (this->slider[sliderId]->maximum - this->slider[sliderId]->minimum) + 0.5f),
				this->sliderWidth,              // Fill the whole width
				this->preMargin() + 1           // Use the full margin
			);
		}
	}
	// Or a horizontal slider?
	else {
		// Is it a triangular needle?
		if(this->slider[sliderId]->text.isEmpty()) {
			this->slider[sliderId]->rect = QRect(
				// The needle should be center-aligned, 0.5 pixel offset for exact pixelization
				(long) ((double) (this->width() - this->_preMargin - this->_postMargin - 1) * (this->slider[sliderId]->value - this->slider[sliderId]->minimum) / (this->slider[sliderId]->maximum - this->slider[sliderId]->minimum) + 0.5f) + this->_preMargin - 3,
				0,                              // Start at the top
				7,                              // The needle is 7 px wide
				this->sliderWidth               // Fill the whole height
			);
		}
		// Or a thin needle with text?
		else {
			int sliderLength = this->fontMetrics().size(0, this->slider[sliderId]->text).width() + 2;
			this->slider[sliderId]->rect = QRect(
				// The needle is at the right side, the text before it, 0.5 pixel offset for exact pixelization
				(long) ((double) (this->width() - this->_preMargin - this->_postMargin - 1) * (this->slider[sliderId]->value - this->slider[sliderId]->minimum) / (this->slider[sliderId]->maximum - this->slider[sliderId]->minimum) + 0.5f) + this->_preMargin - sliderLength + 1,
				0,                              // Start at the top
				sliderLength,                   // The width depends on the text
				this->sliderWidth               // Fill the whole height
			);
		}
	}
	
	return this->slider[sliderId]->rect;
}

/// \brief Search for the widest slider element.
/// \return The calculated width of the slider.
int LevelSlider::calculateWidth() {
	// At least 8 px for the needles
	this->sliderWidth = 8;
	
	// Is it a vertical slider?
	if(this->_direction == Qt::RightArrow || this->_direction == Qt::LeftArrow) {
		for(QList<LevelSliderParameters *>::iterator slider = this->slider.begin(); slider != this->slider.end(); ++slider) {
			int sliderWidth = this->fontMetrics().size(0, (*slider)->text).width();
			if(sliderWidth > this->sliderWidth)
				this->sliderWidth = sliderWidth;
		}
	}
	// Or a horizontal slider?
	else {
		for(QList<LevelSliderParameters *>::iterator slider = this->slider.begin(); slider != this->slider.end(); ++slider) {
			int sliderWidth = this->fontMetrics().size(0, (*slider)->text).height();
			if(sliderWidth > this->sliderWidth)
				this->sliderWidth = sliderWidth;
		}
	}
	
	return this->sliderWidth;
}

/// \brief Fix the value if it's outside the limits.
/// \param index The index of the slider who should be fixed.
/// \return 0 when ok, -1 on error, 1 when increased and 2 when decreased.
int LevelSlider::fixValue(int index) {
	if(index < 0 || index >= this->slider.count())
		return -1;
	
	double lowest = qMin(this->slider[index]->minimum, this->slider[index]->maximum);
	double highest = qMax(this->slider[index]->minimum, this->slider[index]->maximum);
	if(this->slider[index]->value < lowest) {
		this->slider[index]->value = lowest;
		return 1;
	}
	else if(this->slider[index]->value > highest) {
		this->slider[index]->value = highest;
		return 2;
	}
	return 0;
}
