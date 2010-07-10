////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  colorbox.cpp
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


#include <QColorDialog>
#include <QFocusEvent>
#include <QPushButton>


#include "colorbox.h"


////////////////////////////////////////////////////////////////////////////////
// class ColorBox
/// \brief Initializes the widget.
/// \param color Initial color value.
/// \param parent The parent widget.
ColorBox::ColorBox(QColor color, QWidget *parent) : QPushButton(parent) {
	this->setColor(color);
	
	connect(this, SIGNAL(clicked()), this, SLOT(waitForColor()));
}

/// \brief Cleans up the widget.
ColorBox::~ColorBox() {
}

/// \brief Get the current color.
/// \return The current color as QColor.
const QColor ColorBox::getColor() {
	return this->color;
}

/// \brief Sets the color.
/// \param color The new color.
void ColorBox::setColor(QColor color) {
	this->color = color;
	this->setText(QString("#%1").arg((unsigned int) this->color.rgba(), 8, 16, QChar('0')));
	this->setPalette(QPalette(this->color));
	
	emit colorChanged(this->color);
}

/// \brief Wait for the color dialog and apply chosen color.
void ColorBox::waitForColor() {
	this->setFocus();
	this->setDown(true);
	
	QColor color = QColorDialog::getColor(this->color, this, 0, QColorDialog::ShowAlphaChannel);
	
	if(color.isValid())
		this->setColor(color);
}
