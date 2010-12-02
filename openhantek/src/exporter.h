////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file exporter.h
/// \brief Declares the Exporter class.
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


#ifndef EXPORTER_H
#define EXPORTER_H


#include <QObject>
#include <QSize>


class DsoSettings;
class DataAnalyzer;


////////////////////////////////////////////////////////////////////////////////
/// \enum ExportFormat                                                exporter.h
/// \brief Possible file formats for the export.
enum ExportFormat {
	EXPORT_FORMAT_PRINTER,
	EXPORT_FORMAT_PDF, EXPORT_FORMAT_PS,
	EXPORT_FORMAT_IMAGE,
	EXPORT_FORMAT_CSV
};

////////////////////////////////////////////////////////////////////////////////
/// \class Exporter                                                   exporter.h
/// \brief Exports the oscilloscope screen to a file or prints it.
class Exporter : public QObject {
	Q_OBJECT
	
	public:
		Exporter(DsoSettings *settings, DataAnalyzer *dataAnalyzer, QWidget *parent = 0);
		~Exporter();
		
		void setFilename(QString filename);
		void setFormat(ExportFormat format);
		
		bool doExport();
	
	private:
		DataAnalyzer *dataAnalyzer;
		DsoSettings *settings;
		
		QString filename;
		ExportFormat format;
		QSize size;
};


#endif
