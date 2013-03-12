////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  glgenerator.cpp
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


#include <QGLWidget>
#include <QMutex>


#include "glgenerator.h"

#include "dataanalyzer.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// class GlGenerator
/// \brief Initializes the scope widget.
/// \param settings The target settings object.
/// \param parent The parent widget.
GlGenerator::GlGenerator(DsoSettings *settings, QObject *parent) : QObject(parent) {
	this->settings = settings;
	
	this->dataAnalyzer = 0;
	this->digitalPhosphorDepth = 0;
	
	this->generateGrid();
}

/// \brief Deletes OpenGL objects.
GlGenerator::~GlGenerator() {
	/// \todo Clean up vaChannel
}

/// \brief Set the data analyzer whose data will be drawn.
/// \param dataAnalyzer Pointer to the DataAnalyzer class.
void GlGenerator::setDataAnalyzer(DataAnalyzer *dataAnalyzer) {
	if(this->dataAnalyzer)
		disconnect(this->dataAnalyzer, SIGNAL(finished()), this, SLOT(generateGraphs()));
	this->dataAnalyzer = dataAnalyzer;
	connect(this->dataAnalyzer, SIGNAL(finished()), this, SLOT(generateGraphs()));
}

/// \brief Prepare arrays for drawing the data we get from the data analyzer.
void GlGenerator::generateGraphs() {
	if(!this->dataAnalyzer)
		return;
	
	// Adapt the number of graphs
	for(int mode = Dso::CHANNELMODE_VOLTAGE; mode < Dso::CHANNELMODE_COUNT; ++mode)
		this->vaChannel[mode].resize(this->settings->scope.voltage.count());
	
	// Set digital phosphor depth to one if we don't use it
	if(this->settings->view.digitalPhosphor)
		this->digitalPhosphorDepth = this->settings->view.digitalPhosphorDepth;
	else
		this->digitalPhosphorDepth = 1;
	
	// Handle all digital phosphor related list manipulations
	for(int mode = Dso::CHANNELMODE_VOLTAGE; mode < Dso::CHANNELMODE_COUNT; ++mode) {
		for(unsigned int channel = 0; channel < this->vaChannel[mode].size(); ++channel) {
			// Move the last list element to the front
			this->vaChannel[mode][channel].push_front(std::vector<GLfloat>());
			
			// Resize lists for vector array to fit the digital phosphor depth
			this->vaChannel[mode][channel].resize(this->digitalPhosphorDepth);
		}
	}
	
	this->dataAnalyzer->mutex()->lock();
	
	switch(this->settings->scope.horizontal.format) {
		case Dso::GRAPHFORMAT_TY:
			// Add graphs for channels
			for(int mode = Dso::CHANNELMODE_VOLTAGE; mode < Dso::CHANNELMODE_COUNT; ++mode) {
				for(int channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
					// Check if this channel is used and available at the data analyzer
					if(((mode == Dso::CHANNELMODE_VOLTAGE) ? this->settings->scope.voltage[channel].used : this->settings->scope.spectrum[channel].used) && this->dataAnalyzer->data(channel) && !this->dataAnalyzer->data(channel)->samples.voltage.sample.empty()) {
						// Check if the sample count has changed
						unsigned int sampleCount = (mode == Dso::CHANNELMODE_VOLTAGE) ? this->dataAnalyzer->data(channel)->samples.voltage.sample.size() : this->dataAnalyzer->data(channel)->samples.spectrum.sample.size();
						unsigned int neededSize = sampleCount * 2;
						for(unsigned int index = 0; index < this->digitalPhosphorDepth; ++index) {
							if(this->vaChannel[mode][channel][index].size() != neededSize)
								this->vaChannel[mode][channel][index].clear(); // Something was changed, drop old traces
						}
						
						// Set size directly to avoid reallocations
						this->vaChannel[mode][channel].front().resize(neededSize);
						
						// Iterator to data for direct access
						std::vector<GLfloat>::iterator glIterator = this->vaChannel[mode][channel].front().begin();
						
						// What's the horizontal distance between sampling points?
						double horizontalFactor;
						if(mode == Dso::CHANNELMODE_VOLTAGE)
							horizontalFactor = this->dataAnalyzer->data(channel)->samples.voltage.interval / this->settings->scope.horizontal.timebase;
						else
							horizontalFactor = this->dataAnalyzer->data(channel)->samples.spectrum.interval / this->settings->scope.horizontal.frequencybase;
						
						// Fill vector array
						if(mode == Dso::CHANNELMODE_VOLTAGE) {
							std::vector<double>::const_iterator dataIterator = this->dataAnalyzer->data(channel)->samples.voltage.sample.begin();
							const double gain = this->settings->scope.voltage[channel].gain;
							const double offset = this->settings->scope.voltage[channel].offset;
							
							for(unsigned int position = 0; position < sampleCount; ++position) {
								*(glIterator++) = position * horizontalFactor - DIVS_TIME / 2;
								*(glIterator++) = *(dataIterator++) / gain + offset;
							}
						}
						else {
							std::vector<double>::const_iterator dataIterator = this->dataAnalyzer->data(channel)->samples.spectrum.sample.begin();
							const double magnitude = this->settings->scope.spectrum[channel].magnitude;
							const double offset = this->settings->scope.spectrum[channel].offset;
							
							for(unsigned int position = 0; position < sampleCount; ++position) {
								*(glIterator++) = position * horizontalFactor - DIVS_TIME / 2;
								*(glIterator++) = *(dataIterator++) / magnitude + offset;
							}
						}
					}
					else {
						// Delete all vector arrays
						for(unsigned int index = 0; index < this->digitalPhosphorDepth; ++index)
							this->vaChannel[mode][channel][index].clear();
					}
				}
			}
			break;
			
		case Dso::GRAPHFORMAT_XY:
			for(int channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
				// For even channel numbers check if this channel is used and this and the following channel are available at the data analyzer
				if(channel % 2 == 0 && channel + 1 < this->settings->scope.voltage.size() && this->settings->scope.voltage[channel].used && this->dataAnalyzer->data(channel) && !this->dataAnalyzer->data(channel)->samples.voltage.sample.empty() && this->dataAnalyzer->data(channel + 1) && !this->dataAnalyzer->data(channel + 1)->samples.voltage.sample.empty()) {
					// Check if the sample count has changed
					const unsigned int sampleCount = qMin(this->dataAnalyzer->data(channel)->samples.voltage.sample.size(), this->dataAnalyzer->data(channel + 1)->samples.voltage.sample.size());
					const unsigned int neededSize = sampleCount * 2;
					for(unsigned int index = 0; index < this->digitalPhosphorDepth; ++index) {
						if(this->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel][index].size() != neededSize)
							this->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel][index].clear(); // Something was changed, drop old traces
					}
					
					// Set size directly to avoid reallocations
					this->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel].front().resize(neededSize);
					
					// Iterator to data for direct access
					std::vector<GLfloat>::iterator glIterator = this->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel].front().begin();
					
					// Fill vector array
					unsigned int xChannel = channel;
					unsigned int yChannel = channel + 1;
					std::vector<double>::const_iterator xIterator = this->dataAnalyzer->data(xChannel)->samples.voltage.sample.begin();
					std::vector<double>::const_iterator yIterator = this->dataAnalyzer->data(yChannel)->samples.voltage.sample.begin();
					const double xGain = this->settings->scope.voltage[xChannel].gain;
					const double yGain = this->settings->scope.voltage[yChannel].gain;
					const double xOffset = this->settings->scope.voltage[xChannel].offset;
					const double yOffset = this->settings->scope.voltage[yChannel].offset;
					
					for(unsigned int position = 0; position < sampleCount; ++position) {
						*(glIterator++) = *(xIterator++) / xGain + xOffset;
						*(glIterator++) = *(yIterator++) / yGain + yOffset;
					}
				}
				else {
					// Delete all vector arrays
					for(unsigned int index = 0; index < this->digitalPhosphorDepth; ++index)
						this->vaChannel[Dso::CHANNELMODE_VOLTAGE][channel][index].clear();
				}
				
				// Delete all spectrum graphs
				for(unsigned int index = 0; index < this->digitalPhosphorDepth; ++index)
					this->vaChannel[Dso::CHANNELMODE_SPECTRUM][channel][index].clear();
			}
			break;
		
		default:
			break;
	}
	
	this->dataAnalyzer->mutex()->unlock();
	
	emit graphsGenerated();
}

/// \brief Create the needed OpenGL vertex arrays for the grid.
void GlGenerator::generateGrid() {
	// Grid
	this->vaGrid[0].resize(((DIVS_TIME * DIVS_SUB - 2) * (DIVS_VOLTAGE - 2) + (DIVS_VOLTAGE * DIVS_SUB - 2) * (DIVS_TIME - 2) - ((DIVS_TIME - 2) * (DIVS_VOLTAGE - 2))) * 2);
	std::vector<GLfloat>::iterator glIterator = this->vaGrid[0].begin();
	// Draw vertical lines
	for(int div = 1; div < DIVS_TIME / 2; ++div) {
		for(int dot = 1; dot < DIVS_VOLTAGE / 2 * DIVS_SUB; ++dot) {
			float dotPosition = (float) dot / DIVS_SUB;
			*(glIterator++) = -div;
			*(glIterator++) = -dotPosition;
			*(glIterator++) = -div;
			*(glIterator++) = dotPosition;
			*(glIterator++) = div;
			*(glIterator++) = -dotPosition;
			*(glIterator++) = div;
			*(glIterator++) = dotPosition;
		}
	}
	// Draw horizontal lines
	for(int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
		for(int dot = 1; dot < DIVS_TIME / 2 * DIVS_SUB; ++dot) {
			if(dot % DIVS_SUB == 0)
				continue;                   // Already done by vertical lines
			float dotPosition = (float) dot / DIVS_SUB;
			*(glIterator++) = -dotPosition;
			*(glIterator++) = -div;
			*(glIterator++) = dotPosition;
			*(glIterator++) = -div;
			*(glIterator++) = -dotPosition;
			*(glIterator++) = div;
			*(glIterator++) = dotPosition;
			*(glIterator++) = div;
		}
	}
	
	// Axes
	this->vaGrid[1].resize((2 + (DIVS_TIME * DIVS_SUB - 2) + (DIVS_VOLTAGE * DIVS_SUB - 2)) * 4);
	glIterator = this->vaGrid[1].begin();
	// Horizontal axis
	*(glIterator++) = -DIVS_TIME / 2;
	*(glIterator++) = 0;
	*(glIterator++) = DIVS_TIME / 2;
	*(glIterator++) = 0;
	// Vertical axis
	*(glIterator++) = 0;
	*(glIterator++) = -DIVS_VOLTAGE / 2;
	*(glIterator++) = 0;
	*(glIterator++) = DIVS_VOLTAGE / 2;
	// Subdiv lines on horizontal axis
	for(int line = 1; line < DIVS_TIME / 2 * DIVS_SUB; ++line) {
		float linePosition = (float) line / DIVS_SUB;
		*(glIterator++) = linePosition;
		*(glIterator++) = -0.05;
		*(glIterator++) = linePosition;
		*(glIterator++) = 0.05;
		*(glIterator++) = -linePosition;
		*(glIterator++) = -0.05;
		*(glIterator++) = -linePosition;
		*(glIterator++) = 0.05;
	}
	// Subdiv lines on vertical axis
	for(int line = 1; line < DIVS_VOLTAGE / 2 * DIVS_SUB; ++line) {
		float linePosition = (float) line / DIVS_SUB;
		*(glIterator++) = -0.05;
		*(glIterator++) = linePosition;
		*(glIterator++) = 0.05;
		*(glIterator++) = linePosition;
		*(glIterator++) = -0.05;
		*(glIterator++) = -linePosition;
		*(glIterator++) = 0.05;
		*(glIterator++) = -linePosition;
	}
	
	// Border
	this->vaGrid[2].resize(4 * 2);
	glIterator = this->vaGrid[2].begin();
	*(glIterator++) = -DIVS_TIME / 2;
	*(glIterator++) = -DIVS_VOLTAGE / 2;
	*(glIterator++) = DIVS_TIME / 2;
	*(glIterator++) = -DIVS_VOLTAGE / 2;
	*(glIterator++) = DIVS_TIME / 2;
	*(glIterator++) = DIVS_VOLTAGE / 2;
	*(glIterator++) = -DIVS_TIME / 2;
	*(glIterator++) = DIVS_VOLTAGE / 2;
}
