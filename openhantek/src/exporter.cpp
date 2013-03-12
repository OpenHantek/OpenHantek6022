////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  exporter.cpp
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

#include <QFile>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextStream>


#include "exporter.h"

#include "dataanalyzer.h"
#include "dso.h"
#include "glgenerator.h"
#include "helper.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// class HorizontalDock
/// \brief Initializes the printer object.
Exporter::Exporter(DsoSettings *settings, DataAnalyzer *dataAnalyzer, QWidget *parent) : QObject(parent) {
	this->settings = settings;
	this->dataAnalyzer = dataAnalyzer;
	
	this->format = EXPORT_FORMAT_PRINTER;
}

/// \brief Cleans up everything.
Exporter::~Exporter() {
}

/// \brief Set the filename of the output file (Not used for printing).
void Exporter::setFilename(QString filename) {
	if(!filename.isEmpty())
		this->filename = filename;
}

/// \brief Set the output format.
void Exporter::setFormat(ExportFormat format) {
	if(format >= EXPORT_FORMAT_PRINTER && format <= EXPORT_FORMAT_CSV)
		this->format = format;
}

/// \brief Print the document (May be a file too)
bool Exporter::doExport() {
	if(this->format < EXPORT_FORMAT_CSV) {
		// Choose the color values we need
		DsoSettingsColorValues *colorValues;
		if(this->format == EXPORT_FORMAT_IMAGE && this->settings->view.screenColorImages)
			colorValues = &(this->settings->view.color.screen);
		else
			colorValues = &(this->settings->view.color.print);
		
		QPaintDevice *paintDevice;
		
		if(this->format < EXPORT_FORMAT_IMAGE) {
			// We need a QPrinter for printing, pdf- and ps-export
			paintDevice = new QPrinter(QPrinter::HighResolution);
			static_cast<QPrinter *>(paintDevice)->setOrientation(this->settings->view.zoom ? QPrinter::Portrait : QPrinter::Landscape);
			static_cast<QPrinter *>(paintDevice)->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);
			
			if(this->format == EXPORT_FORMAT_PRINTER) {
				// Show the printing dialog
				QPrintDialog dialog(static_cast<QPrinter *>(paintDevice), static_cast<QWidget *>(this->parent()));
				dialog.setWindowTitle(tr("Print oscillograph"));
				if(dialog.exec() != QDialog::Accepted) {
					delete paintDevice;
					return false;
				}
			}
			else {
				// Configure the QPrinter
				static_cast<QPrinter *>(paintDevice)->setOutputFileName(this->filename);
				static_cast<QPrinter *>(paintDevice)->setOutputFormat((this->format == EXPORT_FORMAT_PDF) ? QPrinter::PdfFormat : QPrinter::PostScriptFormat);
			}
		}
		else {
			// We need a QPixmap for image-export
			paintDevice = new QPixmap(this->settings->options.imageSize);
			static_cast<QPixmap *>(paintDevice)->fill(colorValues->background);
		}
		
		// Create a painter for our device
		QPainter painter(paintDevice);
		
		// Get line height
		QFont font;
		QFontMetrics fontMetrics(font, paintDevice);
		double lineHeight = fontMetrics.height();
		
		painter.setBrush(Qt::SolidPattern);
		
		this->dataAnalyzer->mutex()->lock();
		
		// Draw the settings table
		double stretchBase = (double) (paintDevice->width() - lineHeight * 10) / 4;
		
		// Print trigger details
		painter.setPen(colorValues->voltage[this->settings->scope.trigger.source]);
		QString levelString = Helper::valueToString(this->settings->scope.voltage[this->settings->scope.trigger.source].trigger, Helper::UNIT_VOLTS, 3);
		QString pretriggerString = tr("%L1%").arg((int) (this->settings->scope.trigger.position * 100 + 0.5));
		painter.drawText(QRectF(0, 0, lineHeight * 10, lineHeight), tr("%1  %2  %3  %4").arg(this->settings->scope.voltage[this->settings->scope.trigger.source].name, Dso::slopeString(this->settings->scope.trigger.slope), levelString, pretriggerString));
		
		// Print sample count
		painter.setPen(colorValues->text);
		painter.drawText(QRectF(lineHeight * 10, 0, stretchBase, lineHeight), tr("%1 S").arg(this->dataAnalyzer->sampleCount()), QTextOption(Qt::AlignRight));
		// Print samplerate
		painter.drawText(QRectF(lineHeight * 10 + stretchBase, 0, stretchBase, lineHeight), Helper::valueToString(this->settings->scope.horizontal.samplerate, Helper::UNIT_SAMPLES) + tr("/s"), QTextOption(Qt::AlignRight));
		// Print timebase
		painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, 0, stretchBase, lineHeight), Helper::valueToString(this->settings->scope.horizontal.timebase, Helper::UNIT_SECONDS, 0) + tr("/div"), QTextOption(Qt::AlignRight));
		// Print frequencybase
		painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, 0, stretchBase, lineHeight), Helper::valueToString(this->settings->scope.horizontal.frequencybase, Helper::UNIT_HERTZ, 0) + tr("/div"), QTextOption(Qt::AlignRight));
		
		// Draw the measurement table
		stretchBase = (double) (paintDevice->width() - lineHeight * 6) / 10;
		int channelCount = 0;
		for(int channel = this->settings->scope.voltage.count() - 1; channel >= 0; channel--) {
			if((this->settings->scope.voltage[channel].used || this->settings->scope.spectrum[channel].used) && this->dataAnalyzer->data(channel)) {
				++channelCount;
				double top = (double) paintDevice->height() - channelCount * lineHeight;
				
				// Print label
				painter.setPen(colorValues->voltage[channel]);
				painter.drawText(QRectF(0, top, lineHeight * 4, lineHeight), this->settings->scope.voltage[channel].name);
				// Print coupling/math mode
				if((unsigned int) channel < this->settings->scope.physicalChannels)
					painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight), Dso::couplingString((Dso::Coupling) this->settings->scope.voltage[channel].misc));
				else
					painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight), Dso::mathModeString((Dso::MathMode) this->settings->scope.voltage[channel].misc));
				
				// Print voltage gain
				painter.drawText(QRectF(lineHeight * 6, top, stretchBase * 2, lineHeight), Helper::valueToString(this->settings->scope.voltage[channel].gain, Helper::UNIT_VOLTS, 0) + tr("/div"), QTextOption(Qt::AlignRight));
				// Print spectrum magnitude
				painter.setPen(colorValues->spectrum[channel]);
				painter.drawText(QRectF(lineHeight * 6 + stretchBase * 2, top, stretchBase * 2, lineHeight), Helper::valueToString(this->settings->scope.spectrum[channel].magnitude, Helper::UNIT_DECIBEL, 0) + tr("/div"), QTextOption(Qt::AlignRight));
				
				// Amplitude string representation (4 significant digits)
				painter.setPen(colorValues->text);
				painter.drawText(QRectF(lineHeight * 6 + stretchBase * 4, top, stretchBase * 3, lineHeight), Helper::valueToString(this->dataAnalyzer->data(channel)->amplitude, Helper::UNIT_VOLTS, 4), QTextOption(Qt::AlignRight));
				// Frequency string representation (5 significant digits)
				painter.drawText(QRectF(lineHeight * 6 + stretchBase * 7, top, stretchBase * 3, lineHeight), Helper::valueToString(this->dataAnalyzer->data(channel)->frequency, Helper::UNIT_HERTZ, 5), QTextOption(Qt::AlignRight));
			}
		}
		
		// Draw the marker table
		double scopeHeight;
		stretchBase = (double) (paintDevice->width() - lineHeight * 10) / 4;
		painter.setPen(colorValues->text);
		
		// Calculate variables needed for zoomed scope
		double divs = fabs(this->settings->scope.horizontal.marker[1] - this->settings->scope.horizontal.marker[0]);
		double time = divs * this->settings->scope.horizontal.timebase;
		double zoomFactor = DIVS_TIME / divs;
		double zoomOffset = (this->settings->scope.horizontal.marker[0] + this->settings->scope.horizontal.marker[1]) / 2;
		
		if(this->settings->view.zoom) {
			scopeHeight = (double) (paintDevice->height() - (channelCount + 5) * lineHeight) / 2;
			double top = 2.5 * lineHeight + scopeHeight;
			
			painter.drawText(QRectF(0, top, stretchBase, lineHeight), tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3));
			
			painter.drawText(QRectF(lineHeight * 10, top, stretchBase, lineHeight), Helper::valueToString(time, Helper::UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
			painter.drawText(QRectF(lineHeight * 10 + stretchBase, top, stretchBase, lineHeight), Helper::valueToString(1.0 / time, Helper::UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));
			
			painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase, lineHeight), Helper::valueToString(time / DIVS_TIME, Helper::UNIT_SECONDS, 3) + tr("/div"), QTextOption(Qt::AlignRight));
			painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, top, stretchBase, lineHeight), Helper::valueToString(divs  * this->settings->scope.horizontal.frequencybase / DIVS_TIME, Helper::UNIT_HERTZ, 3) + tr("/div"), QTextOption(Qt::AlignRight));
		}
		else {
			scopeHeight = (double) paintDevice->height() - (channelCount + 4) * lineHeight;
			double top = 2.5 * lineHeight + scopeHeight;
			
			painter.drawText(QRectF(0, top, stretchBase, lineHeight), tr("Marker 1/2"));
			
			painter.drawText(QRectF(lineHeight * 10, top, stretchBase * 2, lineHeight), Helper::valueToString(time, Helper::UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
			painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase * 2, lineHeight), Helper::valueToString(1.0 / time, Helper::UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));
		}
		
		// Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
		painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE, (double) (paintDevice->width() - 1) / 2, (scopeHeight - 1) / 2 + lineHeight * 1.5), false);
		
		// Draw the graphs
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(Qt::NoBrush);
		
		for(int zoomed = 0; zoomed < (this->settings->view.zoom ? 2 : 1); ++zoomed) {
			switch(this->settings->scope.horizontal.format) {
				case Dso::GRAPHFORMAT_TY:
					// Add graphs for channels
					for(int channel = 0 ; channel < this->settings->scope.voltage.count(); ++channel) {
						if(this->settings->scope.voltage[channel].used && this->dataAnalyzer->data(channel)) {
							painter.setPen(colorValues->voltage[channel]);
							
							// What's the horizontal distance between sampling points?
							double horizontalFactor = this->dataAnalyzer->data(channel)->samples.voltage.interval / this->settings->scope.horizontal.timebase;
							// How many samples are visible?
							double centerPosition, centerOffset;
							if(zoomed) {
								centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
								centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
							}
							else {
								centerPosition = DIVS_TIME / 2 / horizontalFactor;
								centerOffset = DIVS_TIME / horizontalFactor / 2;
							}
							unsigned int firstPosition = qMax((int) (centerPosition - centerOffset), 0);
							unsigned int lastPosition = qMin((int) (centerPosition + centerOffset), (int) this->dataAnalyzer->data(channel)->samples.voltage.sample.size() - 1);
							
							// Draw graph
							QPointF *graph = new QPointF[lastPosition - firstPosition + 1];
							
							for(unsigned int position = firstPosition; position <= lastPosition; ++position)
								graph[position - firstPosition] = QPointF(position * horizontalFactor - DIVS_TIME / 2, this->dataAnalyzer->data(channel)->samples.voltage.sample[position] / this->settings->scope.voltage[channel].gain + this->settings->scope.voltage[channel].offset);
							
							painter.drawPolyline(graph, lastPosition - firstPosition + 1);
							delete[] graph;
						}
					}
				
					// Add spectrum graphs
					for (int channel = 0; channel < this->settings->scope.spectrum.count(); ++channel) {
						if(this->settings->scope.spectrum[channel].used && this->dataAnalyzer->data(channel)) {
							painter.setPen(colorValues->spectrum[channel]);
							
							// What's the horizontal distance between sampling points?
							double horizontalFactor = this->dataAnalyzer->data(channel)->samples.spectrum.interval / this->settings->scope.horizontal.frequencybase;
							// How many samples are visible?
							double centerPosition, centerOffset;
							if(zoomed) {
								centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
								centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
							}
							else {
								centerPosition = DIVS_TIME / 2 / horizontalFactor;
								centerOffset = DIVS_TIME / horizontalFactor / 2;
							}
							unsigned int firstPosition = qMax((int) (centerPosition - centerOffset), 0);
							unsigned int lastPosition = qMin((int) (centerPosition + centerOffset), (int) this->dataAnalyzer->data(channel)->samples.spectrum.sample.size() - 1);
							
							// Draw graph
							QPointF *graph = new QPointF[lastPosition - firstPosition + 1];
							
							for(unsigned int position = firstPosition; position <= lastPosition; ++position)
								graph[position - firstPosition] = QPointF(position * horizontalFactor - DIVS_TIME / 2, this->dataAnalyzer->data(channel)->samples.spectrum.sample[position] / this->settings->scope.spectrum[channel].magnitude + this->settings->scope.spectrum[channel].offset);
							
							painter.drawPolyline(graph, lastPosition - firstPosition + 1);
							delete[] graph;
						}
					}
					break;
					
				case Dso::GRAPHFORMAT_XY:
					break;
				
				default:
					break;
			}
			
			// Set DIVS_TIME / zoomFactor x DIVS_VOLTAGE matrix for zoomed oscillograph
			painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME * zoomFactor, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE, (double) (paintDevice->width() - 1) / 2 - zoomOffset * zoomFactor * (paintDevice->width() - 1) / DIVS_TIME, (scopeHeight - 1) * 1.5 + lineHeight * 4), false);
		}
		
		this->dataAnalyzer->mutex()->unlock();
		
		// Draw grids
		painter.setRenderHint(QPainter::Antialiasing, false);
		for(int zoomed = 0; zoomed < (this->settings->view.zoom ? 2 : 1); ++zoomed) {
			// Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
			painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE, (double) (paintDevice->width() - 1) / 2, (scopeHeight - 1) * (zoomed + 0.5) + lineHeight * 1.5 + lineHeight * 2.5 * zoomed), false);
			
			// Grid lines
			painter.setPen(colorValues->grid);
			
			if(this->format < EXPORT_FORMAT_IMAGE) {
				// Draw vertical lines
				for(int div = 1; div < DIVS_TIME / 2; ++div) {
					for(int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
						painter.drawLine(QPointF((double) -div - 0.02, (double) -dot / 5), QPointF((double) -div + 0.02, (double) -dot / 5));
						painter.drawLine(QPointF((double) -div - 0.02, (double) dot / 5), QPointF((double) -div + 0.02, (double) dot / 5));
						painter.drawLine(QPointF((double) div - 0.02, (double) -dot / 5), QPointF((double) div + 0.02, (double) -dot / 5));
						painter.drawLine(QPointF((double) div - 0.02, (double) dot / 5), QPointF((double) div + 0.02, (double) dot / 5));
					}
				}
				// Draw horizontal lines
				for(int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
					for(int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
						painter.drawLine(QPointF((double) -dot / 5, (double) -div - 0.02), QPointF((double) -dot / 5, (double) -div + 0.02));
						painter.drawLine(QPointF((double) dot / 5, (double) -div - 0.02), QPointF((double) dot / 5, (double) -div + 0.02));
						painter.drawLine(QPointF((double) -dot / 5, (double) div - 0.02), QPointF((double) -dot / 5, (double) div + 0.02));
						painter.drawLine(QPointF((double) dot / 5, (double) div - 0.02), QPointF((double) dot / 5, (double) div + 0.02));
					}
				}
			}
			else {
				// Draw vertical lines
				for(int div = 1; div < DIVS_TIME / 2; ++div) {
					for(int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
						painter.drawPoint(QPointF(-div, (double) -dot / 5));
						painter.drawPoint(QPointF(-div, (double) dot / 5));
						painter.drawPoint(QPointF(div, (double) -dot / 5));
						painter.drawPoint(QPointF(div, (double) dot / 5));
					}
				}
				// Draw horizontal lines
				for(int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
					for(int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
						if(dot % 5 == 0)
							continue;                       // Already done by vertical lines
						painter.drawPoint(QPointF((double) -dot / 5, -div));
						painter.drawPoint(QPointF((double) dot / 5, -div));
						painter.drawPoint(QPointF((double) -dot / 5, div));
						painter.drawPoint(QPointF((double) dot / 5, div));
					}
				}
			}
				
			// Axes
			painter.setPen(colorValues->axes);
			painter.drawLine(QPointF(-DIVS_TIME / 2, 0), QPointF(DIVS_TIME / 2, 0));
			painter.drawLine(QPointF(0, -DIVS_VOLTAGE / 2), QPointF(0, DIVS_VOLTAGE / 2));
			for(double div = 0.2; div <= DIVS_TIME / 2; div += 0.2) {
				painter.drawLine(QPointF(div, -0.05), QPointF(div, 0.05));
				painter.drawLine(QPointF(-div, -0.05), QPointF(-div, 0.05));
			}
			for(double div = 0.2; div <= DIVS_VOLTAGE / 2; div += 0.2) {
				painter.drawLine(QPointF(-0.05, div), QPointF(0.05, div));
				painter.drawLine(QPointF(-0.05, -div), QPointF(0.05, -div));
			}
			
			// Borders
			painter.setPen(colorValues->border);
			painter.drawRect(QRectF(-DIVS_TIME / 2, -DIVS_VOLTAGE / 2, DIVS_TIME, DIVS_VOLTAGE));
		}
		
		painter.end();
		
		if(this->format == EXPORT_FORMAT_IMAGE)
			static_cast<QPixmap *>(paintDevice)->save(this->filename);
		
		delete paintDevice;
		
		return true;
	}
	else {
		QFile csvFile(this->filename);
		if(!csvFile.open(QIODevice::WriteOnly | QIODevice::Text))
			return false;
		
		QTextStream csvStream(&csvFile);
		
		for(int channel = 0 ; channel < this->settings->scope.voltage.count(); ++channel) {
			if(this->dataAnalyzer->data(channel)) {
				if(this->settings->scope.voltage[channel].used) {
					// Start with channel name and the sample interval
					csvStream << "\"" << this->settings->scope.voltage[channel].name << "\"," << this->dataAnalyzer->data(channel)->samples.voltage.interval;
					
					// And now all sample values in volts
					for(unsigned int position = 0; position < this->dataAnalyzer->data(channel)->samples.voltage.sample.size(); ++position)
						csvStream << "," << this->dataAnalyzer->data(channel)->samples.voltage.sample[position];
					
					// Finally a newline
					csvStream << '\n';
				}
				
				if(this->settings->scope.spectrum[channel].used) {
					// Start with channel name and the sample interval
					csvStream << "\"" << this->settings->scope.spectrum[channel].name << "\"," << this->dataAnalyzer->data(channel)->samples.spectrum.interval;
					
					// And now all magnitudes in dB
					for(unsigned int position = 0; position < this->dataAnalyzer->data(channel)->samples.spectrum.sample.size(); ++position)
						csvStream << "," << this->dataAnalyzer->data(channel)->samples.spectrum.sample[position];
					
					// Finally a newline
					csvStream << '\n';
				}
			}
		}
		
		csvFile.close();
		
		return true;
	}
}
