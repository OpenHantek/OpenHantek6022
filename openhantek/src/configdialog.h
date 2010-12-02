////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file configdialog.h
/// \brief Declares the DsoConfigDialog class.
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


#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H


#include <QDialog>


/*#if defined(OS_UNIX)
#define CONFIG_PATH QDir::homePath() + "/.config/paranoiacs.net/openhantek"
#define CONFIG_FILE CONFIG_PATH "/openhantek.conf"
#elif defined(OS_DARWIN)
#define CONFIG_PATH QDir::homePath() + "/Library/Application Support/OpenHantek"
#define CONFIG_FILE CONFIG_PATH "/openhantek.plist"
#elif defined(OS_WINDOWS)
//#define CONFIG_PATH QDir::homePath() + "" // Too hard to get and this OS sucks anyway, ignore it
#define CONFIG_FILE "HKEY_CURRENT_USER\\Software\\paranoiacs.net\\OpenHantek"
#endif*/

#define CONFIG_LIST_WIDTH            128 ///< The width of the page selection widget
#define CONFIG_LIST_ITEMHEIGHT        80 ///< The height of one item in the page selection widget
#define CONFIG_LIST_ICONSIZE          48 ///< The icon size in the page selection widget


class DsoConfigAnalysisPage;
class DsoConfigColorsPage;
class DsoConfigFilesPage;
class DsoConfigScopePage;
class DsoSettings;

class QHBoxLayout;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;


////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigDialog                                        configdialog.h
/// \brief The dialog for the configuration options.
class DsoConfigDialog : public QDialog {
	Q_OBJECT
	
	public:
		DsoConfigDialog(DsoSettings *settings, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~DsoConfigDialog();
	
	public slots:
		void accept();
		void apply();
		
		void changePage(QListWidgetItem *current, QListWidgetItem *previous);
	
	private:
		void createIcons();
		
		DsoSettings *settings;
		
		QVBoxLayout *mainLayout;
		QHBoxLayout *horizontalLayout;
		QHBoxLayout *buttonsLayout;
		
		QListWidget *contentsWidget;
		QStackedWidget *pagesWidget;
		
		DsoConfigAnalysisPage *analysisPage;
		DsoConfigColorsPage *colorsPage;
		DsoConfigFilesPage *filesPage;
		DsoConfigScopePage *scopePage;
		
		QPushButton *acceptButton, *applyButton, *rejectButton;
};


#endif
