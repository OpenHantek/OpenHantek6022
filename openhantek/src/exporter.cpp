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


#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>


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
	this->size = QSize(640, 480);
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
	if(format >= EXPORT_FORMAT_PRINTER && format <= EXPORT_FORMAT_IMAGE)
		this->format = format;
}

/// \brief Set the size for the output image.
void Exporter::setSize(QSize size) {
	if(size.isValid())
		this->size = size;
}

/// \brief Print the document (May be a file too)
bool Exporter::doExport() {
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
		((QPrinter *) paintDevice)->setOrientation(this->settings->view.zoom ? QPrinter::Portrait : QPrinter::Landscape);
		((QPrinter *) paintDevice)->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);
		
		if(this->format == EXPORT_FORMAT_PRINTER) {
			// Show the printing dialog
			QPrintDialog *dialog = new QPrintDialog((QPrinter *) paintDevice, (QWidget *) this->parent());
			dialog->setWindowTitle(tr("Print oscillograph"));
			if(dialog->exec() != QDialog::Accepted)
				return false;
		}
		else {
			// Configure the QPrinter
			((QPrinter *) paintDevice)->setOutputFileName(this->filename);
			((QPrinter *) paintDevice)->setOutputFormat((this->format == EXPORT_FORMAT_PDF) ? QPrinter::PdfFormat : QPrinter::PostScriptFormat);
		}
	}
	else {
		// We need a QPixmap for image-export
		paintDevice = new QPixmap(this->size);
		((QPixmap *) paintDevice)->fill(colorValues->background);
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
	for(int channel = 0; channel < this->settings->scope.voltage.count(); channel++) {
		if(this->settings->scope.voltage[channel].used || this->settings->scope.spectrum[channel].used) {
			channelCount++;
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
	
	// Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
	double screenHeight = (double) paintDevice->height() - (channelCount + 3) * lineHeight;
	painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME, 0, 0, -(screenHeight - 1) / DIVS_VOLTAGE, (double) (paintDevice->width() - 1) / 2, (screenHeight - 1) / 2 + lineHeight * 2), false);
	
	// Draw the graphs
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(Qt::NoBrush);
	
	switch(this->settings->scope.horizontal.format) {
		case Dso::GRAPHFORMAT_TY:
			// Add graphs for channels
			for(int channel = 0 ; channel < this->settings->scope.voltage.count(); channel++) {
				if(this->settings->scope.voltage[channel].used) {
					painter.setPen(colorValues->voltage[channel]);
					
					// What's the horizontal distance between sampling points?
					double horizontalFactor = this->dataAnalyzer->data(channel)->samples.voltage.interval / this->settings->scope.horizontal.timebase;
					// How many samples are visible?
					unsigned int lastPosition = DIVS_TIME / horizontalFactor;
					if(lastPosition >= this->dataAnalyzer->data(channel)->samples.voltage.count)
						lastPosition = this->dataAnalyzer->data(channel)->samples.voltage.count - 1;
					
					// Draw graph
					QPointF *graph = new QPointF[lastPosition + 1];
					for(unsigned int position = 0; position <= lastPosition; position++)
						graph[position] = QPointF(position * horizontalFactor - DIVS_TIME / 2, this->dataAnalyzer->data(channel)->samples.voltage.sample[position] / this->settings->scope.voltage[channel].gain + this->settings->scope.voltage[channel].offset);
					painter.drawPolyline(graph, lastPosition + 1);
				}
			}
		
			// Add spectrum graphs
			for (int channel = 0; channel < this->settings->scope.spectrum.count(); channel++) {
				if(this->settings->scope.spectrum[channel].used) {
					painter.setPen(colorValues->spectrum[channel]);
					
					// What's the horizontal distance between sampling points?
					double horizontalFactor = this->dataAnalyzer->data(channel)->samples.spectrum.interval / this->settings->scope.horizontal.frequencybase;
					// How many samples are visible?
					unsigned int lastPosition = DIVS_TIME / horizontalFactor;
					if(lastPosition >= this->dataAnalyzer->data(channel)->samples.spectrum.count)
						lastPosition = this->dataAnalyzer->data(channel)->samples.spectrum.count - 1;
					
					// Draw graph
					QPointF *graph = new QPointF[lastPosition + 1];
					for(unsigned int position = 0; position <= lastPosition; position++)
						graph[position] = QPointF(position * horizontalFactor - DIVS_TIME / 2, this->dataAnalyzer->data(channel)->samples.spectrum.sample[position] / this->settings->scope.spectrum[channel].magnitude + this->settings->scope.spectrum[channel].offset);
					painter.drawPolyline(graph, lastPosition + 1);
				}
			}
			break;
			
		case Dso::GRAPHFORMAT_XY:
			break;
		
		default:
			break;
	}
	
	this->dataAnalyzer->mutex()->unlock();
	
	// Draw grid
	painter.setRenderHint(QPainter::Antialiasing, false);
	// Grid lines
	painter.setPen(colorValues->grid);
	
	if(this->format < EXPORT_FORMAT_IMAGE) {
		// Draw vertical lines
		for(int div = 1; div < DIVS_TIME / 2; div++) {
			for(int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; dot++) {
				painter.drawLine(QPointF((double) -div - 0.02, (double) -dot / 5), QPointF((double) -div + 0.02, (double) -dot / 5));
				painter.drawLine(QPointF((double) -div - 0.02, (double) dot / 5), QPointF((double) -div + 0.02, (double) dot / 5));
				painter.drawLine(QPointF((double) div - 0.02, (double) -dot / 5), QPointF((double) div + 0.02, (double) -dot / 5));
				painter.drawLine(QPointF((double) div - 0.02, (double) dot / 5), QPointF((double) div + 0.02, (double) dot / 5));
			}
		}
		// Draw horizontal lines
		for(int div = 1; div < DIVS_VOLTAGE / 2; div++) {
			for(int dot = 1; dot < DIVS_TIME / 2 * 5; dot++) {
				painter.drawLine(QPointF((double) -dot / 5, (double) -div - 0.02), QPointF((double) -dot / 5, (double) -div + 0.02));
				painter.drawLine(QPointF((double) dot / 5, (double) -div - 0.02), QPointF((double) dot / 5, (double) -div + 0.02));
				painter.drawLine(QPointF((double) -dot / 5, (double) div - 0.02), QPointF((double) -dot / 5, (double) div + 0.02));
				painter.drawLine(QPointF((double) dot / 5, (double) div - 0.02), QPointF((double) dot / 5, (double) div + 0.02));
			}
		}
	}
	else {
		// Draw vertical lines
		for(int div = 1; div < DIVS_TIME / 2; div++) {
			for(int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; dot++) {
				painter.drawPoint(QPointF(-div, (double) -dot / 5));
				painter.drawPoint(QPointF(-div, (double) dot / 5));
				painter.drawPoint(QPointF(div, (double) -dot / 5));
				painter.drawPoint(QPointF(div, (double) dot / 5));
			}
		}
		// Draw horizontal lines
		for(int div = 1; div < DIVS_VOLTAGE / 2; div++) {
			for(int dot = 1; dot < DIVS_TIME / 2 * 5; dot++) {
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
	
	painter.end();
	
	if(this->format == EXPORT_FORMAT_IMAGE)
		((QPixmap *) paintDevice)->save(this->filename);
	
	return true;
}
