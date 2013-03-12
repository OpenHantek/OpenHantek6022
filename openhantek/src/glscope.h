////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file glscope.h
/// \brief Declares the GlScope class.
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
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


#ifndef GLSCOPE_H
#define GLSCOPE_H


#include <QtOpenGL>


#include "glgenerator.h"
#include "dso.h"


class DataAnalyzer;
class DsoSettings;


////////////////////////////////////////////////////////////////////////////////
/// \class GlScope                                                     glscope.h
/// \brief OpenGL accelerated widget that displays the oscilloscope screen.
class GlScope : public QGLWidget {
	Q_OBJECT
	
	public:
		GlScope(DsoSettings *settings, QWidget* parent = 0);
		~GlScope();
		
		void setGenerator(GlGenerator *generator);
		void setZoomMode(bool zoomed);
	
	protected:
		void initializeGL();
		void paintGL();
		void resizeGL(int width, int height);
		
		void drawGrid();
	
	private:
		GlGenerator *generator;
		DsoSettings *settings;
		
		std::vector<GLfloat> vaMarker[2];
		bool zoomed;
};


#endif
