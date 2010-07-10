////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  main.cpp
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


#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>


#include "openhantek.h"


/// \brief Initialize resources and translations and show the main window.
int main(int argc, char *argv[]) {
	Q_INIT_RESOURCE(application);

	QApplication openHantekApplication(argc, argv);

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	openHantekApplication.installTranslator(&qtTranslator);

	QTranslator openHantekTranslator;
	openHantekTranslator.load("openhantek_" + QLocale::system().name(), QMAKE_TRANSLATIONS_PATH);
	openHantekApplication.installTranslator(&openHantekTranslator);

	OpenHantekMainWindow *openHantekMainWindow = new OpenHantekMainWindow();
	openHantekMainWindow->show();

	return openHantekApplication.exec();
}
