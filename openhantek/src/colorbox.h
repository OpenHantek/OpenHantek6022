////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file colorbox.h
/// \brief Declares the ColorBox class.
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


#ifndef COLORBOX_H
#define COLORBOX_H


#include <QColor>
#include <QPushButton>


////////////////////////////////////////////////////////////////////////////////
/// \class ColorBox                                                   colorbox.h
/// \brief A widget for the selection of a color.
class ColorBox : public QPushButton {
	Q_OBJECT
	
	public:
		ColorBox(QColor color, QWidget *parent = 0);
		~ColorBox();
		
		const QColor getColor();
	
	public slots:
		void setColor(QColor color);
		void waitForColor();
	
	private:
		QColor color;
	
	signals:
		void colorChanged(QColor color); ///< The color has been changed
};


#endif
