// SPDX-License-Identifier: GPL-2.0+


#define CONFIG_LIST_WIDTH 128      ///< The width of the page selection widget
#define CONFIG_LIST_ITEMHEIGHT 100 ///< The height of one item in the page selection widget
#define CONFIG_LIST_ICONWIDTH 80   ///< The icon size in the page selection widget
#define CONFIG_LIST_ICONHEIGHT 64  ///< The icon size in the page selection widget

#include <QDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "configdialog.h"

#include "DsoConfigAnalysisPage.h"
#include "DsoConfigColorsPage.h"
#include "DsoConfigScopePage.h"

#include "dsosettings.h"

////////////////////////////////////////////////////////////////////////////////
// class DsoConfigDialog
/// \brief Creates the configuration dialog and sets initial values.
/// \param settings The target settings object.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
DsoConfigDialog::DsoConfigDialog( DsoSettings *settings, QWidget *parent ) : QDialog( parent ), settings( settings ) {
    this->setWindowTitle( tr( "Settings" ) );

    this->contentsWidget = new QListWidget;
    this->contentsWidget->setViewMode( QListView::IconMode );
    this->contentsWidget->setIconSize( QSize( CONFIG_LIST_ICONWIDTH, CONFIG_LIST_ICONHEIGHT ) );
    this->contentsWidget->setMovement( QListView::Static );
    this->contentsWidget->setGridSize(
        QSize( CONFIG_LIST_WIDTH - 2 * this->contentsWidget->frameWidth(), CONFIG_LIST_ITEMHEIGHT ) );
    this->contentsWidget->setMaximumWidth( CONFIG_LIST_WIDTH );
    this->contentsWidget->setMinimumWidth( CONFIG_LIST_WIDTH );
    this->contentsWidget->setMinimumHeight( CONFIG_LIST_ITEMHEIGHT * 3 + 2 * ( this->contentsWidget->frameWidth() ) );

    this->analysisPage = new DsoConfigAnalysisPage( settings );
    this->colorsPage = new DsoConfigColorsPage( settings );
    this->scopePage = new DsoConfigScopePage( settings );
    this->pagesWidget = new QStackedWidget;
    this->pagesWidget->addWidget( this->scopePage );
    this->pagesWidget->addWidget( this->analysisPage );
    this->pagesWidget->addWidget( this->colorsPage );

    this->acceptButton = new QPushButton( tr( "&Ok" ) );
    this->acceptButton->setDefault( true );
    this->applyButton = new QPushButton( tr( "&Apply" ) );
    this->rejectButton = new QPushButton( tr( "&Cancel" ) );

    this->createIcons();
    this->contentsWidget->setCurrentRow( 0 );

    this->sectionsLayout = new QHBoxLayout;
    this->sectionsLayout->addWidget( this->contentsWidget );
    this->sectionsLayout->addWidget( this->pagesWidget, 1 );

    this->buttonsLayout = new QHBoxLayout;
    this->buttonsLayout->setSpacing( 8 );
    this->buttonsLayout->addStretch( 1 );
    this->buttonsLayout->addWidget( this->acceptButton );
    this->buttonsLayout->addWidget( this->applyButton );
    this->buttonsLayout->addWidget( this->rejectButton );

    this->mainLayout = new QVBoxLayout;
    mainLayout->addSpacing( 8 );
    this->mainLayout->addLayout( this->sectionsLayout );
    this->mainLayout->addStretch( 1 );
    this->mainLayout->addSpacing( 8 );
    this->mainLayout->addLayout( this->buttonsLayout );
    this->setLayout( this->mainLayout );

    connect( this->acceptButton, &QAbstractButton::clicked, this, &DsoConfigDialog::accept );
    connect( this->applyButton, &QAbstractButton::clicked, this, &DsoConfigDialog::apply );
    connect( this->rejectButton, &QAbstractButton::clicked, this, &QDialog::reject );
}


/// \brief Cleans up the dialog.
DsoConfigDialog::~DsoConfigDialog() {}


/// \brief Create the icons for the pages.
void DsoConfigDialog::createIcons() {
    QListWidgetItem *scopeButton = new QListWidgetItem( contentsWidget );
    scopeButton->setIcon( QIcon( ":config/scope.png" ) );
    scopeButton->setText( tr( "Scope" ) );

    QListWidgetItem *spectrumButton = new QListWidgetItem( contentsWidget );
    spectrumButton->setIcon( QIcon( ":config/spectrum.png" ) );
    spectrumButton->setText( tr( "Analysis" ) );

    QListWidgetItem *colorsButton = new QListWidgetItem( contentsWidget );
    colorsButton->setIcon( QIcon( ":config/colors.png" ) );
    colorsButton->setText( tr( "Colors" ) );

    connect( contentsWidget, &QListWidget::currentItemChanged, this, &DsoConfigDialog::changePage );
}


/// \brief Saves the settings and closes the dialog.
void DsoConfigDialog::accept() {
    this->apply();
    QDialog::accept();
}


/// \brief Saves the settings.
void DsoConfigDialog::apply() {
    this->scopePage->saveSettings();
    this->analysisPage->saveSettings();
    this->colorsPage->saveSettings();
}


/// \brief Change the config page.
/// \param current The page that has been selected.
/// \param previous The page that was selected before.
void DsoConfigDialog::changePage( QListWidgetItem *current, QListWidgetItem *previous ) {
    if ( !current )
        current = previous;
    pagesWidget->setCurrentIndex( contentsWidget->row( current ) );
}
