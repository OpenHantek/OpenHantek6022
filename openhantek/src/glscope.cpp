////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  glscope.cpp
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


#include <cmath>

#include <QColor>


#include "glscope.h"

#include "dataanalyzer.h"
#include "glgenerator.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// class GlScope
/// \brief Initializes the scope widget.
/// \param settings The settings that should be used.
/// \param parent The parent widget.
GlScope::GlScope(DsoSettings *settings, QWidget *parent) : QGLWidget(parent) {
	this->settings = settings;
	
	this->generator = 0;
	this->zoomed = false;
}

/// \brief Deletes OpenGL objects.
GlScope::~GlScope() {
}

/// \brief Initializes OpenGL output.
void GlScope::initializeGL() {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glPointSize(1);
	
	qglClearColor(this->settings->view.color.screen.background);
	
	glShadeModel(GL_SMOOTH/*GL_FLAT*/);
	glLineStipple(1, 0x3333);
	
	glEnableClientState(GL_VERTEX_ARRAY);
}

/// \brief Draw the graphs and the grid.
void GlScope::paintGL() {
	if(!this->isVisible())
		return;
	
	// Clear OpenGL buffer and configure settings
	glClear(GL_COLOR_BUFFER_BIT);
	glLineWidth(1);
	
	// Draw the graphs
	if(this->generator && this->generator->digitalPhosphorDepth > 0) {
		if(this->settings->view.antialiasing) {
			glEnable(GL_POINT_SMOOTH);
			glEnable(GL_LINE_SMOOTH);
			glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		}
		
		// Apply zoom settings via matrix transformation
		if(this->zoomed) {
			glPushMatrix();
			glScalef(DIVS_TIME / fabs(this->settings->scope.horizontal.marker[1] - this->settings->scope.horizontal.marker[0]), 1.0, 1.0);
			glTranslatef(-(this->settings->scope.horizontal.marker[0] + this->settings->scope.horizontal.marker[1]) / 2, 0.0, 0.0);
		}
		
		// Values we need for the fading of the digital phosphor
		double *fadingFactor = new double[this->generator->digitalPhosphorDepth];
		fadingFactor[0] = 100;
		double fadingRatio = pow(10.0, 2.0 / this->generator->digitalPhosphorDepth);
		for(unsigned int index = 1; index < this->generator->digitalPhosphorDepth; ++index)
			fadingFactor[index] = fadingFactor[index - 1] * fadingRatio;
		
		switch(this->settings->scope.horizontal.format) {
			case Dso::GRAPHFORMAT_TY:
				// Real and virtual channels
				for(int mode = Dso::CHANNELMODE_VOLTAGE; mode < Dso::CHANNELMODE_COUNT; ++mode) {
					for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
						if((mode == Dso::CHANNELMODE_VOLTAGE) ? this->settings->scope.voltage[channel].used : this->settings->scope.spectrum[channel].used) {
							// Draw graph for all available depths
							for(int index = this->generator->digitalPhosphorDepth - 1; index >= 0; index--) {
								if(!this->generator->vaChannel[mode][channel][index].empty()) {
									if(mode == Dso::CHANNELMODE_VOLTAGE)
										this->qglColor(this->settings->view.color.screen.voltage[channel].darker(fadingFactor[index]));
									else
										this->qglColor(this->settings->view.color.screen.spectrum[channel].darker(fadingFactor[index]));
									glVertexPointer(2, GL_FLOAT, 0, &this->generator->vaChannel[mode][channel][index].front());
									glDrawArrays((this->settings->view.interpolation == Dso::INTERPOLATION_OFF) ? GL_POINTS : GL_LINE_STRIP, 0, this->generator->vaChannel[mode][channel][index].size() / 2);
								}
							}
						}
					}
				}
				break;
			
			case Dso::GRAPHFORMAT_XY:
				// Real and virtual channels
				for(int channel = 0; channel < this->settings->scope.voltage.count() - 1; channel += 2) {
					if(this->settings->scope.voltage[channel].used) {
						// Draw graph for all available depths
						for(int index = this->generator->digitalPhosphorDepth - 1; index >= 0; index--) {
							if(!this->generator->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel][index].empty()) {
								this->qglColor(this->settings->view.color.screen.voltage[channel].darker(fadingFactor[index]));
								glVertexPointer(2, GL_FLOAT, 0, &this->generator->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel][index].front());
								glDrawArrays((this->settings->view.interpolation == Dso::INTERPOLATION_OFF) ? GL_POINTS : GL_LINE_STRIP, 0, this->generator->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel][index].size() / 2);
							}
						}
					}
				}
				break;
			
			default:
				break;
		}
		
		delete[] fadingFactor;
		
		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		
		if(this->zoomed)
			glPopMatrix();
	}
	
	if(!this->zoomed) {
		// Draw vertical lines at marker positions
		glEnable(GL_LINE_STIPPLE);
		this->qglColor(this->settings->view.color.screen.markers);
		
		for(int marker = 0; marker < MARKER_COUNT; ++marker) {
			if(this->vaMarker[marker].size() != 4) {
				this->vaMarker[marker].resize(2 * 2);
				this->vaMarker[marker][1] = -DIVS_VOLTAGE;
				this->vaMarker[marker][3] = DIVS_VOLTAGE;
			}
			
			this->vaMarker[marker][0] = this->settings->scope.horizontal.marker[marker];
			this->vaMarker[marker][2] = this->settings->scope.horizontal.marker[marker];
			
			glVertexPointer(2, GL_FLOAT, 0, &this->vaMarker[marker].front());
			glDrawArrays(GL_LINES, 0, this->vaMarker[marker].size() / 2);
		}
		
		glDisable(GL_LINE_STIPPLE);
	}
	
	// Draw grid
	this->drawGrid();
}

/// \brief Resize the widget.
/// \param width The new width of the widget.
/// \param height The new height of the widget.
void GlScope::resizeGL(int width, int height) {
	glViewport(0, 0, (GLint) width, (GLint) height);

	glMatrixMode(GL_PROJECTION);

	// Set axes to div-scale and apply correction for exact pixelization
	glLoadIdentity();
	GLdouble pixelizationWidthCorrection = (width > 0) ? (GLdouble) width / (width - 1) : 1;
	GLdouble pixelizationHeightCorrection = (height > 0) ? (GLdouble) height / (height - 1) : 1;
	glOrtho(-(DIVS_TIME / 2) * pixelizationWidthCorrection, (DIVS_TIME / 2) * pixelizationWidthCorrection, -(DIVS_VOLTAGE / 2) * pixelizationHeightCorrection, (DIVS_VOLTAGE / 2) * pixelizationHeightCorrection, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
}

/// \brief Set the generator that provides the vertex arrays.
/// \param generator Pointer to the GlGenerator class.
void GlScope::setGenerator(GlGenerator *generator) {
	if(this->generator)
		disconnect(this->generator, SIGNAL(graphsGenerated()), this, SLOT(updateGL()));
	this->generator = generator;
	connect(this->generator, SIGNAL(graphsGenerated()), this, SLOT(updateGL()));
}

/// \brief Set the zoom mode for this GlScope.
/// \param zoomed true magnifies the area between the markers.
void GlScope::setZoomMode(bool zoomed) {
	this->zoomed = zoomed;
}

/// \brief Draw the grid.
void GlScope::drawGrid() {
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_LINE_SMOOTH);
	
	// Grid
	this->qglColor(this->settings->view.color.screen.grid);
	glVertexPointer(2, GL_FLOAT, 0, &this->generator->vaGrid[0].front());
	glDrawArrays(GL_POINTS, 0, this->generator->vaGrid[0].size() / 2);
	// Axes
	this->qglColor(this->settings->view.color.screen.axes);
	glVertexPointer(2, GL_FLOAT, 0, &this->generator->vaGrid[1].front());
	glDrawArrays(GL_LINES, 0, this->generator->vaGrid[1].size() / 2);
	// Border
	this->qglColor(this->settings->view.color.screen.border);
	glVertexPointer(2, GL_FLOAT, 0, &this->generator->vaGrid[2].front());
	glDrawArrays(GL_LINE_LOOP, 0, this->generator->vaGrid[2].size() / 2);
}
