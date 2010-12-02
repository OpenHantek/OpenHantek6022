////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  configdialog.cpp
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


#include <QDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>


#include "configdialog.h"

#include "configpages.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// class DsoConfigDialog
/// \brief Creates the configuration dialog and sets initial values.
/// \param settings The target settings object.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
DsoConfigDialog::DsoConfigDialog(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags) {
	this->settings = settings;
	
	this->setWindowTitle(tr("Settings"));
	
	this->contentsWidget = new QListWidget;
	this->contentsWidget->setViewMode(QListView::IconMode);
	this->contentsWidget->setIconSize(QSize(CONFIG_LIST_ICONSIZE, CONFIG_LIST_ICONSIZE));
	this->contentsWidget->setMovement(QListView::Static);
	this->contentsWidget->setGridSize(QSize(CONFIG_LIST_WIDTH - 2 * this->contentsWidget->frameWidth(), CONFIG_LIST_ITEMHEIGHT));
	this->contentsWidget->setMaximumWidth(CONFIG_LIST_WIDTH);
	this->contentsWidget->setMinimumHeight(CONFIG_LIST_ITEMHEIGHT * 3 + 2 * (this->contentsWidget->frameWidth()));
	
	this->analysisPage = new DsoConfigAnalysisPage(this->settings);
	this->colorsPage = new DsoConfigColorsPage(this->settings);
	this->filesPage = new DsoConfigFilesPage(this->settings);
	this->scopePage = new DsoConfigScopePage(this->settings);
	this->pagesWidget = new QStackedWidget;
	this->pagesWidget->addWidget(this->analysisPage);
	this->pagesWidget->addWidget(this->colorsPage);
	this->pagesWidget->addWidget(this->filesPage);
	this->pagesWidget->addWidget(this->scopePage);
	
	this->acceptButton = new QPushButton(tr("&Ok"));
	this->acceptButton->setDefault(true);
	this->applyButton = new QPushButton(tr("&Apply"));
	this->rejectButton = new QPushButton(tr("&Cancel"));
	
	this->createIcons();
	this->contentsWidget->setCurrentRow(0);
	
	this->horizontalLayout = new QHBoxLayout;
	this->horizontalLayout->addWidget(this->contentsWidget);
	this->horizontalLayout->addWidget(this->pagesWidget, 1);
	
	this->buttonsLayout = new QHBoxLayout;
	this->buttonsLayout->setSpacing(8);
	this->buttonsLayout->addStretch(1);
	this->buttonsLayout->addWidget(this->acceptButton);
	this->buttonsLayout->addWidget(this->applyButton);
	this->buttonsLayout->addWidget(this->rejectButton);
	
	this->mainLayout = new QVBoxLayout;
	this->mainLayout->addLayout(this->horizontalLayout);
	this->mainLayout->addStretch(1);
	this->mainLayout->addSpacing(8);
	this->mainLayout->addLayout(this->buttonsLayout);
	this->setLayout(this->mainLayout);
	
	connect(this->acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(this->applyButton, SIGNAL(clicked()), this, SLOT(apply()));
	connect(this->rejectButton, SIGNAL(clicked()), this, SLOT(reject()));
}

/// \brief Cleans up the dialog.
DsoConfigDialog::~DsoConfigDialog() {
}

/// \brief Create the icons for the pages.
void DsoConfigDialog::createIcons() {
	QListWidgetItem *analysisButton = new QListWidgetItem(contentsWidget);
	analysisButton->setIcon(QIcon(":config/analysis.png"));
	analysisButton->setText(tr("Analysis"));
	
	QListWidgetItem *colorsButton = new QListWidgetItem(contentsWidget);
	colorsButton->setIcon(QIcon(":config/colors.png"));
	colorsButton->setText(tr("Colors"));
	
	QListWidgetItem *filesButton = new QListWidgetItem(contentsWidget);
	filesButton->setIcon(QIcon(":config/files.png"));
	filesButton->setText(tr("Files"));
	
	QListWidgetItem *scopeButton = new QListWidgetItem(contentsWidget);
	scopeButton->setIcon(QIcon(":config/scope.png"));
	scopeButton->setText(tr("Scope"));
	
	connect(contentsWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem *)));
}

/// \brief Saves the settings and closes the dialog.
void DsoConfigDialog::accept() {
	this->apply();
	
	QDialog::accept();
}

/// \brief Saves the settings.
void DsoConfigDialog::apply() {
	this->analysisPage->saveSettings();
	this->colorsPage->saveSettings();
	this->filesPage->saveSettings();
	this->scopePage->saveSettings();
}

/// \brief Change the config page.
/// \param current The page that has been selected.
/// \param previous The page that was selected before.
void DsoConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous) {
	if (!current)
		current = previous;
	
	pagesWidget->setCurrentIndex(contentsWidget->row(current));
}
